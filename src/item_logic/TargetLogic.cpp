#include "TargetLogic.h"

TargetLogic::TargetLogic(ItemData& item_) : PhysicsBody(item_) {
    data->type = ItemData::ItemType::Target;
}

TargetLogic::~TargetLogic() = default;

void TargetLogic::on_collide_with(PhysicsBody& other) {
    // Scoped lock if needed
    // std::scoped_lock lock(data_mutex, other.get_mutex());
    // Add collision behavior here
}
