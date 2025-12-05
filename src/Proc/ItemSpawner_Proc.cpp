#include "BlackBoard.h"
#include "DroneLogic.h"
#include "TargetLogic.h"
#include "ObstacleLogic.h"

#include <cstdlib>
#include <vector>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

int main() {
    BlackBoard blackboard;

    auto [width, height] = blackboard.getPlayAreaSize();
    blackboard.setSpawnPermission(true);

    // ----------------------------
    // Spawn drones
    // ----------------------------
    const int number_of_drones = 1;
    std::vector<ItemLogic*> drone_logics;
    for (int d = 0; d < number_of_drones; ++d) {
        ItemData drone_data;
        drone_data.type = ItemData::ItemType::Drone;
        drone_data.Pos_x = 0; //width / 2.0;
        drone_data.Pos_y = 0; //height / 2.0;
        drone_data.mass = 10.0;
        drone_data.visc_damp_coef = 1;
        drone_data.active = true;

        int item_index = blackboard.addItem_protected(drone_data);
        ItemData* shared_data_ptr = blackboard.getItem(item_index);

        ItemLogic* drone_logic = blackboard.addLogicObject<DroneLogic>(shared_data_ptr);
        drone_logics.push_back(drone_logic);
    }

    // ----------------------------
    // Main loop
    // ----------------------------
    while (true) {
        if (blackboard.getSpawnPermission()) {
            // Seed RNG with global time
            std::srand(static_cast<unsigned int>(blackboard.getGlobalTime()));

            auto [width, height] = blackboard.getPlayAreaSize();
            int screen_min = std::min(width, height);

            int number_of_obstacles = screen_min / 2;
            int number_of_targets = screen_min / 2;
            blackboard.setSpawnRequestsNum(number_of_obstacles, number_of_targets);

            // ----------------------------
            // Spawn targets
            // ----------------------------
            std::vector<ItemLogic*> target_logics;
            for (int i = 0; i < number_of_targets; ++i) {
                ItemData target_data;
                target_data.type = ItemData::ItemType::Target;
                target_data.Pos_x = static_cast<double>(std::rand() % width);
                target_data.Pos_y = static_cast<double>(std::rand() % height);
                target_data.mass = 500.0;
                target_data.visc_damp_coef = 1000;
                target_data.attr_coef = 2.0;
                target_data.active = true;

                int item_index = blackboard.addItem_protected(target_data);
                ItemData* shared_data_ptr = blackboard.getItem(item_index);

                ItemLogic* target_logic = blackboard.addLogicObject<TargetLogic>(shared_data_ptr);
                target_logics.push_back(target_logic);

            }

            // ----------------------------
            // Spawn obstacles
            // ----------------------------
            std::vector<ItemLogic*> obstacle_logics;
            for (int j = 0; j < number_of_obstacles; ++j) {
                ItemData obstacle_data;
                obstacle_data.type = ItemData::ItemType::Obstacle;
                obstacle_data.Pos_x = static_cast<double>(std::rand() % width);
                obstacle_data.Pos_y = static_cast<double>(std::rand() % height);
                obstacle_data.mass = 500.0;
                obstacle_data.visc_damp_coef = 1000;
                obstacle_data.repl_coef = 3.0;
                obstacle_data.active = true;

                int item_index = blackboard.addItem_protected(obstacle_data);
                ItemData* shared_data_ptr = blackboard.getItem(item_index);

                ItemLogic* obstacle_logic = blackboard.addLogicObject<ObstacleLogic>(shared_data_ptr);
                obstacle_logics.push_back(obstacle_logic);
            }

            // Reset spawn permission
            blackboard.setSpawnPermission(false);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
}

