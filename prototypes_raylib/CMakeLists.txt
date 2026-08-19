cmake_minimum_required(VERSION 3.22)
project(RTR4_Prototypes LANGUAGES CXX)

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party")

set(RAYLIB_DIR  "${THIRD_PARTY_DIR}/raylib")
set(IMGUI_DIR   "${THIRD_PARTY_DIR}/imgui")
set(RLIMGUI_DIR "${THIRD_PARTY_DIR}/rlimgui")

set(RAYLIB_INCLUDE_DIR "${RAYLIB_DIR}/include")
set(RAYLIB_LIBRARY     "${RAYLIB_DIR}/lib/raylib.lib")

if(NOT EXISTS "${RAYLIB_INCLUDE_DIR}/raylib.h")
    message(FATAL_ERROR "Could not find 'raylib.h' at: ${RAYLIB_INCLUDE_DIR}")
endif()

add_library(raylib STATIC IMPORTED)
set_target_properties(raylib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_INCLUDE_DIR}"
    IMPORTED_LOCATION "${RAYLIB_LIBRARY}"
)

if(WIN32)
    set_target_properties(raylib PROPERTIES
        INTERFACE_LINK_LIBRARIES "opengl32;gdi32;winmm;shell32;user32"
    )
endif()


set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
)

if(EXISTS "${RLIMGUI_DIR}/rlImGui.cpp")
    set(RLIMGUI_SOURCES "${RLIMGUI_DIR}/rlImGui.cpp")
elseif(EXISTS "${RLIMGUI_DIR}/rlimgui.cpp")
    set(RLIMGUI_SOURCES "${RLIMGUI_DIR}/rlimgui.cpp")
else()
    message(FATAL_ERROR "Could not find 'rlImGui.cpp' inside ${RLIMGUI_DIR}")
endif()

add_executable(raylib_prototype 
    src/main.cpp
    ${IMGUI_SOURCES}
    ${RLIMGUI_SOURCES}
)

target_link_libraries(raylib_prototype PRIVATE raylib)

target_include_directories(raylib_prototype PRIVATE 
    src
    "${IMGUI_DIR}"
    "${RLIMGUI_DIR}"
)

set_target_properties(raylib_prototype PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEBUGGER_WORKING_DIRECTORY    "${CMAKE_SOURCE_DIR}"
)