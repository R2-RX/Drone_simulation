#include "BlackBoard.h"

BlackBoard::BlackBoard(const std::string& shm_name)
    : shm_data(shm_name)
{
    logicPool.fill(nullptr);
    logicGenerations.fill(0);

    for (int i = 0; i < MAX_LOGIC_OBJECTS; ++i)
        freeLogicSlots.push_back(i);
}

BlackBoard::~BlackBoard() {
    for (int i = 0; i < MAX_LOGIC_OBJECTS; ++i)
        delete logicPool[i];

    shm_data.clean_up();
}

// -------- Logic Management --------
void BlackBoard::removeLogicObject(ItemLogic* obj) {
    if (!obj) throw std::invalid_argument("Null pointer passed to removeLogicObject");

    int index = obj->bb_index;

    if (index < 0 || index >= MAX_LOGIC_OBJECTS)
        throw std::out_of_range("Index out of range in removeLogicObject");

    if (logicGenerations[index] != obj->bb_generation)
        throw std::runtime_error("Stale pointer passed to removeLogicObject");

    initialPositions.erase(obj);

    delete obj;
    logicPool[index] = nullptr;

    logicGenerations[index]++;
    freeLogicSlots.push_back(index);
}

ItemLogic* BlackBoard::getLogicObject(int index) {
    if (index < 0 || index >= MAX_LOGIC_OBJECTS)
        return nullptr;
    return logicPool[index];
}

std::vector<ItemLogic*> BlackBoard::getAllLogicObjects() {
    std::vector<ItemLogic*> active;
    for (ItemLogic* ptr : logicPool)
        if (ptr)
            active.push_back(ptr);
    return active;
}

// -------- Items --------
int BlackBoard::addItem_protected(ItemData& item) {
    int slot = -1;

    shm_data.with_lock([&](BlackBoardShared* data) {
        for (int i = 0; i < MAX_ITEMS; ++i) {
            if (!data->items[i].active) {
                data->items[i] = item;
                data->items[i].active = true;
                slot = i;
                break;
            }
        }
    });

    if (slot == -1)
        throw std::runtime_error("No free slot in BlackBoard items");

    return slot;
}

ItemData* BlackBoard::getItem(int index) {
    if (index < 0 || index >= MAX_ITEMS) return nullptr;

    ItemData* ptr = nullptr;
    shm_data.with_lock([&](BlackBoardShared* data) {
        ptr = &data->items[index];
    });
    return ptr;
}

void BlackBoard::removeItem(int index) {
    if (index < 0 || index >= MAX_ITEMS) return;

    shm_data.with_lock([&](BlackBoardShared* data) {
        data->items[index].active = false;
    });
}

// -------- Simulation State --------

void BlackBoard::rescalePositions(int current_width_, int current_height_, int reference_width_, int reference_height_) {
    double scaleX = static_cast<double>(current_width_) / reference_width_;
    double scaleY = static_cast<double>(current_height_) / reference_height_;

    std::vector<ItemLogic*> objects = this->getAllLogicObjects();

    for (auto* obj : objects) {
        auto* phys_obj = dynamic_cast<PhysicsBody*>(obj);
        if (!phys_obj) continue;

        // If this object doesn't have an initial position stored yet, save its current position (for new objects)
        if (initialPositions.find(phys_obj) == initialPositions.end()) {
            initialPositions[phys_obj] = phys_obj->getPosition();
        }

        // Scaling
        auto [initX, initY] = initialPositions[phys_obj];
        phys_obj->setPosition(initX * scaleX, initY * scaleY);
    }
}

void BlackBoard::updateDroneStats(double dt) {
    shm_data.with_lock([&](BlackBoardShared* data) {
        for (int i = 0; i < MAX_ITEMS; ++i) {
            ItemData& item = data->items[i];
            if (!item.active) continue;
            if (item.type != ItemData::ItemType::Drone) continue;

            // Update timestamp
            item.timeStamp += dt;

            item.score = roundTo(
              item.number_of_hit_targets * 30.0
            - item.number_of_hit_obstacles * 5.0
            - item.timeStamp * 0.001
            - item.distance_traveled * 0.1,
            1);
        }
    });
}


void BlackBoard::setSpawnRequestsNum(int obstacles, int targets) {
    shm_data.with_lock([&](BlackBoardShared* data) {
        data->obstacle_spawn_request_num = obstacles;
        data->target_spawn_request_num = targets;
    });
}

std::pair<int,int> BlackBoard::getSpawnRequestsNum() {
    std::pair<int,int> out;
    shm_data.with_lock([&](BlackBoardShared* data) {
        out = { data->obstacle_spawn_request_num,
                data->target_spawn_request_num };
    });
    return out;
}

void BlackBoard::setGlobalTime(int64_t time) {
    shm_data.with_lock([&](BlackBoardShared* data) {
        data->simulationGlobalTime = time;
    });
}

int64_t BlackBoard::getGlobalTime() {
    int64_t t = 0;
    shm_data.with_lock([&](BlackBoardShared* data) {
        t = data->simulationGlobalTime;
    });
    return t;
}

void BlackBoard::setPlayAreaSize(int width, int height) {
    shm_data.with_lock([&](BlackBoardShared* data) {
        data->Width_play_area = width;
        data->Height_play_area = height;
    });
}

std::pair<int,int> BlackBoard::getPlayAreaSize() {
    std::pair<int,int> size;
    shm_data.with_lock([&](BlackBoardShared* data) {
        size = { data->Width_play_area, data->Height_play_area };
    });
    return size;
}

bool BlackBoard::getSpawnPermission() {
    bool val = false;
    shm_data.with_lock([&](BlackBoardShared* data) {
        val = data->Spawn_permission;
    });
    return val;
}

void BlackBoard::setSpawnPermission(bool permission) {
    shm_data.with_lock([&](BlackBoardShared* data) {
        data->Spawn_permission = permission;
    });
}

Pair_ BlackBoard::getKeyboardEvent() {
    Pair_ out;
    shm_data.with_lock([&](BlackBoardShared* data) {
        out = data->Force_input;
    });
    return out;
}

void BlackBoard::setKeyboardEvent(const Pair_& input_) {
    shm_data.with_lock([&](BlackBoardShared* data) {
        data->Force_input = input_;
    });
}
