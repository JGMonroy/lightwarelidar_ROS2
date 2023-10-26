cmake_minimum_required(VERSION 3.8)
project(lightwarelidar2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_library(lwNx src/lwNx.cpp)
add_library(platformLinux src/linux/platformLinux.cpp)
add_library(serialPortLinux src/linux/lwSerialPortLinux.cpp)

include_directories(
  include/lightwarelidar2
  include/lightwarelidar2/linux
)

add_executable(sf30c src/sf30c.cpp)
ament_target_dependencies(sf30c rclcpp sensor_msgs)

add_executable(sf40c src/sf40c.cpp)
ament_target_dependencies(sf40c rclcpp sensor_msgs)

add_executable(sf45b src/sf45b.cpp)
ament_target_dependencies(sf45b rclcpp sensor_msgs)

# target_include_directories(sf40c PRIVATE
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/lightwarelidar2>
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/lightwarelidar2/linux>
#   $<INSTALL_INTERFACE:include>)

target_link_libraries(sf30c
  lwNx
  platformLinux
  serialPortLinux
)

target_link_libraries(sf40c
  lwNx
  platformLinux
  serialPortLinux
)

target_link_libraries(sf45b
  lwNx
  platformLinux
  serialPortLinux
)

install(TARGETS
  sf30c
  sf40c
  sf45b
  DESTINATION lib/${PROJECT_NAME})
  
ament_package()
