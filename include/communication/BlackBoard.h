#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <chrono>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "BlackBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <semaphore.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

#include "config.h"
#include "SharedMemoryData.h"
#include "ItemLogic.h"
#include "PhysicsBody.h"
#include "Logger.h"

//The structs and the enums are shared definition-- not a shared memory structure
struct Menu {
    // Menu sections and choices
    enum class Section { MAIN_MENU, NETWORK_MENU };
    enum class MainChoice { STANDALONE, NETWORK, EXIT_MAIN };
    enum class NetworkChoice { SERVER, CLIENT, BACK };

    // Menu labels
    static inline const char* mainMenuLabels[] = { "standalone", "network", "Exit" };
    static inline const char* networkMenuLabels[] = { "server", "client", "back" };
};

struct MenuResult {
    Menu::Section section; // which section the result is from
    int choice;            // stores MainChoice or NetworkChoice as int
};

enum WatchDogProcName { 
    GlobalTimer_Proc,  
    Keyboard_Proc, 
    GameLoop_Proc, 
    ItemSpawner_Proc,
    Master_Proc,
    NetworkGate_Proc,
    WatchDog_Proc,
    WD_Count }; 

//keyboard input
struct Point{double x ,y;};

// POD shared memory layout
struct BlackBoardShared {
    ItemData items[MAX_ITEMS];
    pid_t process_pid[NUM_PROCESSES];

    int obstacle_spawn_request_num = 0;
    int target_spawn_request_num = 0;
    int drone_spawn_request_num = 0;

    int Game_mode = 0;
    int Network_side = 0; // server = 0 ,clinet = 1

    bool Spawn_Status = true;

    int64_t simulationGlobalTime = 0;

    int Width_play_area = 0;
    int Height_play_area = 0;

    char IP[64];
    char Port[16];

    double g_timeStamp;
};

// BlackBoard class managing shared memory and logic objects
class BlackBoard {
private:
    SharedMemoryData<BlackBoardShared> shm_data;

    sem_t* syncSemaphore = nullptr;

    // Logic Pool
    std::array<ItemLogic*, MAX_LOGIC_OBJECTS> logicPool{}; //pointer to ItemLogic which holds address of object data in shared memory
    std::array<uint32_t, MAX_LOGIC_OBJECTS> logicGenerations{}; //genetation stamps which is basically a counter to track versions of 
                                                               //logic objects and by tracking versions we mean if an object has been removed and a new object has taken its place in the same slot
    std::vector<int> freeLogicSlots; //this keeps track of free slots in logic pool

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
        logic->bb_generation = ++logicGenerations[index];  

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
    void updateDroneStats(double dt);

    void setSpawnRequestsNum(int obstacles, int targets);
    std::pair<int,int> getSpawnRequestsNum();

    void setGlobalTime(int64_t time);
    int64_t getGlobalTime();

    void setPlayAreaSize(int width, int height);
    std::pair<int,int> getPlayAreaSize();

    void waitForPermission();
    void signalPermission();

    pid_t getProcessPid(int index);
    void setProcessPid(int index, pid_t P);

    bool getSpawnStatus();
    void setSpawnStatus(bool permission);
    
    double getTimeStamp();

    void setGameMode(Menu::MainChoice choice);
    Menu::MainChoice getGameMode();

    void setNetworkSide(Menu::NetworkChoice choice);
    Menu::NetworkChoice getNetworkSide();

    void setIP(const std::string& ip);
    std::string getIP();

    void setPort(const std::string& port);
    std::string getPort();
};

#endif // BLACKBOARD_H
