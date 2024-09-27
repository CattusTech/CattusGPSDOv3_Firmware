add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/u8g2)
target_compile_definitions(u8g2 INTERFACE
    U8G2_USE_DYNAMIC_ALLOC=1
)
