cmake_minimum_required(VERSION 3.8)
project(turtlesim_cpp_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(turtlesim REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(my_interfaces_pkg REQUIRED)

add_executable(turtle1_exec src/turtle1.cpp) 
ament_target_dependencies(turtle1_exec rclcpp turtlesim geometry_msgs my_interfaces_pkg)

add_executable(alive_exec src/alive.cpp) 
ament_target_dependencies(alive_exec rclcpp turtlesim geometry_msgs my_interfaces_pkg)

install(TARGETS
  turtle1_exec
  alive_exec
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()