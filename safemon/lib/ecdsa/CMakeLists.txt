# GMP
find_library(GMP_LIB gmp REQUIRED)
find_path(GMP_INCLUDE gmp.h REQUIRED)

# OpenSSL
find_package(OpenSSL REQUIRED)

# ECDSA library
add_library(ecdsa
    src/bigint.cpp
    src/ec_point.cpp
    src/ecdsa.cpp
)

target_include_directories(ecdsa
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE ${GMP_INCLUDE}
)

target_link_libraries(ecdsa PRIVATE ${GMP_LIB} OpenSSL::Crypto)

# Tests (only when built directly, not when included from parent)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test ecdsa_test)
endif()