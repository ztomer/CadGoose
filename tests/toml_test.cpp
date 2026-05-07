#include <iostream>
#include <fstream>
#include <toml.hpp>

int main() {
    toml::value config = toml::table{
        {"debug", toml::table{
            {"toTerminal", true},
            {"visuals", false}
        }},
        {"general", toml::table{
            {"globalScale", 1.5f},
            {"audioEnabled", true}
        }},
        {"color", toml::table{
            {"goose", toml::table{
                {"r", 0.82f},
                {"g", 0.82f},
                {"b", 0.82f}
            }}
        }}
    };

    std::ofstream ofs("output_test2.toml");
    ofs << config << "\n";
    ofs.close();

    std::cout << "Generated TOML:\n";
    std::ifstream ifs("output_test2.toml");
    std::cout << ifs.rdbuf();

    auto read_back = toml::parse("output_test2.toml");
    std::cout << "\nRead back: globalScale = " << toml::get<float>(read_back.at("general").at("globalScale")) << "\n";

    return 0;
}
