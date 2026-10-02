#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/header.hpp>
#include "std_msgs/msg/int32_multi_array.hpp"
#include "std_msgs/msg/multi_array_dimension.hpp"
#include <geometry_msgs/msg/twist.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <sensor_msgs/msg/joy_feedback.hpp>

using namespace std::placeholders;
using namespace std::chrono_literals;

class ThrustAllocationNode : public rclcpp::Node
{
private:
// sub'n'pub
    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscriber_;
    rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;

// Hull constants
    double mass_;
    double d1x_;
    double d1y_;
    double d2x_;
    double d2y_;
    double d3x_;
    double d3y_;
// Joystick commands
    double Fx_;
    double Fy_;
    double tau_;
    double deadzone_;
// Thruster output
    double T1_;
    double T2_;
    double T3_;

public:
    ThrustAllocationNode() : Node("thrust_allocation_node")
    {
// Parametres
        // Hull
        this->declare_parameter("mass", rclcpp::PARAMETER_DOUBLE);
        this->declare_parameter("d1x", 0.35);
        this->declare_parameter("d1y", 0.1575);
        this->declare_parameter("d2x", 0.35);
        this->declare_parameter("d2y", -0.1575);
        this->declare_parameter("d3x", 0.25);
        this->declare_parameter("d3y", 0.35);
        this->declare_parameter("deadzone", 0.05);

        mass_ = this->get_parameter("mass").as_double();
        d1x_  = this->get_parameter("d1x").as_double();
        d1y_  = this->get_parameter("d1y").as_double();
        d2x_  = this->get_parameter("d2x").as_double();
        d2y_  = this->get_parameter("d2y").as_double();
        d3x_  = this->get_parameter("d3x").as_double();
        d3y_  = this->get_parameter("d3y").as_double();

        // Controller
        deadzone_ = this->get_parameter("deadzone").as_double();

        // Recieve joy
        subscriber_ = create_subscription<sensor_msgs::msg::Joy> (
            "/joy", 10,
            std::bind(&ThrustAllocationNode::joystickCommands, this, _1));

// Publish
        publisher_ = create_publisher<std_msgs::msg::Int32MultiArray>(
            "thrust_allocation", 10);
        // Initialize command and output values
        Fx_ = 0.0;
        Fy_ = 0.0;
        tau_ = 0.0;
        T1_ = 0;
        T2_ = 0;
        T3_ = 0;

        auto timer_callback =
        [this]() -> void {
            // Compute thruster outputs from latest joystick inputs
            this->thrusterForce();
            auto message = std_msgs::msg::Int32MultiArray();
            message.data = {(int32_t)T1_, (int32_t)T2_, (int32_t)T3_};
            this->publisher_->publish(message);
        };
        timer_ = this->create_wall_timer(30ms, timer_callback);
    }

// Interpret sticks
    void joystickCommands(const sensor_msgs::msg::Joy::ConstSharedPtr msg)
    {
        // Initializing declarations
        Fx_  =  msg->axes.at(1);   // Surge
        Fy_  = -msg->axes.at(0);   // Sway
        tau_ = -msg->axes.at(3);   // Yaw
        deadzone_ = 0.05;
        // Simple deadzone logic
        if(abs(Fx_)  <= deadzone_)
            Fx_ = 0;
        if(abs(Fy_)  <= deadzone_)
            Fy_ = 0;
        if(abs(tau_) <= deadzone_)
            tau_ = 0;
    }

// Math
    double scaleJoystick(double x)
    {
        if (x >= 0.0f)
            return std::lround(1500 + 400*x);
        else
            return std::lround(1500 + 200*x);
    }

    void thrusterForce()
    {
        T1_ = 2*Fx_ * ((d2y_)/(d2y_- d1y_)) + Fy_* (d3x_/(d2y_ - d1y_)) - tau_/3 * (1/(d2y_ - d1y_));
        T2_ = - 2*Fx_ * ((d1y_)/(d2y_- d1y_)) - Fy_* (d3x_/(d2y_ - d1y_)) + tau_/3 * (1/(d2y_ - d1y_));
        T3_ = Fy_;

        double maxThrust = std::max({std::abs(T1_), std::abs(T2_), std::abs(T3_)});
        {
            if(maxThrust > 1.0){
                T1_ /= maxThrust;
                T2_ /= maxThrust;
                T3_ /= maxThrust;
            }
        }

        T1_ = scaleJoystick(T1_);
        T2_ = scaleJoystick(T2_);
        T3_ = scaleJoystick(T3_);

    }
};