#include "Pauk.h"
#include "Parser.h"

int main() {
    SetConsoleOutputCP(1251);
    try {
        ini_parser par("data.ini");
        INI ini;
        pqxx::connection bd("host=" + par.get_value<std::string>("bd.host_bd")
            + " port=" + std::to_string(par.get_value<int>("bd.port_bd"))
            + " dbname=" + par.get_value<std::string>("bd.name_bd")
            + " user=" + par.get_value<std::string>("bd.user_bd")
            + " password=" + par.get_value<std::string>("bd.password_bd"));
        ini.start_sayt = par.get_value<std::string>("pauk.start_sayt");
        ini.path = par.get_value<std::string>("pauk.path");
        ini.port = std::to_string(par.get_value<int>("pauk.port"));
        ini.recursiya = par.get_value<int>("pauk.recursiya");
        Pauk pauk(bd, ini);
    }
    catch (const std::string& e) {
        std::cout << e << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}