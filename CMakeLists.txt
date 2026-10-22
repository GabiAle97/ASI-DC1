cmake_minimum_required(VERSION 3.15)
project(DinoCrisisCutsceneSkipper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Debe ser Win32")
endif()

# Forzar runtime estático
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(${PROJECT_NAME} SHARED src/dllmain.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    external/minhook/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/minhook/lib/libMinHook.x86.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "DinoCrisisCutsceneSkipper"
    SUFFIX ".asi"
    PREFIX ""
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /GS- /EHsc)
    target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH:NO)
endif()