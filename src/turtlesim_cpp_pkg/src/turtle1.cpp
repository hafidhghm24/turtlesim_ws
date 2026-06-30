#include "rclcpp/rclcpp.hpp"
#include "turtlesim/msg/pose.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <cmath> 

const double PI = 3.141592653589793;

class turtle_controller : public rclcpp::Node {
public:
    turtle_controller() : Node("controller_node") {

        this->declare_parameter("Xtarget", 8.0);
        this->declare_parameter("Ytarget", 4.0);

        target_x = this->get_parameter("Xtarget").as_double();
        target_y = this->get_parameter("Ytarget").as_double();

        turtle1_pose_sub_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose", 10, 
            std::bind(&turtle_controller::position_callback, this, std::placeholders::_1));
        
        RCLCPP_INFO(this->get_logger(), "Abonnement au topic /turtle1/pose établi !");

        turtle1_cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(
            "/turtle1/cmd_vel", 10);
        
        RCLCPP_INFO(this->get_logger(), "Publisher sur /turtle1/cmd_vel créé !");

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100), 
            std::bind(&turtle_controller::controller_callback, this));
    }

private:
    void position_callback(const turtlesim::msg::Pose pose_msg) {

        this->pose_ = pose_msg;
        

        this->pose_received_ = true;
    }

    void controller_callback() {

        if (!pose_received_) {
            return;
        }

        auto cmd_msg = geometry_msgs::msg::Twist();

        dist_x = target_x - pose_.x;
        dist_y = target_y - pose_.y;
        
        distance = std::sqrt((dist_x * dist_x) + (dist_y * dist_y));

        if (distance > 0.1) {

            cmd_msg.linear.x = 2.0 * distance;

            target_angle = std::atan2(dist_y, dist_x);
            angle_diff = target_angle - pose_.theta;

            if (angle_diff > PI) {
                angle_diff -= 2.0 * PI;
            } else if (angle_diff < -PI) {
                angle_diff += 2.0 * PI;
            }

            cmd_msg.angular.z = 6.0 * angle_diff;
        } 
        else {
            cmd_msg.linear.x = 0.0;
            cmd_msg.angular.z = 0.0;
            RCLCPP_INFO(this->get_logger(), "Cible atteinte!");
        }

        this->turtle1_cmd_pub_->publish(cmd_msg);

        RCLCPP_INFO(
            this->get_logger(), 
            "/cmd_vel => Vitesse Linéaire = %.2f m/s, Vitesse Angulaire = %.2f rad/s",
            cmd_msg.linear.x,
            cmd_msg.angular.z
        );
    }

    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr turtle1_pose_sub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr turtle1_cmd_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    turtlesim::msg::Pose pose_;
    bool pose_received_ = false;
    double distance;
    float target_angle;
    float angle_diff;

    // Cible
    float dist_x;
    float dist_y;
    double target_x;
    double target_y;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    
    auto node = std::make_shared<turtle_controller>();
    rclcpp::spin(node);
    
    rclcpp::shutdown();
    return 0;
}