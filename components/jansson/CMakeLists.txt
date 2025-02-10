message(STATUS "---- Jansson CMakeLists.txt ----")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")
message(STATUS "ESP-IDF COMPONENT: ${COMPONENT_DIR}")

if(CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(STATUS "Skipping duplicate execution in root context")
    return()
endif()

option(JANSSON_BUILD_SHARED_LIBS "Build shared libraries." OFF)
option(USE_URANDOM "Use /dev/urandom to seed the hash function." ON)
option(USE_WINDOWS_CRYPTOAPI "Use CryptGenRandom to seed the hash function." ON)
option(USE_DTOA "Use dtoa for optimal floating-point to string conversions." ON)

option(JANSSON_ESP32_OPTIMIZED "ESP32-specific optimizations" ON)
if(JANSSON_ESP32_OPTIMIZED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")  # Optimize for size
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")  # Enable Link Time Optimization
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")  # Keep debug info
  set(JANSSON_BUILD_SHARED_LIBS OFF)  # Use static libraries
endif()

set(JANSSON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(JANSSON_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jansson/include)

configure_file(${JANSSON_SOURCE_DIR}/jansson/cmake/jansson_config.h.cmake
               ${JANSSON_BINARY_DIR}/jansson/include/jansson_config.h)

file(COPY ${JANSSON_SOURCE_DIR}/jansson/src/jansson.h
     DESTINATION ${JANSSON_BINARY_DIR}/jansson/include/)
