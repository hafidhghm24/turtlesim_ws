#include "rclcpp/rclcpp.hpp"
#include "turtlesim/msg/pose.hpp"
#include "geometry_msgs/msg/twist.hpp"

#include "my_interfaces_pkg/msg/turtle.hpp"
#include "my_interfaces_pkg/msg/turtle_liste.hpp"
#include "my_interfaces_pkg/srv/catch_turtle.hpp"

#include <cmath> 


using namespace std::chrono_literals;

const double PI = 3.141592653589793;

class turtle_controller : public rclcpp::Node {
public:
    turtle_controller() : Node("controller_node") {

        //PARAMETRES
        this->declare_parameter("closest_first", true);
        closest_turtle_first_ = this->get_parameter("closest_first").as_bool();
        
        
        //recuperer la position de notre tortue principale avec le service /turtle1/pose
        turtle1_pose_sub_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose", 10, 
            std::bind(&turtle_controller::position_callback, this, std::placeholders::_1));
        RCLCPP_INFO(this->get_logger(), "Abonnement => /turtle1/pose créé !");
        
        //on recupére la TurtleListe publié sur /alive_turtles
        turtle1_spawn_sub_ = this->create_subscription<my_interfaces_pkg::msg::TurtleListe>(
            "/alive_turtles", 10, 
            std::bind(&turtle_controller::alive_callback, this, std::placeholders::_1));
        RCLCPP_INFO(this->get_logger(), "Abonnement => /alive_turtles créé !");

        //envoie de la commande de mouvement sur /cmd_vel
        turtle1_cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(
            "/turtle1/cmd_vel", 10);
        RCLCPP_INFO(this->get_logger(), "Publisher => /turtle1/cmd_vel créé !");

        //apelle de controller_callback() tout les 100ms
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100), 
            std::bind(&turtle_controller::controller_callback, this));

        //pour étre informer si la tortue cible es atteinte
        client_kill_ = this->create_client<my_interfaces_pkg::srv::CatchTurtle>("catch_turtle");
        RCLCPP_INFO(this->get_logger(), "client du service /CatchTurtle !");
    }

private:
    /**
     * @brief reception depuis /pose la position de la main tortue
     */
    void position_callback(const turtlesim::msg::Pose::SharedPtr pose_msg) {
        this->pose_ = *pose_msg;
        this->pose_received_ = true;
    }

    /**
     * @brief mise a jour de la tortue cible a partir de la liste (trouve la plus proche) ou (par ordre de spawn)
     */
    void alive_callback(const my_interfaces_pkg::msg::TurtleListe::SharedPtr msg) {
        if (!msg->turtles.empty()) {

            //on cherche la tortue la plus proche
            if(closest_turtle_first_){
                min_distance = 10000;
                for(size_t i = 0; i < msg->turtles.size(); i++){
                    dist_x = msg->turtles[i].x - this->pose_.x;
                    dist_y = msg->turtles[i].y - this->pose_.y;
                    distance = std::sqrt((dist_x * dist_x) + (dist_y * dist_y));

                    if(distance<min_distance){
                        min_distance = distance;
                        closest_target_ = msg->turtles[i];
                    }
                }
                this->turtle_target_ = closest_target_;

            }
            //on cherche les tortues par ordre de spawn
            else{
                this->turtle_target_ = msg->turtles[0];
            }

             
        } else {
            this->turtle_target_.name = ""; 
        }
    }

    /**
     * @brief sil y a une tortue cible on clalcule la trajectoire et langle pour ensuite publier sur /cmd_vel la vitesse de translation et rotation pour y aller ensuite appelle de catch_turtle() pour la supprimer
     */
    void controller_callback() {
        if (!pose_received_ || turtle_target_.name == "") {
            return;
        }

        auto cmd_msg = geometry_msgs::msg::Twist();

        dist_x = turtle_target_.x - pose_.x;
        dist_y = turtle_target_.y - pose_.y;
        
        distance = std::sqrt((dist_x * dist_x) + (dist_y * dist_y));

        if (distance > 0.5) { 
            cmd_msg.linear.x = 2.0 * distance;

            target_angle = std::atan2(dist_y, dist_x);
            angle_diff = target_angle - pose_.theta;

            if (angle_diff > PI) {
                angle_diff -= 2.0 * PI;
            } else if (angle_diff < -PI) {
                angle_diff += 2.0 * PI;
            }

            cmd_msg.angular.z = 6.0 * angle_diff;
            RCLCPP_INFO(this->get_logger(), "vitesse deplacement (forward = %f, rotation = %f)", cmd_msg.linear.x, cmd_msg.angular.z);
        } 
        else {
            cmd_msg.linear.x = 0.0;
            cmd_msg.angular.z = 0.0;
            this->catch_turtle(turtle_target_.name);
        }

        this->turtle1_cmd_pub_->publish(cmd_msg);
    }
    
    /**
     * @brief envoyer le nom de la tortue quon a ciblé sur /catch_turtle vers alive.cpp pour la kill apres ça on apelle callback_kill_srv()
     */
    void catch_turtle(const std::string& turtle_name){
        while(!client_kill_->wait_for_service(1s)){
            RCLCPP_WARN(this->get_logger(), "attente du service /catch_turtle");
        }

        auto request = std::make_shared<my_interfaces_pkg::srv::CatchTurtle::Request>();
        request->name = turtle_name;
        
        this->client_kill_->async_send_request(
            request,
            [this](rclcpp::Client<my_interfaces_pkg::srv::CatchTurtle>::SharedFuture future) {
                this->callback_kill_srv(future);
            }
        );
    }

    /**
     * @brief log pour savoir si la suppression a été effectuer avec succes 
     */
    void callback_kill_srv(rclcpp::Client<my_interfaces_pkg::srv::CatchTurtle>::SharedFuture future){

        auto response = future.get();
        if(response->success == true){
            RCLCPP_INFO(this->get_logger(), "success de la suppression");
        }
        else{
            RCLCPP_INFO(this->get_logger(), "echec de la suppression");
        }
    }
    

    //----DECLARATION DES VARIABLE INTERNE----
    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr turtle1_pose_sub_;
    rclcpp::Subscription<my_interfaces_pkg::msg::TurtleListe>::SharedPtr turtle1_spawn_sub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr turtle1_cmd_pub_;
    rclcpp::Client<my_interfaces_pkg::srv::CatchTurtle>::SharedPtr client_kill_;
    rclcpp::TimerBase::SharedPtr timer_;

    // Variables de contrôle
    turtlesim::msg::Pose pose_; //position de la main tortue
    bool pose_received_ = false;

    my_interfaces_pkg::msg::Turtle turtle_target_; //tortue cible
    my_interfaces_pkg::msg::Turtle closest_target_;

    bool closest_turtle_first_; //pour activer le mode "chercher la plus proche"
    
    double distance;
    double min_distance;
    float target_angle;
    float angle_diff;
    float dist_x;
    float dist_y;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    
    auto node = std::make_shared<turtle_controller>();
    rclcpp::spin(node);
    
    rclcpp::shutdown();
    return 0;
}