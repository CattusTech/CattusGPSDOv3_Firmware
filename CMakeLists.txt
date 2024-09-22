cmake_minimum_required(VERSION 3.23)

project(gpsdo_v3_fw NONE)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include("cmake/complier.cmake")

enable_language(C ASM)

#file(GLOB_RECURSE PROJECT_SOURCES_CXX ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE PROJECT_SOURCES_C ${CMAKE_SOURCE_DIR}/src/*.c)
file(GLOB HAL_SOURCES_C ${CMAKE_SOURCE_DIR}/thirdparty/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Src/*.c)
file(GLOB HAL_EXAMPLES_C ${CMAKE_SOURCE_DIR}/thirdparty/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Src/*template.c)
list(REMOVE_ITEM HAL_SOURCES_C ${HAL_EXAMPLES_C})

include("cmake/freertos.cmake")
include("cmake/u8g2.cmake")

add_executable(gpsdo_v3_fw)
target_include_directories(gpsdo_v3_fw PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/thirdparty/STM32CubeG4/Drivers/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_SOURCE_DIR}/thirdparty/STM32CubeG4/Drivers/CMSIS/Core/Include
    ${CMAKE_SOURCE_DIR}/thirdparty/FreeRTOS-Kernel/include
    ${CMAKE_SOURCE_DIR}/thirdparty/u8g2/csrc/
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_sources(gpsdo_v3_fw PRIVATE
    ${PROJECT_SOURCES_CXX}
    ${PROJECT_SOURCES_C}
    ${HAL_SOURCES_C}
    ${CMAKE_SOURCE_DIR}/src/startup.s
)
target_link_libraries(gpsdo_v3_fw
    freertos_kernel
    freertos_config
    u8g2
)
add_custom_command(
    TARGET gpsdo_v3_fw
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex gpsdo_v3_fw.elf gpsdo_v3_fw.hex
)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
