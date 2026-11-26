cmake_minimum_required(VERSION 3.15)
project(desktop-rat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)

# Исходники
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.h")

# Опция: с консолью (отладка) или чистый GUI (финальный)
option(BUILD_CONSOLE "Build as console application for debugging" OFF)

if(BUILD_CONSOLE)
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
endif()

# Include-пути: src/ и libs/ (там stb_image.h)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs
)

# Системные библиотеки
target_link_libraries(${PROJECT_NAME} PRIVATE
    user32
    gdi32
    shell32
    comctl32
    ole32
    msimg32    # для TransparentBlt (если понадобится позже)
)

# Копируем папку resources/ в каталог сборки после сборки
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    COMMENT "Copying resources to build directory"
)

# Оптимизация Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O2)
        target_link_options(${PROJECT_NAME} PRIVATE -s)
    endif()
endif()
