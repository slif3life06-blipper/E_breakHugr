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
    std::thread keyboard_thread_;
    bool running_ = true;
    bool cooldown_active_ = false;
    rclcpp::TimerBase::SharedPtr cooldown_timer_;



    public:
    EstopPublisher()
        : Node("estop_publisher")
    {
        publisher_ = this->create_publisher<std_msgs::msg::Bool>("Estop", 10);

        RCLCPP_INFO(this->get_logger(),
                    "E-stop publisher started. Press 0 to activate E-stop.");

        keyboard_thread_ = std::thread(&EstopPublisher::keyboardLoop, this);
    }



    ~EstopPublisher()
    {
        running_ = false;

        if (keyboard_thread_.joinable()) {
            keyboard_thread_.join();
        }
    }



    class joysticklistener : public rclcpp::Node
    {
        private:  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscriber_;
        public:
        joysticklistener(): Node("joysticklistener")
        {
            subscriber_ = this->create_subscription<sensor_msgs::msg::Joy>(
                "/joy",
                10,  std::bind(&joysticklistener::joy_callback, this,std::placeholders::_1));
        }

        private:
        bool  previous_lb = false;
        bool  previous_rb = false;
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

            if (rb && lb && previous_lb && previous_rb)
            {
                std::cout << "Button LB and RB pressed" << std::endl;
                publisher_ = this->create_publisher<std_msgs::msg::Bool>("Estop", 10);
            }

            previous_lb = lb;
            previous_rb = rb;

        }
        
    };



    private:
    void keyboardLoop()
    {
        // Put terminal into raw mode so individual key presses are detected
        struct termios old_settings, new_settings;

        tcgetattr(STDIN_FILENO, &old_settings);
        new_settings = old_settings;
        new_settings.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);

       

        while (running_ && rclcpp::ok()) {
            char key;
            std::cin.get(key);

            if (key == '0') {

                    if (cooldown_active_) {
                    RCLCPP_WARN(
                        this->get_logger(),
                        "E-stop is on cooldown. Please wait."
                    );
                    continue;
                    }


                std_msgs::msg::Bool msg;
                msg.data = true;

                publisher_->publish(msg);

                RCLCPP_WARN(
                    this->get_logger(),
                    "E-STOP ACTIVATED: Published true to /Estop");

                     cooldown_active_ = true;

                cooldown_timer_ = this->create_wall_timer(
                    std::chrono::seconds(10),
                    [this]()
                    {
                        cooldown_active_ = false;

                        RCLCPP_INFO(
                            this->get_logger(),
                            "E-stop ready. Operator can press 0 again."
                        );
                        cooldown_timer_->cancel();
                        });


            }
        // Restore terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
        }

    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr publisher_;
    std::thread keyboard_thread_;
    
    };
};
