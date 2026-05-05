#include "desktop_parser.h"
#include <iostream>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

std::string parseAppName(const fs::path& appdir) {
    for (const auto& entry : fs::directory_iterator(appdir)) {
        if (entry.path().extension() == ".desktop") {
            std::ifstream f(entry.path());
            std::string line;
            while (std::getline(f, line)) {
                if (line.find("Name=") == 0 && line.find("Name[") != 0) {
                    return line.substr(5);
                }
            }
        }
    }
    return "Application";
}
