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

#define SHM_NAME "/blackboard_shm"

#define PIPE_BLACKBOARD "/tmp/blackboard_pipe"

#define KEYBOARD_Data_PIPE  "/tmp/keyboard_data_pipe"
#define PIPE_WINDOW     "/tmp/window_pipe"
#define PIPE_OBSTACLE   "/tmp/obstacle_pipe"
#define PIPE_TARGET     "/tmp/target_pipe"
#define PIPE_BLACKBOARD "/tmp/blackboard_pipe"
#define PIPE_DYNAMICS   "/tmp/dynamics_pipe"

// HYPERPARAMETERS

#define Scaleing_pixel_to_meter 0.3125
// you should check the  data type of socket input and see wheather its in meter or in pixel


#define RENDER_DELAY 100000 // microseconds
#define RETRY_DELAY 50000   // for opening pipes
#define MAX_RETRIES 20      // for opening pipes
#define TIMEOUT_SECONDS 10  // for watchdog


#define DT  0.001           // time step
#define MAX_OBJECTS 100     // max number of obstacles and targets
#define BLACKBOARD_CHECK_DELAY      1  // update blackboard every 1 second
#define OBSTACLE_GENERATION_DELAY   4  // generate obstacles every 4 seconds
#define TARGET_GENERATION_DELAY     6 // generate targets every 6 seconds
#define WATCHDOG_Timer_DELAY    2  // check heartbeat every 1 second

#define LIVE_MONITORING  "live_monitoring.txt"

// Debug settings
// #define ENABLE_LOGGING     1
// #define DEBUG_LEVEL        2   // 1: Low, 2: Medium, 3: High

// // Other global settings
// #define TIMEOUT_DURATION   5000  // Timeout in milliseconds

#endif // CONFIG_H