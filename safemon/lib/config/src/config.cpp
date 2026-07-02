#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>

static std::set<uint32_t> parse_ids(const std::string& val) {
    std::set<uint32_t> ids;
    std::istringstream ss(val);
    std::string token;
    while (std::getline(ss, token, ',')) {
        try {
            // trim whitespace
            size_t start = token.find_first_not_of(" \t");
            size_t end   = token.find_last_not_of(" \t");
            if (start == std::string::npos) continue;
            token = token.substr(start, end - start + 1);
            ids.insert(std::stoul(token, nullptr, 16));
        } catch (...) {
            std::cerr << "[config] Invalid ID: " << token << "\n";
        }
    }
    return ids;
}

SafemonConfig load_config(const std::string& path) {
    SafemonConfig cfg;  // defaults from struct

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "[config] " << path
                  << " not found, using defaults\n";
        return cfg;
    }

    std::string line;
    while (std::getline(file, line)) {
        // skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        // trim whitespace from key and val
        auto trim = [](std::string& s) {
            size_t start = s.find_first_not_of(" \t");
            size_t end   = s.find_last_not_of(" \t\r\n");
            s = (start == std::string::npos) ? "" :
                s.substr(start, end - start + 1);
        };
        trim(key);
        trim(val);

        if      (key == "drm_device")      cfg.drm_device      = val;
        else if (key == "redis_host")      cfg.redis_host      = val;
        else if (key == "redis_port")      cfg.redis_port      = std::stoi(val);
        else if (key == "known_ids")       cfg.known_ids       = parse_ids(val);
        else if (key == "timeout_seconds") cfg.timeout_seconds = std::stoi(val);
        else if (key == "keyboard_device") cfg.keyboard_device = val;
        else if (key == "mouse_device")    cfg.mouse_device    = val;
        else if (key == "input_backend") {
            if (val == "wayland") cfg.input_backend = InputBackend::Wayland;
            else                  cfg.input_backend = InputBackend::Evdev;
        }
        else std::cerr << "[config] Unknown key: " << key << "\n";
    }

    std::cout << "[config] Loaded from " << path << "\n";
    return cfg;
}