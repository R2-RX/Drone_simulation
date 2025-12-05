#ifndef ITEMDATA_H
#define ITEMDATA_H

#include <cstdint>
#include <mutex>

struct ItemData {
    enum class ItemType { Drone, Obstacle, Target };

    ItemType type;

    double Pos_x{1.0}, Pos_y{1.0};
    double Force_x{0.0}, Force_y{0.0};
    double Vel_x{0.0}, Vel_y{0.0};
    double mass{10.0};
    double visc_damp_coef{1.0};
    double repl_coef{0.2};
    double attr_coef{0.1};

    double timeStamp{0.0};
    double distance_traveled{0.0};
    double score{0.0};
    uint64_t number_of_hit_targets{0};
    uint64_t number_of_hit_obstacles{0};
    bool active{false};
};

extern std::mutex item_data_mutex;  

// Rounding numbers
double roundTo(double value, int decimals);
void roundItemData(ItemData& data, int decimals = 2);

// Convert ItemType to string
const char* type_to_string(ItemData::ItemType type);

// Print all components of ItemData
void print_item_info(const ItemData& item);

#endif
