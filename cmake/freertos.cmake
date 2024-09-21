add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM INTERFACE
    include
)

target_compile_definitions(freertos_config INTERFACE
    projCOVERAGE_TEST=0
)
set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set(FREERTOS_PORT "GCC_ARM_CM4F" CACHE STRING "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/FreeRTOS-Kernel)
