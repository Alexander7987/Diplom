#include "Server.h"
#include "Parser.h"

int main() {
    SetConsoleOutputCP(1251);
    try {
        ini_parser par("data.ini");
        pqxx::connection bd("host=" + par.get_value<std::string>("bd.host_bd")
            + " port=" + std::to_string(par.get_value<int>("bd.port_bd"))
            + " dbname=" + par.get_value<std::string>("bd.name_bd")
            + " user=" + par.get_value<std::string>("bd.user_bd")
            + " password=" + par.get_value<std::string>("bd.password_bd"));
        int server_port = par.get_value<int>("server.port");
        Server server(bd, server_port);
        server.Start_Server();
    }
    catch (const std::string e) {
        std::cout << e << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
