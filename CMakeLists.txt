cmake_minimum_required(VERSION 3.10)

project(biscuit VERSION 0.1 LANGUAGES CXX)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Building with debug options")
	add_compile_options(-ggdb3 -O0)
else()
	message("Building with release options")
	add_compile_options(-O2)
endif()

add_compile_options(-Werror -Wall -Wextra -pedantic -pipe)

add_subdirectory(src/biscuit)
