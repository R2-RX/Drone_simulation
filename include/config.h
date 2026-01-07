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
#define BLACKBOARD_SHM_SEM "/blackboard_shm_sem"
#define SYNC_SEM "/syncSemaphore"

// Number of processes
#define NUM_PROCESSES 7

// Data pipe line
#define KEYBOARD_Data_PIPE  "/tmp/keyboard_data_pipe"

// Number of Game components
#define MAX_ITEMS 2048  //Blackboard
#define MAX_LOGIC_OBJECTS MAX_ITEMS // Same limit as items

// Should check the  data type of socket input and see wheather its in meter or in pixel
#define Scaleing_pixel_to_meter 0.3125

#define UPS  30         // Update per-second (dt = 1/UPS)

// watch dog Pipe lines 
#define GLOBALTIMER_PIPE_WD     "/tmp/globaltimer_pipe_wd"
#define GAMELOOP_PIPE_WD        "/tmp/gameloop_pipe_wd"
#define MASTER_PIPE_WD          "/tmp/master_pipe_wd"
#define NETWORKGATE_PIPE_WD    "/tmp/master_pipe_net"
#define KEYBOARD_PIPE_WD        "/tmp/keyboard_pipe_wd"
#define ITEMSPAWNER_PIPE_WD     "/tmp/itemspawner_pipe_wd"

//#define MAX_PIPE_RETRIES 100  // (MAX_RETRIES*10ms)/1000 sec for opening pipes]
#define WATCHDOG_TIMEOUT_SECONDS 2 // seconds before declaring timeout

#define SPAWN_TIME_INTERVAL 20 // 20 seconds

// Object Coefficents (T and O)
#define  ATTRACTION_COEFFICIENT 20
#define  REPULSIVE_COEFFICIENT 50

// File
#define LIVE_MONITORING  "live_monitoring.txt"
#define SYSTEM_WIDE_LOG  "system_wide.log"
//#define Save_GAME_DATA  "game_data.dat"

// Tuning hyperparameters
#define DRONE_MASS 10
#define DRONE_ASSIST_FORCE (DRONE_MASS * 3) // Artificial force to stabilize the drone in simulation; set to 0 for real physics
#define DRONE_FRICTION_FACTOR (DRONE_MASS * 2)  // Simulation-only friction; use measured value for real drone


// Network
#define NETWORK_PORT "5050"


#include <cmath>  // for M_PI

// Define rotation angles in radians
#define ALPHA_0           0.0
#define ALPHA_PI_2        (M_PI / 2.0)     // +90°
#define ALPHA_MINUS_PI_2  (-M_PI / 2.0)    // -90°
#define ALPHA_PI          M_PI             // 180°
#define ALPHA_MINUS_PI    (-M_PI)          // -180° (same as 180°)

// Macro to convert radians to degrees
#define RAD_TO_DEG(rad)   ((rad) * 180.0 / M_PI)

#define Genetic_ALPHA ALPHA_0 //Choosing a “current” rotation angle ( sin() and cos() in C++ take radians)

// Example:
//  Genetic_Alpha  ALPHA_0         // 90° in radians
//  Genetic_Alpha  RAD_TO_DEG(ALPHA_0); // 90° in degrees

#endif // CONFIG_H