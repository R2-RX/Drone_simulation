#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <unordered_map>

#include "SharedMemoryData.h"
#include "ItemLogic.h"
#include "PhysicsBody.h"

#define MAX_ITEMS 2048
#define MAX_LOGIC_OBJECTS 2048

//keyboard input
struct Pair_{double x ,y;};

// POD shared memory layout
struct BlackBoardShared {
    ItemData items[MAX_ITEMS];

    int obstacle_spawn_request_num = 0;
    int target_spawn_request_num = 0;
    int drone_spawn_request_num = 0;

    bool Spawn_permission = false;
    
    Pair_ Force_input = {0.0, 0.0};

    int64_t simulationGlobalTime = 0;

    int Width_play_area = 0;
    int Height_play_area = 0;

    std::vector<std::pair<double,double>> ref_positions;
};

class BlackBoard {
private:
    SharedMemoryData<BlackBoardShared> shm_data;

    // Logic Pool
    std::array<ItemLogic*, MAX_LOGIC_OBJECTS> logicPool{}; //pointer to ItemLogic which holds address of object data in shared memory
    std::array<uint32_t, MAX_LOGIC_OBJECTS> logicGenerations{}; //genetation stamps which is basically a counter to track versions of 
                                                               //logic objects and by tracking versions we mean if an object has been removed and a new object has taken its place in the same slot
    std::vector<int> freeLogicSlots; //this keeps track of free slots in logic pool

    std::unordered_map<ItemLogic*, std::pair<double,double>> initialPositions;

public:
    BlackBoard(const std::string& shm_name = SHM_NAME);
    ~BlackBoard();

     ItemData* current_target = nullptr;
     
    // -------- Logic Management --------
    template<typename LogicType>
    LogicType* addLogicObject(ItemData* data) {
        if (freeLogicSlots.empty())
            throw std::runtime_error("Logic pool full");

        int index = freeLogicSlots.back();
        freeLogicSlots.pop_back();

        LogicType* logic = new LogicType(*data);

        // Assign identity for safe removal
        logic->bb_index = index;
        logic->bb_generation = ++logicGenerations[index]; //  

        logicPool[index] = logic;
        return logic;  // Return derived class pointer directly
    }

    void removeLogicObject(ItemLogic* obj);

    ItemLogic* getLogicObject(int index);
    std::vector<ItemLogic*> getAllLogicObjects();

    // -------- Items --------
    int addItem_protected(ItemData& item);
    ItemData* getItem(int index);
    void removeItem(int index);

    // -------- Global Simulation State --------
    void rescalePositions(int current_width_, int current_height_, int reference_width_, int reference_height_);

    void updateDroneStats(double dt);

    void setSpawnRequestsNum(int obstacles, int targets);
    std::pair<int,int> getSpawnRequestsNum();

    void setGlobalTime(int64_t time);
    int64_t getGlobalTime();

    void setPlayAreaSize(int width, int height);
    std::pair<int,int> getPlayAreaSize();

    bool getSpawnPermission();
    void setSpawnPermission(bool permission);

    Pair_ getKeyboardEvent();
    void setKeyboardEvent(const Pair_& input_);

};

#endif // BLACKBOARD_H
