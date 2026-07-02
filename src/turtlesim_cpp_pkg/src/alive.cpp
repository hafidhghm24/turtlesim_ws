#include "rclcpp/rclcpp.hpp"
#include "turtlesim/srv/spawn.hpp"
#include "turtlesim/srv/kill.hpp"

#include "my_interfaces_pkg/msg/turtle.hpp"
#include "my_interfaces_pkg/msg/turtle_liste.hpp"
#include "my_interfaces_pkg/srv/catch_turtle.hpp"


#include <random>
#include <string>
#include <vector> 

using namespace std::chrono_literals;
using namespace std::placeholders;

class turtle_spawner : public rclcpp::Node {
public:
    turtle_spawner() : Node("alive_node"), turtle_compteur(0) {

        //PARAMETERS
        this->declare_parameter("spawn_frequency", 1.0);
        spawn_frequency_ = this->get_parameter("spawn_frequency").as_double();
        this->declare_parameter("turtle_name", "turtle");
        turtle_name_prefix_ = this->get_parameter("turtle_name").as_string();

        //client qui va appeler le service /spawn pour faire aparaitre une tortue
        turtle_client_ = this->create_client<turtlesim::srv::Spawn>("/spawn");
        RCLCPP_WARN(this->get_logger(), "Creation du client turtle_spawner => /spawn");

        //toute les 2sec apelle de la fonction random_arguments_spawn
        timer_ = this->create_wall_timer(std::chrono::milliseconds((int)((1.0/spawn_frequency_)*1000.0)), std::bind(&turtle_spawner::random_arguments_spawn, this));
        RCLCPP_INFO(this->get_logger(), "frequence de spawn = %d", (int)((1.0/spawn_frequency_)*1000.0));

        //pour envoie de la liste des tortue
        turtle_pub_ = this->create_publisher<my_interfaces_pkg::msg::TurtleListe>("alive_turtles", 10);
        RCLCPP_WARN(this->get_logger(), "Creation du publisher sur turtle_bub => /alive_turteles");

        //reception de la demande client sur /catch_turtle pour tué la tortue cible
        kill_srv_ = this->create_service<my_interfaces_pkg::srv::CatchTurtle>("catch_turtle", std::bind(&turtle_spawner::kill_callback, this, _1, _2));
        RCLCPP_WARN(this->get_logger(), "creation du service kill_srv => /catch_turtle");
        
        //apelle du service /kill pour supprimer la tortue
        kill_client_ = this->create_client<turtlesim::srv::Kill>("/kill");
        RCLCPP_WARN(this->get_logger(), "creation du client kill_client => /kill ");
    }
    
private:

    //----CREATION DUNE TORTUE CHAQUE 2sec----
    /**
     * @brief generer aleatoirement les coordonée de la tortue qui va spawn ensuite apelle de la fonction spowner_call()
     */
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

    /**
     * @brief envoie la request avec les coordoné de la tortue au service /spawn pour creer une tortue
     * lorceque le service es demande on apelle le spowner_callback()
     */
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

        turtle_client_->async_send_request(
            request, 
            [this, x, y, theta](rclcpp::Client<turtlesim::srv::Spawn>::SharedFuture future) {
                this->spowner_callback(future, x, y, theta);
            }
        );
    }

    /**
     * @brief apres la demande /spawn on va remplir la TurtleListe avec la new_turtle et appeler la fonction alive_liste_publisher()
     */
    void spowner_callback(rclcpp::Client<turtlesim::srv::Spawn>::SharedFuture future, float x, float y, float theta) {
        auto response = future.get();
        if(response->name != ""){
            RCLCPP_INFO(this->get_logger(), "nom: %s", response->name.c_str()); 
            
            my_interfaces_pkg::msg::Turtle new_turtle;
            new_turtle.name = response->name;
            new_turtle.x = x;
            new_turtle.y = y;
            new_turtle.theta = theta;
            
            this->turtles_alive_liste_.push_back(new_turtle);
            this->alive_liste_publisher();
        }
    }

    /**
     * @brief publier sur /alive_turteles la liste (un msg de type TurtleListe)
     */
    void alive_liste_publisher(){
        my_interfaces_pkg::msg::TurtleListe msg;
        msg.turtles = turtles_alive_liste_;
        this->turtle_pub_->publish(msg);
    }


        
    //----SUPPRIMER LA TORTUE CIBLE ATTEINTE----

    /**
     * @brief confirmation success=true et apelle call_kill_srv()
     */
    void kill_callback(const std::shared_ptr<my_interfaces_pkg::srv::CatchTurtle::Request> request, 
                       std::shared_ptr<my_interfaces_pkg::srv::CatchTurtle::Response> response) {
        
        this->call_kill_srv(request->name);
        response->success = true; 
    }


    /**
     * @brief attente de reception de la demande sur /catch_turtele et apelle du service /kill avec kill_client et ensuite apelle de callback_kill_srv()
     */
    void call_kill_srv(const std::string& turtle_name){
        while(!kill_client_->wait_for_service(1s)){
            RCLCPP_WARN(this->get_logger(), "attente du service /kill");
        }

        auto request = std::make_shared<turtlesim::srv::Kill::Request>();
        request->name = turtle_name;
        
        
        this->kill_client_->async_send_request(
            request,
            [this, turtle_name](rclcpp::Client<turtlesim::srv::Kill>::SharedFuture future) {
                this->callback_kill_srv(future, turtle_name);
            }
        );
    }

    /**
     * @brief apres lenvoie de la requete sur /kill cette fonction va mettre a jour la liste et appeler alive_liste_publisher() pour lenvoyer
     */
    void callback_kill_srv(rclcpp::Client<turtlesim::srv::Kill>::SharedFuture future, const std::string& turtle_name){
        
        auto response = future.get();
        //si la cible atteinte (turtle_name) es presente dans la liste on va la retirer
        for(size_t i = 0; i < this->turtles_alive_liste_.size(); i++){
            if (this->turtles_alive_liste_[i].name == turtle_name){
                this->turtles_alive_liste_.erase(this->turtles_alive_liste_.begin() + i);

                //envoie de la nouvelle liste
                this->alive_liste_publisher();
                break;
            }
        }
    }


    //----DECLARATION DES VARIABLE INTERNE----
    rclcpp::Client<turtlesim::srv::Spawn>::SharedPtr turtle_client_;
    rclcpp::Publisher<my_interfaces_pkg::msg::TurtleListe>::SharedPtr turtle_pub_;
    rclcpp::Service<my_interfaces_pkg::srv::CatchTurtle>::SharedPtr kill_srv_;
    rclcpp::Client<turtlesim::srv::Kill>::SharedPtr kill_client_;
    rclcpp::TimerBase::SharedPtr timer_;
    
    std::vector<my_interfaces_pkg::msg::Turtle> turtles_alive_liste_;

    std::string turtle_name_prefix_;
    int turtle_compteur;
    double spawn_frequency_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<turtle_spawner>();

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}