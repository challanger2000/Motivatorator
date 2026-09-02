smtg_add_vst3plugin(Motivatorator
    MotivatoratorProcessor.cpp
    MotivatoratorProcessor.h
    MotivatoratorEditor.cpp
    MotivatoratorEditor.h
    MotivatoratorFactory.cpp)

target_link_libraries(Motivatorator PRIVATE sdk vstgui_support)

if(WIN32)
    target_sources(Motivatorator PRIVATE
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp
    )
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
