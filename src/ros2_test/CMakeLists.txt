cmake_minimum_required(VERSION 3.8)

project(ros2_test)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


#include <std_msgs/msg/bool.hpp>

#include <iostream>
#include <thread>
#include <termios.h>
#include <unistd.h>

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  # Skip copyright check
  set(ament_cmake_copyright_FOUND TRUE)

  # Skip cpplint
  set(ament_cmake_cpplint_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
endif()

# Create executable
add_executable(ros2_test
  src/ros2_test.cpp
        include/ros2_test.hpp
        ../Safety_net/src/Safety_net.cpp
        ../Safety_net/src/Safety_net.h
)

# Find header files
target_include_directories(ros2_test PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ROS 2 dependencies
ament_target_dependencies(ros2_test
  rclcpp
  std_msgs
  sensor_msgs
)

# Install header files
install(
  DIRECTORY include/
  DESTINATION include
)

# Install executable
install(
  TARGETS ros2_test
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()