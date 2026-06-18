#pragma once
#include <vector>
#include <string>

namespace Safemon {

    struct Bucket {
        float score;
    };

    struct Day {
        std::string date;
        std::string label;
        std::vector<Bucket> buckets;
    };

    struct WaterfallData {
        int buckets_per_day = 0;
        int days            = 0;
        std::vector<Day> day_list;

        bool Load(const std::string& path);
    };

} // namespace Safemon