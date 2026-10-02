#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <cmath>
#include "rclcpp/rclcpp.hpp"
#include "thrusterallocate.hpp"


int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ThrustAllocationNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}