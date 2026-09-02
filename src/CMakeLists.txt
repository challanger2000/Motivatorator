find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(MOTIVATORATOR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(MOTIVATORATOR_GENERATED_PHRASEBANK "${MOTIVATORATOR_GENERATED_DIR}/GeneratedPhraseBank.h")
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

smtg_add_vst3plugin(Motivatorator
    MotivatoratorProcessor.cpp
    MotivatoratorProcessor.h
    MotivatoratorEditor.cpp
    MotivatoratorEditor.h
    MotivatoratorFactory.cpp
    VoicePrototype.cpp
    VoicePrototype.h
    "${MOTIVATORATOR_GENERATED_PHRASEBANK}")

target_include_directories(Motivatorator PRIVATE "${MOTIVATORATOR_GENERATED_DIR}")
target_link_libraries(Motivatorator PRIVATE sdk vstgui_support)

if(WIN32)
    target_sources(Motivatorator PRIVATE
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp
    )
    target_link_libraries(Motivatorator PRIVATE sapi ole32)
    if(MSVC)
        target_compile_options(Motivatorator PRIVATE /utf-8)
    endif()
endif()

smtg_target_add_plugin_resources(Motivatorator
    RESOURCES
    ../resource/Motivatorator.uidesc
    ../resource/motivator_base_760x428.png
    ../resource/gnomi_positive.png
    ../resource/gnomi_negative.png
    ../resource/rocky_positive.png
    ../resource/rocky_negative.png
    ../resource/dom_positive.png
    ../resource/dom_negative.png)

smtg_target_configure_version_file(Motivatorator)
