cmake_minimum_required (VERSION 3.1)
project (difference)

add_subdirectory(vendor/libtwittercpp)
add_subdirectory(vendor/verbly)

find_package(PkgConfig)
pkg_check_modules(GraphicsMagick GraphicsMagick++ REQUIRED)
pkg_check_modules(yaml-cpp yaml-cpp REQUIRED)

include_directories(
  vendor/libtwittercpp/src
  vendor/libtwittercpp/vendor/curlcpp/include
  vendor/verbly/lib
  ${GraphicsMagick_INCLUDE_DIRS}
  ${yaml-cpp_INCLUDE_DIRS})

add_executable(difference difference.cpp main.cpp)
set_property(TARGET difference PROPERTY CXX_STANDARD 11)
set_property(TARGET difference PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(difference verbly ${yaml-cpp_LIBRARIES} twitter++ ${GraphicsMagick_LIBRARIES})
