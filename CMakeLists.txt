cmake_minimum_required (VERSION 3.1)
project (difference)

add_subdirectory(vendor/curlcpp)
add_subdirectory(vendor/verbly)

find_package(PkgConfig)
pkg_check_modules(GraphicsMagick GraphicsMagick++ REQUIRED)
pkg_check_modules(mastodonpp mastodonpp REQUIRED)
pkg_check_modules(yaml-cpp yaml-cpp REQUIRED)

include_directories(
  ${CURLCPP_SOURCE_DIR}/include
  ${mastodonpp_INCLUDE_DIRS}
  vendor/verbly/lib
  vendor/json
  ${GraphicsMagick_INCLUDE_DIRS}
  ${yaml-cpp_INCLUDE_DIRS})

link_directories(
  ${mastodonpp_LIBRARY_DIRS})

add_executable(difference difference.cpp main.cpp imagenet.cpp)
set_property(TARGET difference PROPERTY CXX_STANDARD 17)
set_property(TARGET difference PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(difference verbly ${yaml-cpp_LIBRARIES} curlcpp ${GraphicsMagick_LIBRARIES} ${mastodonpp_LIBRARIES})
