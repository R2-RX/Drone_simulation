#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "Ncurses_Win.h"
#include "Keyboard.h"
#include "config.h"
#include "Pipe.h"
#include "DroneLogic.h"
#include "TargetLogic.h"
#include "ObstacleLogic.h"
#include "PhysicsBody.h"
#include "BlackBoard.h"
#include "Logger.h" 

Logger logger(SYSTEM_WIDE_LOG);

static volatile bool shutdownFlage = false;

void create_file(std::string name);
void general_cleanUp();
void handle_sigterm(int signum) {shutdownFlage = true;}

int main() {
    signal(SIGTERM, handle_sigterm); // from watchdog

    // Remove old resources
    general_cleanUp();

    create_file(LIVE_MONITORING);

    // makeing new shm (BlackBoard)
    BlackBoard blackboard;
    // set master pid in shm
    blackboard.setProcessPid(WatchDogProcName::Master_Proc, getpid());

    // pipes
    Pipe<char> global_timer_pipe(GLOBALTIMER_PIPE_WD);
    Pipe<char> keyboard_pipe(KEYBOARD_PIPE_WD);
    Pipe<char> gameloop_pipe(GAMELOOP_PIPE_WD);
    Pipe<char> itemspawner_pipe(ITEMSPAWNER_PIPE_WD);
    Pipe<char> master_pipe(MASTER_PIPE_WD);

    // Fork all child processes
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pid_t pid = fork();
    if (pid == 0) {
        execlp("./GlobalTimer_Proc", "./GlobalTimer_Proc", nullptr);
        logger.log("execlp GlobalTimer_Proc failed"+ std::to_string(errno), blackboard.getProcessPid(WatchDogProcName::GlobalTimer_Proc) ,Logger::LogLevel::ERROR);
        _exit(EXIT_FAILURE);
    } 
    else if (pid > 0) {
        blackboard.setProcessPid(WatchDogProcName::GlobalTimer_Proc, pid);
        logger.log("GlobalTimer_Proc successfully Started ...", blackboard.getProcessPid(WatchDogProcName::GlobalTimer_Proc), Logger::LogLevel::INFO);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pid = fork();
    if (pid == 0) {
        execlp("konsole", "konsole", "--hold", "-e", "./KeyBoard_Proc", nullptr);
        logger.log("execlp Keyboard_Proc failed"+ std::to_string(errno), blackboard.getProcessPid(WatchDogProcName::Keyboard_Proc) ,Logger::LogLevel::ERROR);
        _exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        blackboard.setProcessPid(WatchDogProcName::Keyboard_Proc, pid);
        logger.log("Keyboard_Proc successfully Started ...", blackboard.getProcessPid(WatchDogProcName::Keyboard_Proc) ,Logger::LogLevel::INFO);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pid = fork();
    if (pid == 0) {
        execlp("konsole", "konsole", "--hold", "-e", "./GameLoop_Proc", nullptr);
        logger.log("execlp GameLoop_Proc failed"+ std::to_string(errno), blackboard.getProcessPid(WatchDogProcName::GameLoop_Proc) ,Logger::LogLevel::ERROR);
        _exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        blackboard.setProcessPid(WatchDogProcName::GameLoop_Proc, pid);
        logger.log("GameLoop_Proc successfully Started ...", blackboard.getProcessPid(WatchDogProcName::GameLoop_Proc) ,Logger::LogLevel::INFO);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pid = fork();
    if (pid == 0) {
        execlp("./ItemSpawner_Proc", "./ItemSpawner_Proc", nullptr);
        logger.log("execlp ItemSpawner_Proc failed"+ std::to_string(errno), blackboard.getProcessPid(WatchDogProcName::ItemSpawner_Proc) ,Logger::LogLevel::ERROR);
        _exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        blackboard.setProcessPid(WatchDogProcName::ItemSpawner_Proc, pid);
        logger.log("ItemSpawner_Proc successfully Started ...", blackboard.getProcessPid(WatchDogProcName::ItemSpawner_Proc) ,Logger::LogLevel::INFO);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    pid = fork();
    if (pid == 0) {
        execlp("./WatchDog_Proc", "./WatchDog_Proc", nullptr);
        logger.log("execlp WatchDog_Proc failed"+ std::to_string(errno), blackboard.getProcessPid(WatchDogProcName::WatchDog_Proc) ,Logger::LogLevel::ERROR);
        _exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        blackboard.setProcessPid(WatchDogProcName::WatchDog_Proc, pid);
        logger.log("WatchDog_Proc successfully Started ...", blackboard.getProcessPid(WatchDogProcName::WatchDog_Proc) ,Logger::LogLevel::INFO);
    }

    // --------------------------------------------------------------------------------- 
    // ---------------------------------------------------------------------------------
    logger.log("Master_Proc started all children", blackboard.getProcessPid(WatchDogProcName::Master_Proc), Logger::LogLevel::INFO);

    int64_t now =  blackboard.getGlobalTime();

    // ------------------------------
    // ------ send a msg to watch dog -----
    // ------------------------------
    std::thread([&](){
        while (true) {
            master_pipe.send_data('M'); // heartbeat
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // send frequently enough
            if (shutdownFlage) 
                break; // watchdog will react
        }
        logger.log("Received SIGTERM, shutting down...", getpid(),Logger::LogLevel::WARNING);
    }).detach();

    // Wait for children to exit
    int status;
    while (wait(&status) > 0) {

        if (WIFEXITED(status))
            logger.log("Child exited normally: " + std::to_string(WEXITSTATUS(status)), getpid(), Logger::LogLevel::INFO);
        else if (WIFSIGNALED(status))
            logger.log("Child terminated by signal: " + std::to_string(WTERMSIG(status)), getpid(), Logger::LogLevel::WARNING);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    logger.log("Master_Proc exit!", getpid(), Logger::LogLevel::INFO);
    return EXIT_SUCCESS;
}

void create_file(std::string name) {
     // make a new file
    std::ofstream file(name, std::ios::app); 
    if (!file.is_open())
    {
       throw std::runtime_error(std::string("failed to open LIVE_MONITORING file") + std::strerror(errno)); 
    }
}

void general_cleanUp() {
    shm_unlink(SHM_NAME);   
    sem_unlink(SPAWN_SEM_NAME);  
    unlink(KEYBOARD_Data_PIPE);
    unlink(GLOBALTIMER_PIPE_WD);
    unlink(GAMELOOP_PIPE_WD);
    unlink(MASTER_PIPE_WD);
    unlink(KEYBOARD_PIPE_WD);
    unlink(ITEMSPAWNER_PIPE_WD);
    remove(LIVE_MONITORING);
    remove(SYSTEM_WIDE_LOG);
}
