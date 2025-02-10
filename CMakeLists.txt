cmake_minimum_required(VERSION 3.16)
add_compile_options(-Wno-unused-function)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_world)

# Add 'components/libbluesky' as an additional component
list(APPEND EXTRA_COMPONENT_DIRS components/libbluesky components/jansson)