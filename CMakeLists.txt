cmake_minimum_required (VERSION 3.1)
project (difference)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(vendor/libtwittercpp)
add_subdirectory(vendor/verbly)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)

find_package(PkgConfig)
pkg_check_modules(sqlite3 sqlite3 REQUIRED)
pkg_check_modules(GraphicsMagick GraphicsMagick++ REQUIRED)

include_directories(vendor/libtwittercpp/src ${sqlite3_INCLUDE_DIR} vendor/verbly/lib vendor/curlcpp/include ${GraphicsMagick_INCLUDE_DIRS} vendor/libtwittercpp/vendor/json/src vendor/yaml-cpp/include)
add_executable(difference difference.cpp)
set_property(TARGET difference PROPERTY CXX_STANDARD 11)
set_property(TARGET difference PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(difference verbly ${sqlite3_LIBRARIES} yaml-cpp twitter++ curlcpp ${GraphicsMagick_LIBRARIES})
