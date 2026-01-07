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
static std::atomic<bool>     shutdownFlag{false};
static std::atomic<long>     spawn_counter{0};
static std::atomic<int64_t>  last_spawn_time{0};

// ------------Global singletons----------------
Logger logger(SYSTEM_WIDE_LOG);
BlackBoard blackboard;

// ------------Heartbeat thread for Spawner----------------
void start_spawner_heartbeat(Pipe<char>& pipe, std::atomic<int64_t>& last_spawn_time, Logger& logger) {
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
                    pipe.send_data('S');
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
                    if (shutdownFlag.load(std::memory_order_relaxed)) {
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
}


// ------------Signal handler----------------
void handle_sigterm(int)
{
    shutdownFlag.store(true, std::memory_order_relaxed);
}

int main()
{
    signal(SIGTERM, handle_sigterm); // from watchdog
    signal(SIGINT, handle_sigterm); // Ctrl+C

    blackboard.setProcessPid(WatchDogProcName::ItemSpawner_Proc, getpid());

    // ----------------------------
    // Logic object 
    // ----------------------------
    std::vector<ItemLogic*> drone_logics;
    std::vector<ItemLogic*> target_logics;
    std::vector<ItemLogic*> obstacle_logics;

    bool drones_initialized = false;

    Menu::MainChoice mode = blackboard.getGameMode();

    // ----------------------------
    // Main spawn loop
    // ----------------------------
    if (mode == Menu::MainChoice::STANDALONE) {

        logger.log("Spawner running in STANDALONE mode.",
            blackboard.getProcessPid(WatchDogProcName::ItemSpawner_Proc),
            Logger::LogLevel::INFO);

        // ------------Create heartbeat pipe----------------
        Pipe<char> itemspawner_pipe_wd(ITEMSPAWNER_PIPE_WD);
        // ----------*****Heartbeat thread (detached)*****------------
        start_spawner_heartbeat(itemspawner_pipe_wd, last_spawn_time, logger);

        while (true)
        {
            if (blackboard.getSpawnStatus())
            {
                blackboard.waitForPermission();

                auto [width, height] = blackboard.getPlayAreaSize();
                int screen_min = std::min(width, height);

                int number_of_obstacles = screen_min / 2;
                int number_of_targets   = screen_min / 2;
                int number_of_drones    = 1;

                blackboard.setSpawnRequestsNum(number_of_obstacles, number_of_targets);

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
                        data.Pos_y = static_cast<double>(height) - 1;
                        data.mass = DRONE_MASS;
                        data.visc_damp_coef = DRONE_FRICTION_FACTOR;
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

            if (shutdownFlag.load(std::memory_order_relaxed))
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

    } else if (mode == Menu::MainChoice::NETWORK ) {
        logger.log("Spawner running in NETWORK mode.",
            blackboard.getProcessPid(WatchDogProcName::ItemSpawner_Proc),
            Logger::LogLevel::INFO);

            if (blackboard.getSpawnStatus())
            {
                blackboard.waitForPermission();

                auto [width, height] = blackboard.getPlayAreaSize();
                int screen_min = std::min(width, height);

                int number_of_obstacles = 1;//getConnectedClientsNum();
                int number_of_targets   = 0;
                int number_of_drones    = 1;

                blackboard.setSpawnRequestsNum(number_of_obstacles, number_of_targets);

                // ----------------------------
                // Spawn drones (once)
                // ----------------------------
                if (!drones_initialized)
                {
                    for (int d = 0; d < number_of_drones; ++d)
                    {
                        ItemData data{};
                        data.type = ItemData::ItemType::Drone;
                        data.Pos_x = static_cast<double>(width - 1);
                        data.Pos_y = 0.1;
                        data.mass = DRONE_MASS;
                        data.visc_damp_coef = DRONE_FRICTION_FACTOR;
                        data.active = true;

                        blackboard.addItem_protected(data);
                    }
                    drones_initialized = true;
                }

                // ----------------------------
                // Spawn obstacles
                // ----------------------------
                for (int i = 0; i < number_of_obstacles; ++i)
                {
                    ItemData data{};
                    data.type = ItemData::ItemType::Obstacle;
                    data.Pos_x = 0.1;
                    data.Pos_y = static_cast<double>(height - 1);
                    data.mass = DRONE_MASS;
                    data.visc_damp_coef = DRONE_FRICTION_FACTOR;
                    data.active = true;

                    blackboard.addItem_protected(data);
                }
                logger.log("Items spawned! ", blackboard.getProcessPid(WatchDogProcName::ItemSpawner_Proc), Logger::LogLevel::INFO);
                blackboard.setSpawnStatus(false);
            }

            logger.log("Spawner exited!", blackboard.getProcessPid(WatchDogProcName::ItemSpawner_Proc), Logger::LogLevel::INFO);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            } else {
                logger.log("Spawner exiting immediately as no spawn permission!", blackboard.getProcessPid(WatchDogProcName::ItemSpawner_Proc), Logger::LogLevel::INFO);
            }
            
    return 0;
}
