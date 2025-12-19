// include/config.h

#ifndef CONFIG_H
#define CONFIG_H

// Compiler-specific settings
#if defined(_WIN32)
    #define PLATFORM_WINDOWS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
#endif

// Shared memory
#define SHM_NAME "/blackboard_shm"

// Semaphore
#define SPAWN_SEM_NAME "/spawn_semaphore"

// Number of processes
#define NUM_PROCESSES 6

// Data pipe line
#define KEYBOARD_Data_PIPE  "/tmp/keyboard_data_pipe"

// Number of Game components
#define MAX_ITEMS 2048  //Blackboard
#define MAX_LOGIC_OBJECTS MAX_ITEMS // Same limit as items

// Should check the  data type of socket input and see wheather its in meter or in pixel
#define Scaleing_pixel_to_meter 0.3125

#define UPS  30         // Update per-second (dt = 1/UPS)
#define TARGET_OBSTACLE_GENERATION_DELAY  6 // generate every 6 seconds

// watch dog Pipe lines 
#define GLOBALTIMER_PIPE_WD     "/tmp/globaltimer_pipe_wd"
#define GAMELOOP_PIPE_WD        "/tmp/gameloop_pipe_wd"
#define MASTER_PIPE_WD          "/tmp/master_pipe_wd"
#define KEYBOARD_PIPE_WD        "/tmp/keyboard_pipe_wd"
#define ITEMSPAWNER_PIPE_WD     "/tmp/itemspawner_pipe_wd"

#define WATCHDOG_Timer_DELAY   50  // 2000 ms between heartbeat checks // just check the spawner counter if you want to be accurate
#define MAX_PIPE_RETRIES 100  // (MAX_RETRIES*10ms)/1000 sec for opening pipes
#define WATCHDOG_TIMEOUT_SECONDS 3// 5 seconds before declaring timeout

#define SPAWN_TIME_INTERVAL 20 // 20 seconds

// Object Coefficents (T and O)
#define  ATTRACTION_COEFFICIENT 20
#define  REPULSIVE_COEFFICIENT 30

// File
#define LIVE_MONITORING  "live_monitoring.txt"
#define SYSTEM_WIDE_LOG  "system_wide.log"
//#define Save_GAME_DATA  "game_data.dat"

#endif // CONFIG_H