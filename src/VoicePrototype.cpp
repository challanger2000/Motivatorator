#include "VoicePrototype.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>

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
    int pendingLanguage {0};
    int pendingCharacter {0};
    std::atomic<double> targetRate {44100.0};

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
            int language = 0;
            int character = 0;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [this] { return stopping || pending; });
                if (stopping) break;
                text = pendingText;
                language = pendingLanguage;
                character = pendingCharacter;
                pending = false;
            }
#ifdef _WIN32
            if (comReady) {
                auto audio = synthesize(text, targetRate.load(std::memory_order_relaxed), language, character);
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
    static std::shared_ptr<std::vector<float>> synthesize(const std::u16string& text, double requestedRate, int language, int character) {
        ISpVoice* voice = nullptr;
        ISpStream* speechStream = nullptr;
        IStream* memoryStream = nullptr;
        ISpObjectToken* voiceToken = nullptr;
        WAVEFORMATEX* waveFormat = nullptr;
        auto cleanup = [&] {
            if (voiceToken) voiceToken->Release();
            if (waveFormat) CoTaskMemFree(waveFormat);
            if (speechStream) speechStream->Release();
            if (memoryStream) memoryStream->Release();
            if (voice) voice->Release();
        };

        if (FAILED(CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_INPROC_SERVER, IID_ISpVoice,
                                    reinterpret_cast<void**>(&voice)))) { cleanup(); return {}; }

        const wchar_t* languageFilter = language == 0 ? L"Language=407" : L"Language=409";
        if (SUCCEEDED(SpFindBestToken(SPCAT_VOICES, languageFilter, nullptr, &voiceToken)) && voiceToken) {
            voice->SetVoice(voiceToken);
        }

        const long speakingRate = character == 0 ? 2L : (character == 1 ? -1L : -3L);
        voice->SetRate(speakingRate);

        if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &memoryStream))) { cleanup(); return {}; }
        if (FAILED(CoCreateInstance(CLSID_SpStream, nullptr, CLSCTX_INPROC_SERVER, IID_ISpStream,
                                    reinterpret_cast<void**>(&speechStream)))) { cleanup(); return {}; }

        waveFormat = static_cast<WAVEFORMATEX*>(CoTaskMemAlloc(sizeof(WAVEFORMATEX)));
        if (!waveFormat) { cleanup(); return {}; }
        std::memset(waveFormat, 0, sizeof(WAVEFORMATEX));
        waveFormat->wFormatTag = WAVE_FORMAT_PCM;
        waveFormat->nChannels = 1;
        waveFormat->nSamplesPerSec = 44100;
        waveFormat->wBitsPerSample = 16;
        waveFormat->nBlockAlign = static_cast<WORD>(waveFormat->nChannels * waveFormat->wBitsPerSample / 8);
        waveFormat->nAvgBytesPerSec = waveFormat->nSamplesPerSec * waveFormat->nBlockAlign;
        waveFormat->cbSize = 0;

        if (FAILED(speechStream->SetBaseStream(memoryStream, SPDFID_WaveFormatEx, waveFormat))) { cleanup(); return {}; }
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
        double target = requestedRate;
        if (target < 8000.0) target = 8000.0;
        if (target > 192000.0) target = 192000.0;
        size_t outCount = static_cast<size_t>(static_cast<double>(pcm.size()) * target / sourceRate);
        if (outCount < 1u) outCount = 1u;
        auto out = std::make_shared<std::vector<float>>(outCount);
        const double step = sourceRate / target;
        for (size_t i = 0; i < outCount; ++i) {
            const double sourcePos = static_cast<double>(i) * step;
            size_t a = static_cast<size_t>(sourcePos);
            if (a >= pcm.size()) a = pcm.size() - 1;
            size_t b = a + 1;
            if (b >= pcm.size()) b = pcm.size() - 1;
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

void VoicePrototype::request(const std::u16string& text, int language, int character) {
    if (text.empty()) return;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->pendingText = text;
        impl_->pendingLanguage = language;
        impl_->pendingCharacter = character;
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
