#include "rclcpp/rclcpp.hpp"
#include "turtlesim/srv/spawn.hpp"
/*
    @brief spawn turtles on the window, and to manage which turtle is still “alive” (on the screen).
    TODO: apelle du service /spawn pour creer les tortues 
    TODO: apelle du service /kill pour retirer les tortues
    TODO: publisher sur /alive_turteles de la liste des tortues en vie 
    TODO: Handle a service server to “catch” a turtle, which means to call the /kill service and remove the turtle from the array of alive turtles.
*/
class turtle_spawner : public rclcpp::Node{
public:
    turtle_spawner() : Node("alive_node"){
        
    }
    
private:

};

int main(int argc, char **argv){
    rclcpp::init(argc, argv);
    auto node = std::make_shared<turtle_spawner>();

    rclcpp::shutdown();
    return 0;
}