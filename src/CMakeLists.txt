find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(MOTIVATORATOR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(MOTIVATORATOR_GENERATED_PHRASEBANK "${MOTIVATORATOR_GENERATED_DIR}/GeneratedPhraseBank.h")
set(MOTIVATOR150_EDITOR "${MOTIVATORATOR_GENERATED_DIR}/MotivatoratorEditor150.cpp")
file(MAKE_DIRECTORY "${MOTIVATORATOR_GENERATED_DIR}")

execute_process(
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tools/generate_runtime_phrasebank.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}/.."
            --output "${MOTIVATORATOR_GENERATED_PHRASEBANK}"
    RESULT_VARIABLE MOTIVATORATOR_PHRASEBANK_RESULT
    OUTPUT_VARIABLE MOTIVATORATOR_PHRASEBANK_OUTPUT
    ERROR_VARIABLE MOTIVATORATOR_PHRASEBANK_ERROR
)
if(NOT MOTIVATORATOR_PHRASEBANK_RESULT EQUAL 0)
    message(FATAL_ERROR "PhraseBank generation failed: ${MOTIVATORATOR_PHRASEBANK_ERROR}")
endif()
message(STATUS "${MOTIVATORATOR_PHRASEBANK_OUTPUT}")

# Keep the original GUI source untouched. For this isolated build only, generate
# an editor source that starts VSTGUI at 150% zoom. This scales the complete
# existing interface as one unit, including hit areas and all custom views.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/MotivatoratorEditor.cpp" MOTIVATOR150_EDITOR_SOURCE)
string(REPLACE
    "MotivatoratorEditor::MotivatoratorEditor(EditController*c):VST3Editor(c,\"view\",\"Motivatorator.uidesc\"),controller_(c){}"
    "MotivatoratorEditor::MotivatoratorEditor(EditController*c):VST3Editor(c,\"view\",\"Motivatorator.uidesc\"),controller_(c){setZoomFactor(1.5);}"
    MOTIVATOR150_EDITOR_SOURCE
    "${MOTIVATOR150_EDITOR_SOURCE}")
file(WRITE "${MOTIVATOR150_EDITOR}" "${MOTIVATOR150_EDITOR_SOURCE}")

smtg_add_vst3plugin(Motivator150
    MotivatoratorProcessor.cpp
    MotivatoratorProcessor.h
    "${MOTIVATOR150_EDITOR}"
    MotivatoratorEditor.h
    MotivatoratorFactory.cpp
    VoicePrototype.cpp
    VoicePrototype.h
    "${MOTIVATORATOR_GENERATED_PHRASEBANK}")

target_include_directories(Motivator150 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${MOTIVATORATOR_GENERATED_DIR}")
target_link_libraries(Motivator150 PRIVATE sdk vstgui_support)

if(WIN32)
    target_sources(Motivator150 PRIVATE
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp
    )
    target_link_libraries(Motivator150 PRIVATE sapi ole32)
    if(MSVC)
        target_compile_options(Motivator150 PRIVATE /utf-8)
    endif()
endif()

smtg_target_add_plugin_resources(Motivator150
    RESOURCES
    ../resource/Motivatorator.uidesc
    ../resource/motivator_base_760x428.png
    ../resource/gnomi_positive.png
    ../resource/gnomi_negative.png
    ../resource/rocky_positive.png
    ../resource/rocky_negative.png
    ../resource/dom_positive.png
    ../resource/dom_negative.png)

smtg_target_configure_version_file(Motivator150)
