#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/bool.hpp>

#include <iostream>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <sensor_msgs/msg/joy.hpp>
#include <functional>

class EstopPublisher : public rclcpp::Node
{
private:
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr publisher_;
    bool running_ = true;
    bool cooldown_active_ = false;

    bool previous_lb = false;
    bool previous_rb = false;


    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscriber_;
    rclcpp::TimerBase::SharedPtr cooldown_timer_;



public:
    EstopPublisher()
        : Node("estop_publisher")
    {
        publisher_ = this->create_publisher<std_msgs::msg::Bool>("Estop", 10);

        subscriber_ =
          this->create_subscription<sensor_msgs::msg::Joy>(
              "/joy",
              10,
              std::bind(
                  &EstopPublisher::joy_callback,
                  this,
                  std::placeholders::_1
              )
          );

        RCLCPP_INFO(this->get_logger(),
                    "E-stop publisher started. Press RB and LB to activate E-stop.");

    }



private:

    void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg)
    {
        bool lb = msg->buttons[4];
        bool rb = msg->buttons[5];

        if(lb && !previous_lb)
        {
            std::cout << "Button LB pressed" << std::endl;
        }

        if(rb && !previous_rb)
        {
            std::cout << "Button RB pressed" << std::endl;
        }

        if (rb && lb && !(previous_lb && previous_rb))
        {
            std::cout << "Button LB and RB pressed" << std::endl;
            E_stop();

        }

        previous_lb = lb;
        previous_rb = rb;

    }

    void E_stop()
    {
        if (cooldown_active_)
        {
            RCLCPP_WARN(
                this->get_logger(),
                "E-stop is on cooldown."
            );

            return;
        }


        std_msgs::msg::Bool msg;
        msg.data = true;

        publisher_->publish(msg);

        RCLCPP_WARN(
            this->get_logger(),
            "E-STOP ACTIVATED!"
        );

        cooldown_active_ = true;


        cooldown_timer_ = this->create_wall_timer(
           std::chrono::seconds(10),
           [this]()
           {
               cooldown_active_ = false;

               RCLCPP_INFO(
                   this->get_logger(),
                   "E-stop ready."
               );

               cooldown_timer_->cancel();
           }
       );
    }
};




