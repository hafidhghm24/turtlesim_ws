#include "rclcpp/rclcpp.hpp"
#include "turtlesim/srv/spawn.hpp"
#include <random>
#include <string>

using namespace std::chrono_literals;
using namespace std::placeholders;

/*
    @brief spawn turtles on the window, and to manage which turtle is still “alive” (on the screen).
*/
class turtle_spawner : public rclcpp::Node {
public:
    // CORRECTION : Le membre s'appelle turtle_name_prefix_ (avec l'underscore à la fin)
    turtle_spawner() : Node("alive_node"), turtle_name_prefix_("turtle"), turtle_compteur(0) {
        turtle_client_ = this->create_client<turtlesim::srv::Spawn>("/spawn");
        RCLCPP_WARN(this->get_logger(), "Creation du client turtle_spawner!");

        timer_ = this->create_wall_timer(5s, std::bind(&turtle_spawner::random_arguments_spawn, this));
    }
    
private:
    void random_arguments_spawn() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0, 11.0);
        std::uniform_real_distribution<float> angle_dist(0.0, 6.28); 
        
        
        std::string turtle_name = this->turtle_name_prefix_ + std::to_string(this->turtle_compteur);
        
        float x = dist(gen);
        float y = dist(gen);
        float theta = angle_dist(gen);
        
        spowner_call(x, y, theta, turtle_name);
        this->turtle_compteur++;
    }

    void spowner_call(float x, float y, float theta, std::string turtle_name) {
        if(!turtle_client_->wait_for_service(1s)) {
            RCLCPP_WARN(this->get_logger(), "attente du service /spawn...");
            return;
        }

        auto request = std::make_shared<turtlesim::srv::Spawn::Request>();
        

        request->name = turtle_name;
        request->x = x;
        request->y = y;
        request->theta = theta;

        turtle_client_->async_send_request(request, std::bind(&turtle_spawner::spowner_callback, this, _1));
    }

    void spowner_callback(rclcpp::Client<turtlesim::srv::Spawn>::SharedFuture future) {
        auto response = future.get();
        RCLCPP_INFO(this->get_logger(), "nom: %s", response->name.c_str());
    }

    rclcpp::Client<turtlesim::srv::Spawn>::SharedPtr turtle_client_;
    rclcpp::TimerBase::SharedPtr timer_;

    std::string turtle_name_prefix_;
    int turtle_compteur;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<turtle_spawner>();

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}