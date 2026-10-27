cmake_minimum_required(VERSION 3.21)
project(Schiffbruch LANGUAGES CXX)
include(GNUInstallDirs)
include(CTest)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2)
FetchContent_MakeAvailable(SFML)

add_executable(Schiffbruch WIN32
    src/FrameSurface.cpp
    src/NativeDisplay.cpp
    src/ModernUI.cpp
    src/Crafting.cpp
    src/GameClock.cpp
    src/Nightfall.cpp
    src/SceneView.cpp
    src/Localization.cpp
    src/Interaction.cpp
    src/Survival.cpp
    src/WeatherEffects.cpp
    src/Action.cpp
    src/Application.cpp
    src/ddutil.cpp
    src/Direct.cpp
    src/Expedition.cpp
    src/Game.cpp
    src/globals.cpp
    src/main.cpp
    src/Math.cpp
    src/Renderer.cpp
    src/Routing.cpp
    src/Sound.cpp
    src/World.cpp

    src/Action.hpp
    src/Application.hpp
    src/constants.hpp
    src/ddutil.h
    src/Direct.hpp
    src/Expedition.hpp
    src/ExpeditionRules.hpp
    src/PixelFormat.hpp
    src/ArtPalette.hpp
    src/extern.hpp
    src/Game.hpp
    src/headers.hpp
    src/Math.hpp
    src/Menu.hpp
    src/Renderer.hpp
    src/Routing.hpp
    src/Sound.hpp
    src/State.hpp
    src/types.hpp
    src/World.hpp

    src/Schiffbruch.rc
)

target_link_libraries(Schiffbruch PRIVATE sfml-graphics sfml-audio sfml-main gdi32 msimg32 imm32)
target_compile_features(Schiffbruch PRIVATE cxx_std_17)
option(SCHIFFBRUCH_QA_PREVIEW "Enable isolated experience preview (never for release)" OFF)
if(SCHIFFBRUCH_QA_PREVIEW)
    target_compile_definitions(Schiffbruch PRIVATE SCHIFFBRUCH_QA_PREVIEW)
endif()
if(MSVC)
    target_compile_options(Schiffbruch PRIVATE /utf-8)
endif()

if(BUILD_TESTING)
    foreach(test_name expedition_rules pixel_format)
        add_executable(${test_name}_test tests/${test_name}_test.cpp)
        target_include_directories(${test_name}_test PRIVATE src)
        target_compile_features(${test_name}_test PRIVATE cxx_std_17)
        add_test(NAME ${test_name} COMMAND ${test_name}_test)
    endforeach()
    add_executable(expedition_integration_test tests/expedition_integration_test.cpp src/Expedition.cpp src/Crafting.cpp src/globals.cpp src/FrameSurface.cpp src/WeatherEffects.cpp)
    target_include_directories(expedition_integration_test PRIVATE src)
    target_link_libraries(expedition_integration_test PRIVATE sfml-graphics sfml-audio gdi32 msimg32)
    target_compile_features(expedition_integration_test PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(expedition_integration_test PRIVATE /utf-8)
    endif()
    add_test(NAME expedition_integration COMMAND expedition_integration_test)
    add_executable(frame_surface_test tests/frame_surface_test.cpp src/FrameSurface.cpp)
    target_include_directories(frame_surface_test PRIVATE src)
    target_link_libraries(frame_surface_test PRIVATE gdi32)
    target_compile_features(frame_surface_test PRIVATE cxx_std_17)
    add_test(NAME frame_surface COMMAND frame_surface_test)
    add_executable(localization_test tests/localization_test.cpp)
    target_include_directories(localization_test PRIVATE src)
    target_compile_features(localization_test PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(localization_test PRIVATE /utf-8)
    endif()
    add_dependencies(localization_test Schiffbruch)
    add_test(NAME localization COMMAND localization_test)
    add_executable(experience_test tests/experience_test.cpp src/Interaction.cpp src/Survival.cpp src/WeatherEffects.cpp src/globals.cpp src/FrameSurface.cpp)
    target_include_directories(experience_test PRIVATE src)
    target_compile_features(experience_test PRIVATE cxx_std_17)
    target_link_libraries(experience_test PRIVATE sfml-graphics sfml-audio gdi32 msimg32)
    if(MSVC)
        target_compile_options(experience_test PRIVATE /utf-8)
    endif()
    add_test(NAME experience COMMAND experience_test)
    add_executable(pace_view_test tests/pace_view_test.cpp src/SceneView.cpp src/FrameSurface.cpp src/globals.cpp)
    target_include_directories(pace_view_test PRIVATE src)
    target_compile_features(pace_view_test PRIVATE cxx_std_17)
    target_link_libraries(pace_view_test PRIVATE sfml-graphics sfml-audio gdi32)
    add_test(NAME pace_view COMMAND pace_view_test)
    add_executable(crafting_test tests/crafting_test.cpp src/Crafting.cpp src/Expedition.cpp src/WeatherEffects.cpp src/globals.cpp src/FrameSurface.cpp)
    target_include_directories(crafting_test PRIVATE src)
    target_compile_features(crafting_test PRIVATE cxx_std_17)
    target_link_libraries(crafting_test PRIVATE sfml-graphics sfml-audio gdi32 msimg32)
    add_test(NAME crafting COMMAND crafting_test)
    add_executable(realtime_test tests/realtime_test.cpp src/GameClock.cpp src/Nightfall.cpp src/globals.cpp src/FrameSurface.cpp)
    target_include_directories(realtime_test PRIVATE src)
    target_compile_features(realtime_test PRIVATE cxx_std_17)
    target_link_libraries(realtime_test PRIVATE sfml-graphics sfml-audio gdi32)
    add_test(NAME realtime COMMAND realtime_test)
    if(MSVC)
        target_compile_options(crafting_test PRIVATE /utf-8)
        target_compile_options(realtime_test PRIVATE /utf-8)
    endif()
endif()

# Copy DLLs next to the executable
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET Schiffbruch POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:Schiffbruch> $<TARGET_FILE_DIR:Schiffbruch> COMMAND_EXPAND_LISTS)
endif()

install(DIRECTORY bin/ DESTINATION ${CMAKE_INSTALL_BINDIR})

install(TARGETS Schiffbruch)
install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES Start-Game.vbs DESTINATION . RENAME "开始游戏.vbs")
install(FILES README.md README.upstream.md IMPROVEMENTS.zh-CN.md THIRD_PARTY.md LICENSE "行动与制作对照.md" DESTINATION .)
install(DIRECTORY licenses/ DESTINATION licenses)
