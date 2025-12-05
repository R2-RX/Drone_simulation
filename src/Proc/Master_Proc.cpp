#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "Ncurses_Win.h"
#include "Keyboard.h"
#include "config.h"
#include "Pipe.h"
#include "DroneLogic.h"
#include "TargetLogic.h"
#include "ObstacleLogic.h"
#include "PhysicsBody.h"
#include "BlackBoard.h"
    
int main () {

    shm_unlink(SHM_NAME); // ignore errors if it doesn't exist
    sleep (0.5);
    pid_t pid0 = fork();
    if (pid0 == 0) {
        execlp("./GlobalTimer_Proc","./GlobalTimer_Proc", nullptr);
        perror("execlp GlobalTimer_Proc failed");
        exit(EXIT_FAILURE);
    }
    sleep (0.5);
    pid_t pid1 = fork();
    if (pid1 == 0) {
        execlp("konsole", "konsole", "--hold", "-e", "./KeyBoard_Proc", nullptr);
        perror("execlp KeyBoard_Proc failed");
        exit(EXIT_FAILURE);
    }
    sleep(0.5);
    pid_t pid2 = fork();
    if (pid2 == 0) {
        execlp("konsole", "konsole", "--hold", "-e", "./GameLoop_Proc", nullptr);
        perror("execlp Rendering_Proc failed");
        exit(EXIT_FAILURE);
    }
    sleep(0.5);
    pid_t pid3 = fork();
    if (pid3 == 0) {
        execlp("./ItemSpawner_Proc","./ItemSpawner_Proc", nullptr);
        perror("execlp ItemSpawner_Proc failed");
        exit(EXIT_FAILURE);
    }
    sleep(0.5);
    // pid_t pid4 = fork();
    // if (pid4 == 0) {
    //     execlp("konsole", "konsole", "--hold", "-e", "bash", "-c", "watch -n 0.1 'tail -n 20 test.txt'", nullptr);
    //     perror("execlp live_Log failed");
    //     exit(EXIT_FAILURE);
    // }

    // Parent waits for all children
    int status;
    while (wait(&status) > 0);

    return 0;
}
