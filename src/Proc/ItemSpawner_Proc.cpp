#include "BlackBoard.h"
#include "DroneLogic.h"
#include "TargetLogic.h"
#include "ObstacleLogic.h"
#include "Logger.h"
#include "Pipe.h"
#include "config.h"

#include <cstdlib>
#include <vector>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <iostream>
#include <signal.h>

// -----------Global shared state (atomic)---------------
static std::atomic<bool>     shutdownFlage{false};
static std::atomic<long>     spawn_counter{0};
static std::atomic<int64_t>  last_spawn_time{0};

// ------------Global singletons----------------
Logger logger(SYSTEM_WIDE_LOG);
BlackBoard blackboard;


// ------------Signal handler----------------
void handle_sigterm(int)
{
    shutdownFlage.store(true, std::memory_order_relaxed);
}

int main()
{
    signal(SIGTERM, handle_sigterm); // from watchdog

    blackboard.setProcessPid(
        WatchDogProcName::ItemSpawner_Proc,
        getpid()
    );

    // ------------Create heartbeat pipe----------------
    Pipe<char> itemspawner_pipe_wd(ITEMSPAWNER_PIPE_WD);

    
    // ----------*********88Heartbeat thread (detached)*************------------
    std::thread heartbeat(
        [&]()
        {
            last_spawn_time.store(
                blackboard.getGlobalTime(),
                std::memory_order_relaxed
            );
            const int64_t SPAWN_TIMEOUT_NS = static_cast<int64_t>(SPAWN_TIME_INTERVAL +1) * 1'000'000'000LL; // give the thread two times to try (SPAWN_TIME_INTERVAL +1)
            while (true)
            {
                // Send alive signal
                itemspawner_pipe_wd.send_data('S');
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                int64_t now  = blackboard.getGlobalTime();
                int64_t last = last_spawn_time.load(std::memory_order_relaxed);
                int64_t diff = now - last;

                if (diff > static_cast<int64_t>(SPAWN_TIMEOUT_NS))
                {
                    logger.log(
                        "Spawner not responding! its Heartbeat stopped.",
                        getpid(),
                        Logger::LogLevel::WARNING
                    );
                    break; // watchdog will react
                }
                if (shutdownFlage.load(std::memory_order_relaxed)) {
                    break; // watchdog will react
                }
            }

            logger.log(
                "Spawner Heartbeat thread exiting.",
                getpid(),
                Logger::LogLevel::INFO
            );
        }
    );

    heartbeat.detach();

    // ----------------------------
    // Logic object 
    // ----------------------------
    std::vector<ItemLogic*> drone_logics;
    std::vector<ItemLogic*> target_logics;
    std::vector<ItemLogic*> obstacle_logics;

    bool drones_initialized = false;

    // ----------------------------
    // Main spawn loop
    // ----------------------------
    while (true)
    {
        if (blackboard.getSpawnStatus())
        {
            blackboard.waitSpawnPermission();

            auto [width, height] = blackboard.getPlayAreaSize();
            int screen_min = std::min(width, height);

            int number_of_obstacles = screen_min / 2;
            int number_of_targets   = screen_min / 2;
            int number_of_drones    = 1;

            blackboard.setSpawnRequestsNum(
                number_of_obstacles,
                number_of_targets
            );

            // ----------------------------
            // Spawn drones (once)
            // ----------------------------
            if (!drones_initialized)
            {
                for (int d = 0; d < number_of_drones; ++d)
                {
                    ItemData data{};
                    data.type = ItemData::ItemType::Drone;
                    data.Pos_x = 0.1;
                    data.Pos_y = 0.1;
                    data.mass = 10.0;
                    data.visc_damp_coef = 1;
                    data.active = true;

                    blackboard.addItem_protected(data);
                }
                drones_initialized = true;
            }

            // ----------------------------
            // Spawn targets
            // ----------------------------
            std::srand(
                static_cast<unsigned int>(blackboard.getGlobalTime())
            );

            for (int i = 0; i < number_of_targets; ++i)
            {
                ItemData data{};
                data.type = ItemData::ItemType::Target;
                data.Pos_x = std::rand() % width;
                data.Pos_y = std::rand() % height;
                data.mass = 500.0;
                data.visc_damp_coef = 1000;
                data.attr_coef = ATTRACTION_COEFFICIENT;
                data.active = true;

                blackboard.addItem_protected(data);
            }

            // ----------------------------
            // Spawn obstacles
            // ----------------------------
            for (int i = 0; i < number_of_obstacles; ++i)
            {
                ItemData data{};
                data.type = ItemData::ItemType::Obstacle;
                data.Pos_x = std::rand() % width;
                data.Pos_y = std::rand() % height;
                data.mass = 500.0;
                data.visc_damp_coef = 1000;
                data.repl_coef = REPULSIVE_COEFFICIENT;
                data.active = true;

                blackboard.addItem_protected(data);
            }

            logger.log(
                std::to_string(
                    spawn_counter.fetch_add(1, std::memory_order_relaxed)
                ) + ": Items spawned!",
                blackboard.getProcessPid(
                    WatchDogProcName::ItemSpawner_Proc
                ),
                Logger::LogLevel::INFO
            );

            // ----------------------------
            // Update heartbeat timestamp
            // ----------------------------
            last_spawn_time.store(
                blackboard.getGlobalTime(),
                std::memory_order_relaxed
            );

            blackboard.setSpawnStatus(false);
        }

        if (shutdownFlage.load(std::memory_order_relaxed))
        {
            logger.log(
                "Received SIGTERM, shutting down...",
                getpid(),
                Logger::LogLevel::WARNING
            );
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
