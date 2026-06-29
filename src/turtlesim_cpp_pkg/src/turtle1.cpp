#include "rclcpp/rclcpp.hpp"
#include "turtlesim/msg/pose.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <cmath>

const double PI = 3.141592653589793;
/*
    @brief A custom node to control the turtle (named “turtle1”) which is already spawned in the turtlesim_node.
    TODO: Run a control loop (for example using a timer with a high rate) to reach a given target point. The first turtle on the screen “turtle1” will be the “master” turtle to control. To control the turtle you can subscribe to /turtle1/pose and publish to /turtle1/cmd_vel.
    TODO: The control loop will use a simplified P controller.
    TODO: Subscribe to the /alive_turtles topic to get all current turtles with coordinates. From that information, select a target turtle (turtle to catch).
    TODO: When a turtle has been caught by the master turtle, call the service /catch_turtle advertised by the turtle_spawner node.
*/
class turtle_controller : public rclcpp::Node{
public:
    turtle_controller() : Node("controller_node"){
        //parametre de cible temporaire
        this->declare_parameter("Xtarget", 8.0);
        target_x = this->get_parameter("Xtarget").as_double();
        this->declare_parameter("Ytarget", 4.0);
        target_y = this->get_parameter("Ytarget").as_double();

        turtle1_pose_sub_ = this->create_subscription<turtlesim::msg::Pose>("/turtle1/pose", 10, std::bind(&turtle_controller::position_callback, this, std::placeholders::_1));
        RCLCPP_INFO(this->get_logger(), "subscription au topic /pose !");

        turtle1_cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);
        RCLCPP_INFO(this->get_logger(), "publisher sur /cmd_vel created!");

        timer_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&turtle_controller::controller_callback, this));

    }
    
private:
    void position_callback(const turtlesim::msg::Pose pose_msg){
        this-> pose_ = pose_msg;
        RCLCPP_INFO(this->get_logger(), "position:(x=%f; y=%f; theta=%f)", pose_msg.x, pose_msg.y, pose_msg.theta);
        bool pose_received_ = true;
    }

    void controller_callback(){
        auto cmd_msg = geometry_msgs::msg::Twist();

        if (!pose_received_) return;

        /*TODO: equations distance cible*/
        dist_x = target_x - pose_.x;
        dist_y = target_y - pose_.y;
        distance = std::sqrt((dist_x * dist_x) + (dist_y * dist_y));
        
        /*si la distance a la cible est trop grande*/
        if(distance > 0.5){
            cmd_msg.linear.x = distance;


            /*TODO: equation orientation cible*/
            target_angle = std::atan2(dist_x, dist_y);
            angle_diff = target_angle - pose_.theta;
            if(angle_diff > PI){
                angle_diff -= 2*PI;
            }
            else if(angle_diff < -PI){
                angle_diff += 2*PI;
            }

            cmd_msg.angular.z = angle_diff;
        }

        else{
            cmd_msg.linear.x = 0.0;
            cmd_msg.angular.z = 0.0;
        }
        
        
        
        turtle1_cmd_pub_->publish(cmd_msg);
        RCLCPP_INFO(this->get_logger(), "/cmd_val => (forward_speed = %f; backward_speed= %f; turn_speed= %f)");
    }
    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr turtle1_pose_sub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr turtle1_cmd_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    turtlesim::msg::Pose pose_;
    bool pose_received_ = false;
    double distance;
    float target_angle;
    float angle_diff;

    /*cible temporaire*/
    float dist_x;
    float dist_y;
    double target_x;
    double target_y;
    
};

int main(int argc, char **argv){
    rclcpp::init(argc, argv);
    auto node = std::make_shared<turtle_controller>();

    rclcpp::shutdown();
    return 0;
}