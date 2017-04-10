cmake_minimum_required (VERSION 3.1)
project (difference)

add_subdirectory(vendor/libtwittercpp)
add_subdirectory(vendor/verbly)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)

find_package(PkgConfig)
pkg_check_modules(GraphicsMagick GraphicsMagick++ REQUIRED)

include_directories(vendor/libtwittercpp/src vendor/libtwittercpp/vendor/curlcpp/include vendor/verbly/lib ${GraphicsMagick_INCLUDE_DIRS} vendor/yaml-cpp/include)
add_executable(difference difference.cpp main.cpp)
set_property(TARGET difference PROPERTY CXX_STANDARD 11)
set_property(TARGET difference PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(difference verbly yaml-cpp twitter++ ${GraphicsMagick_LIBRARIES})
