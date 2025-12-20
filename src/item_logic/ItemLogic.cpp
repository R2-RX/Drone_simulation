#include "ItemLogic.h"
#include <mutex>
#include "PhysicsBody.h"

ItemLogic::ItemLogic(ItemData& item_) : data(&item_) {}

void ItemLogic::reset() {
    data->Pos_x = data->Pos_y = 1.0;
    data->Force_x = data->Force_y = 0.0;
    data->Vel_x = data->Vel_y = 0.0;
    data->timeStamp = 0.0;
    data->distance_traveled = 0.0;
    data->score = 0.0;
    data->number_of_hit_targets = 0;
    data->number_of_hit_obstacles = 0;
    data->active = true;
}
