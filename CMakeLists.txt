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

find_package(PkgConfig)
pkg_check_modules(BOTAN REQUIRED botan-2)
pkg_check_modules(LIBSSH2 REQUIRED libssh2)

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(yaml-cpp REQUIRED)

file(GLOB_RECURSE biscuit_src_files src/*.cpp)
add_executable(biscuit "${biscuit_src_files}")
target_compile_features(biscuit PUBLIC cxx_std_17)
target_include_directories(biscuit PRIVATE "${BOTAN_INCLUDE_DIRS}" "${CMAKE_SOURCE_DIR}/include" "${SQLite3_INCLUDE_DIR}")
target_link_libraries(biscuit PRIVATE ${BOTAN_LIBRARIES} fmt nlohmann_json spdlog ssh2 SQLite::SQLite3 yaml-cpp)
