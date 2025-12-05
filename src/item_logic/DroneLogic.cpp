#include "DroneLogic.h"
#include <cmath>   // for std::pow, std::round

// Constructor
DroneLogic::DroneLogic(ItemData& item_) : PhysicsBody(item_) {
    data->type = ItemData::ItemType::Drone;
}

// Update the drone's state each timestep
void DroneLogic::update(double dt) {
    std::lock_guard<std::mutex> lock(data_mutex);

    // Physical interactions: forces, velocity, position
    physical_interaction(dt);

}

// Collision handling
void DroneLogic::on_collide_with(PhysicsBody& other) { 
    // Scoped lock to avoid deadlocks
    std::mutex& m1 = data_mutex;
    std::mutex& m2 = other.get_mutex();
    std::scoped_lock lock(m1, m2);

    if (&other == this) return; // Ignore self-collision

    double dx = other.getPosition().first - this->getPosition().first;
    double dy = other.getPosition().second - this->getPosition().second;

    if (std::sqrt(dx * dx + dy * dy) < 1) { // Simple collision threshold

        ItemData* otherData = other.get_data_ptr();
        if (!otherData) return;

        switch (otherData->type) {
            case ItemData::ItemType::Target:
                data->number_of_hit_targets++;
                other.deactivate();  // deactivate the target
                break;

            case ItemData::ItemType::Obstacle:
                data->number_of_hit_obstacles++;
                other.deactivate();  // deactivate obstacle
                break;

            default:
                break;
        }
    }
}