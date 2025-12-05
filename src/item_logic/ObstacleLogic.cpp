#include "ObstacleLogic.h"

ObstacleLogic::ObstacleLogic(ItemData& item_) : PhysicsBody(item_) {
    data->type = ItemData::ItemType::Obstacle;
}

ObstacleLogic::~ObstacleLogic() = default;

void ObstacleLogic::on_collide_with(PhysicsBody& other) {
    // Scoped lock if needed
    // std::scoped_lock lock(data_mutex, other.get_mutex());
    // Add collision behavior here
}
