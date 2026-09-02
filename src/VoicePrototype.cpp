#include "VoicePrototype.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <sapi.h>
#include <sphelper.h>
#endif

namespace Steinberg::Vst {

struct VoicePrototype::Impl {
    std::mutex mutex;
    std::condition_variable cv;
    std::thread worker;
    bool stopping {false};
    bool pending {false};
    std::u16string pendingText;
    std::atomic<double> targetRate {44100.0};

    // Published by worker, then consumed lock-free by the audio thread.
    std::shared_ptr<const std::vector<float>> published;
    std::shared_ptr<const std::vector<float>> playing;
    std::atomic<uint64_t> generation {0};
    uint64_t seenGeneration {0};
    size_t playPos {0};

    Impl() : worker([this] { run(); }) {}
    ~Impl() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
        }
        cv.notify_one();
        if (worker.joinable()) worker.join();
    }

    void run() {
#ifdef _WIN32
        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool comReady = SUCCEEDED(comResult);
#endif
        for (;;) {
            std::u16string text;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [this] { return stopping || pending; });
                if (stopping) break;
                text = pendingText;
                pending = false;
            }
#ifdef _WIN32
            if (comReady) {
                auto audio = synthesize(text, targetRate.load(std::memory_order_relaxed));
                if (audio && !audio->empty()) {
                    std::atomic_store_explicit(&published,
                        std::shared_ptr<const std::vector<float>>(std::move(audio)),
                        std::memory_order_release);
                    generation.fetch_add(1, std::memory_order_release);
                }
            }
#endif
        }
#ifdef _WIN32
        if (comReady) CoUninitialize();
#endif
    }

#ifdef _WIN32
    static std::shared_ptr<std::vector<float>> synthesize(const std::u16string& text, double requestedRate) {
        ISpVoice* voice = nullptr;
        ISpStream* speechStream = nullptr;
        IStream* memoryStream = nullptr;
        auto cleanup = [&] {
            if (speechStream) speechStream->Release();
            if (memoryStream) memoryStream->Release();
            if (voice) voice->Release();
        };

        if (FAILED(CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_INPROC_SERVER, IID_ISpVoice,
                                    reinterpret_cast<void**>(&voice)))) { cleanup(); return {}; }
        if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &memoryStream))) { cleanup(); return {}; }
        if (FAILED(CoCreateInstance(CLSID_SpStream, nullptr, CLSCTX_INPROC_SERVER, IID_ISpStream,
                                    reinterpret_cast<void**>(&speechStream)))) { cleanup(); return {}; }

        // Fixed 44.1 kHz, 16-bit mono PCM. Defining the WAVEFORMATEX explicitly avoids
        // depending on SDK-specific SpConvertStreamFormatEnum overloads.
        WAVEFORMATEX waveFormat {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 1;
        waveFormat.nSamplesPerSec = 44100;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = static_cast<WORD>(waveFormat.nChannels * waveFormat.wBitsPerSample / 8);
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;
        if (FAILED(speechStream->SetBaseStream(memoryStream, SPDFID_WaveFormatEx, &waveFormat))) { cleanup(); return {}; }
        if (FAILED(voice->SetOutput(speechStream, TRUE))) { cleanup(); return {}; }

        std::wstring wide;
        wide.reserve(text.size());
        for (char16_t c : text) wide.push_back(static_cast<wchar_t>(c));
        if (FAILED(voice->Speak(wide.c_str(), SPF_DEFAULT, nullptr))) { cleanup(); return {}; }
        speechStream->Commit(STGC_DEFAULT);

        STATSTG stat {};
        if (FAILED(memoryStream->Stat(&stat, STATFLAG_NONAME))) { cleanup(); return {}; }
        const auto byteCount = static_cast<size_t>(stat.cbSize.QuadPart);
        if (byteCount < sizeof(int16_t)) { cleanup(); return {}; }
        LARGE_INTEGER zero {};
        memoryStream->Seek(zero, STREAM_SEEK_SET, nullptr);
        std::vector<int16_t> pcm(byteCount / sizeof(int16_t));
        ULONG bytesRead = 0;
        if (FAILED(memoryStream->Read(pcm.data(), static_cast<ULONG>(pcm.size() * sizeof(int16_t)), &bytesRead))) {
            cleanup(); return {};
        }
        pcm.resize(bytesRead / sizeof(int16_t));
        cleanup();
        if (pcm.empty()) return {};

        constexpr double sourceRate = 44100.0;
        const double target = (std::max)(8000.0, (std::min)(requestedRate, 192000.0));
        const size_t outCount = (std::max)(size_t{1}, static_cast<size_t>(pcm.size() * target / sourceRate));
        auto out = std::make_shared<std::vector<float>>(outCount);
        const double step = sourceRate / target;
        for (size_t i = 0; i < outCount; ++i) {
            const double sourcePos = i * step;
            const size_t a = (std::min)(static_cast<size_t>(sourcePos), pcm.size() - 1);
            const size_t b = (std::min)(a + 1, pcm.size() - 1);
            const double frac = sourcePos - static_cast<double>(a);
            const double sample = pcm[a] * (1.0 - frac) + pcm[b] * frac;
            (*out)[i] = static_cast<float>(sample / 32768.0);
        }
        return out;
    }
#endif
};

VoicePrototype::VoicePrototype() : impl_(std::make_unique<Impl>()) {}
VoicePrototype::~VoicePrototype() = default;

void VoicePrototype::setSampleRate(double sampleRate) {
    impl_->targetRate.store(sampleRate > 1.0 ? sampleRate : 44100.0, std::memory_order_relaxed);
}

void VoicePrototype::request(const std::u16string& text) {
    if (text.empty()) return;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->pendingText = text;
        impl_->pending = true;
    }
    impl_->cv.notify_one();
}

float VoicePrototype::nextSample() noexcept {
    const auto generation = impl_->generation.load(std::memory_order_acquire);
    if (generation != impl_->seenGeneration) {
        impl_->playing = std::atomic_load_explicit(&impl_->published, std::memory_order_acquire);
        impl_->seenGeneration = generation;
        impl_->playPos = 0;
    }
    if (!impl_->playing || impl_->playPos >= impl_->playing->size()) return 0.0f;
    return (*impl_->playing)[impl_->playPos++];
}

void VoicePrototype::resetPlayback() noexcept {
    impl_->playing.reset();
    impl_->playPos = 0;
    impl_->seenGeneration = impl_->generation.load(std::memory_order_acquire);
}

} // namespace Steinberg::Vst
