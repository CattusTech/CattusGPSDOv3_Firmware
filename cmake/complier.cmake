set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# Some default GCC settings
# arm-none-eabi- must be part of path environment
set(TOOLCHAIN_PREFIX                arm-none-eabi-)

set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_RANLIB                    ${CMAKE_C_COMPILER}-ranlib)
set(CMAKE_LINKER                    ${TOOLCHAIN_PREFIX}ld)
set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size)

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(SPEC_FLAGS "")
set(MODEL_FLAGS "-DSTM32G431xx -DSTM32G4xx -DUSE_HAL_DRIVER")
set(WARNING_FLAGS "-Wall -Wextra")
set(SECTIONGC_FLAGS "-fdata-sections -ffunction-sections")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(OPTIMIZE_DEBUG_FLAGS "-g3 -gdwarf")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(OPTIMIZE_DEBUG_FLAGS "-g0 -flto -ffat-lto-objects")
else()
    message(FATAL_ERROR "Not a vaild build type: ${CMAKE_BUILD_TYPE}")
endif()

set(CMAKE_C_FLAGS "${TARGET_FLAGS} ${SPEC_FLAGS} ${MODEL_FLAGS} ${WARNING_FLAGS} ${SECTIONGC_FLAGS} ${OPTIMIZE_DEBUG_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# set(CMAKE_LINK_FLAGS "${TARGET_FLAGS} ${SPEC_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/src/system.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--start-group -Wl,--end-group")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
