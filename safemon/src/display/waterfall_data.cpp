#include "waterfall_data.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace Safemon {

    bool WaterfallData::Load(const std::string& path)
    {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "[WaterfallData] Cannot open: " << path << "\n";
            return false;
        }

        try {
            nlohmann::json root = nlohmann::json::parse(f);

            buckets_per_day = root["meta"]["buckets_per_day"];
            days            = root["meta"]["days"];

            for (auto& day_node : root["days"]) {
                Day day;
                day.date  = day_node["date"];
                day.label = day_node["label"];

                for (auto& bucket_node : day_node["buckets"]) {
                    Bucket b;
                    b.score = bucket_node["score"];
                    day.buckets.push_back(b);
                }

                day_list.push_back(day);
            }

            std::cout << "[WaterfallData] Loaded " << days
                      << " days, " << buckets_per_day
                      << " buckets/day\n";
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[WaterfallData] Parse error: " << e.what() << "\n";
            return false;
        }
    }

} // namespace Safemon