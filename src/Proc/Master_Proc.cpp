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
#include "NetworkSocket.h"
#include "Logger.h"

Logger logger(SYSTEM_WIDE_LOG);

static volatile bool shutdownFlag = false;

static std::string ip, port;
 
// ===================== SIGNAL =====================

void handle_sigterm(int signum) {
    shutdownFlag = true;
}

void handle_sigwinch(int signum) {
    (void)signum;
    // DO NOTHING, just to prevent termination on SIGWINCH
}

void create_file(const std::string& name);
void first_general_cleanUp();
void last_general_cleanUp();

// ===================== PRINT CENTERED ===================== 
void print_centered(WINDOW* win, int row, const char* str, attr_t attr);
void draw_server_waiting_screen(WINDOW* win, int height, const std::string &server_ip = "xxx.xxx.xxx.xxx",
                                const std::string &server_port = "xxxxx");

//=================== IP and PORT input boxs (CLIENT) ==========================
bool input_two_boxes(WINDOW* parent, std::string &input1, std::string &input2,
                     int start_row, int start_col,
                     const std::string &prompt1, const std::string &prompt2,
                     const std::string &label1, const std::string &label2);

// ===================== MENU ===================== 
MenuResult run_menu(Ncurses_Win& win);

// ===================== PROCESS =====================
pid_t launch_process(const char* name, const std::vector<const char*>& args, WatchDogProcName proc, BlackBoard& bb);

// ===================== WATCHDOG ===================== 
void start_watchdog_heartbeat(Pipe<char>& pipe);

// ===================== STANDALONE ===================== 
void run_standalone_mode(BlackBoard& bb);

// ===================== NETWORK =====================
void run_network_mode(BlackBoard& bb);

// ===================== CHILD SUPERVISION ===================== 
void supervise_children();

// ===================== MAIN ===================== 

int main() {

    signal(SIGTERM, handle_sigterm);
    signal(SIGINT, handle_sigterm); // Ctrl+C
    signal(SIGWINCH, handle_sigwinch);
    

    // Remove old resources
    first_general_cleanUp();

    create_file(LIVE_MONITORING);
    

    Ncurses_Win menu_win;
    // make new shared memory (BlackBoard)
    BlackBoard blackboard;

    // set master pid in shared memory
    blackboard.setProcessPid(WatchDogProcName::Master_Proc, getpid());

    // pipes
    Pipe<char> global_timer_pipe(GLOBALTIMER_PIPE_WD);
    Pipe<char> keyboard_pipe(KEYBOARD_PIPE_WD);
    Pipe<char> gameloop_pipe(GAMELOOP_PIPE_WD);
    Pipe<char> itemspawner_pipe(ITEMSPAWNER_PIPE_WD);
    Pipe<char> master_pipe(MASTER_PIPE_WD);
    Pipe<char> networkgate_pipe(NETWORKGATE_PIPE_WD);

    bool notCancelled = true;

    MenuResult result = run_menu(menu_win); 

    // get user choice and launch processes accordingly
    if (result.section == Menu::Section::MAIN_MENU) {
        Menu::MainChoice mainChoice = static_cast<Menu::MainChoice>(result.choice);
        blackboard.setGameMode(Menu::MainChoice::STANDALONE);
        switch (mainChoice) {
            case Menu::MainChoice::STANDALONE:
                menu_win.destroy(); // clean up ncurses (menu_win) and return to terminal 
                logger.log("Starting STANDALONE mode", getpid(), Logger::LogLevel::INFO);
                run_standalone_mode(blackboard); //   
                break;
            case Menu::MainChoice::EXIT_MAIN:
                menu_win.destroy(); // clean up ncurses (menu_win) and return to terminal   
                break;  // Exit app 
        }
    } else if (result.section == Menu::Section::NETWORK_MENU) { // NETWORK_MENU
        Menu::NetworkChoice NetworkChoice = static_cast<Menu::NetworkChoice>(result.choice);
        blackboard.setGameMode(Menu::MainChoice::NETWORK); // set game mode to network
        switch (NetworkChoice) {
            case Menu::NetworkChoice::SERVER: 
                blackboard.setNetworkSide(Menu::NetworkChoice::SERVER);
                ip = getWlanIp(logger); // fetch wlan IP to suggest to user 
                port = NETWORK_PORT; // default port
                draw_server_waiting_screen(menu_win.getWindow(), 3, ip, port);

                //std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // wait 3s

                timeout(-1); // blocking (default)
                int ch;
                while ((ch = getch()) != '\n' && ch != KEY_ENTER && ch != 27); // wait for Enter
                if (ch == 27){ // user cancelled input (ESC)
                    menu_win.destroy(); // clean up ncurses (menu_win) and return to terminal 
                    break;
                }

                menu_win.destroy(); // clean up ncurses (menu_win) and return to terminal 
                logger.log("Starting SERVER mode", getpid(), Logger::LogLevel::INFO);

                run_network_mode(blackboard);
                break;
            case Menu::NetworkChoice::CLIENT: 
                blackboard.setNetworkSide(Menu::NetworkChoice::CLIENT);
                notCancelled = input_two_boxes(menu_win.getWindow(), ip, port, 3, 5, "Type Server IP", "Type server Port", "IP Address:", "Port:");
                blackboard.setIP(ip);
                blackboard.setPort(port);
                if (!notCancelled) { // user cancelled input (ESC)
                    menu_win.destroy(); // clean up ncurses (menu_win) and return to terminal 
                    break;
                }
                menu_win.destroy();
                logger.log("Starting CLIENT mode", getpid(), Logger::LogLevel::INFO);
                logger.log("[SERVER] " + ip + ":" + port, getpid(), Logger::LogLevel::INFO);
                run_network_mode(blackboard);
                break;
        }
    }

    // start heartbeat to watchdog
    start_watchdog_heartbeat(master_pipe);

    supervise_children();

    last_general_cleanUp();
    logger.log("Master_Proc exit!", getpid(), Logger::LogLevel::INFO);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // for the logger to flush
    return EXIT_SUCCESS;
}


// ===================== UTILS ======================

void create_file(const std::string& name) {
    std::ofstream file(name, std::ios::app);
    if (!file.is_open())
        throw std::runtime_error("Failed to create file: " + name);
}

void first_general_cleanUp() {
    shm_unlink(SHM_NAME);
    sem_unlink(BLACKBOARD_SHM_SEM);
    sem_unlink(SYNC_SEM);

    unlink(KEYBOARD_Data_PIPE);
    unlink(GLOBALTIMER_PIPE_WD);
    unlink(GAMELOOP_PIPE_WD);
    unlink(MASTER_PIPE_WD);
    unlink(KEYBOARD_PIPE_WD);
    unlink(ITEMSPAWNER_PIPE_WD);
    unlink(NETWORKGATE_PIPE_WD);

    remove(LIVE_MONITORING);
    remove(SYSTEM_WIDE_LOG);
}

void last_general_cleanUp() {
    shm_unlink(SHM_NAME);
    sem_unlink(BLACKBOARD_SHM_SEM);
    sem_unlink(SYNC_SEM);

    unlink(KEYBOARD_Data_PIPE);
    unlink(GLOBALTIMER_PIPE_WD);
    unlink(GAMELOOP_PIPE_WD);
    unlink(MASTER_PIPE_WD);
    unlink(KEYBOARD_PIPE_WD);
    unlink(ITEMSPAWNER_PIPE_WD);
    unlink(NETWORKGATE_PIPE_WD);
}

// ============== PRINT CENTERED ==============

void print_centered(WINDOW* win, int row, const char* str, attr_t attr = COLOR_PAIR(2)) {
    int width = getmaxx(win);
    int col = (width - strlen(str)) / 2;
    wattron(win, attr);
    mvwprintw(win, row, col, "%s", str);
    wattroff(win, attr);
}

void draw_server_waiting_screen(WINDOW* win, int height, const std::string &server_ip,
                                const std::string &server_port) {
    werase(win);
    print_centered(win, height, "Press Enter when you are ready", COLOR_PAIR(2) | A_BLINK); // Waiting for a player to join. Press Enter to continue...
    std::string server_info = "Server IP: " + server_ip + " Port: " + server_port;
    print_centered(win, height + 3, server_info.c_str(), COLOR_PAIR(3));
    wrefresh(win); // update the screen
}
//=================== IP and PORT input boxs (CLIENT) ==========================

bool input_two_boxes(WINDOW* parent, std::string &input1, std::string &input2,
                     int start_row, int start_col,
                     const std::string &prompt1, const std::string &prompt2,
                     const std::string &label1, const std::string &label2) {
    int parent_h, parent_w;
    getmaxyx(parent, parent_h, parent_w);

    int box_h = 3;
    int box_w = std::min(parent_w / 3, 30);

    WINDOW* label_box1 = derwin(parent, 1, box_w, start_row - 1, start_col);
    WINDOW* box1 = derwin(parent, box_h, box_w, start_row, start_col);

    WINDOW* label_box2 = derwin(parent, 1, box_w, start_row + box_h + 1, start_col);
    WINDOW* box2 = derwin(parent, box_h, box_w, start_row + box_h + 2, start_col);

    keypad(box1, TRUE);
    keypad(box2, TRUE);

    int ch;
    int active_box = 1; // 1 = first box, 2 = second box
    int last_key = 0; // to track sequences

    werase(parent);      // clear content of the window
    wrefresh(parent);    // update the screen
    curs_set(1);   // show cursor

    while (true) {
        // Draw first label and box
        print_centered(parent, 0, "--------------- Running as client ---------------");
        print_centered(parent, parent_h - 1, "*Press TAB to switch between boxes*", COLOR_PAIR(7) | A_DIM);
        wrefresh(parent);    // update the screen
        werase(label_box1);
        mvwprintw(label_box1, 0, 0, "%s", label1.c_str());
        wrefresh(label_box1);

        werase(box1);
        box(box1, 0, 0);
        if(input1.empty()) {
            wattron(box1, A_DIM);
            mvwprintw(box1, 1, 1, "%-*s", box_w - 2, prompt1.c_str());
            wattroff(box1, A_DIM);
        } else {
            mvwprintw(box1, 1, 1, "%s", input1.c_str());
        }
        wrefresh(box1);

        // Draw second label and box
        werase(label_box2);
        mvwprintw(label_box2, 0, 0, "%s", label2.c_str());
        wrefresh(label_box2);

        werase(box2);
        box(box2, 0, 0);
        if(input2.empty()) {
            wattron(box2, A_DIM);
            mvwprintw(box2, 1, 1, "%-*s", box_w - 2, prompt2.c_str());
            wattroff(box2, A_DIM);
        } else {
            mvwprintw(box2, 1, 1, "%s", input2.c_str());
        }
        wrefresh(box2);

        // Move cursor to active box
        if(active_box == 1) {
            wmove(box1, 1, 1 + input1.length());
            wrefresh(box1);
        } else {
            wmove(box2, 1, 1 + input2.length());
            wrefresh(box2);
        }


        ch = wgetch(active_box == 1 ? box1 : box2);
        last_key = ch; // update last key

        if(ch == 10) { // Enter
            if(active_box == 1) active_box = 2;
            else if(!input1.empty() && !input2.empty() && input1.length() <= 15) break; // both inputs provided
        } else if(ch == '\t') { // Tab switches box
            active_box = (active_box == 1 ? 2 : 1);
        } else if(ch == 27) { // ESC to exit
            input1.clear();
            input2.clear();
            break;
        } else if(ch == KEY_BACKSPACE || ch == 127 || ch == 8) { // normal backspace
            if(active_box == 1 && !input1.empty()) input1.pop_back();
            else if(active_box == 2 && !input2.empty()) input2.pop_back();
        } else if((ch >= '0' && ch <= '9') || ch == '.') {
            // input length limits
            if(active_box == 1 && input1.length() < (size_t)(box_w - 2) && input1.length() < 15) 
                input1.push_back((char)ch); // e.g., IP address (xxx.xxx.xxx.xxx)
            else if(active_box == 2 && input2.length() < (size_t)(box_w - 2) && input2.length() < 5) 
                input2.push_back((char)ch); // e.g., port number (xxxxx)
        } else if(ch == KEY_RESIZE) {
            getmaxyx(parent, parent_h, parent_w);
        }
    }

    delwin(label_box1);
    delwin(box1);
    delwin(label_box2);
    delwin(box2);
    return (last_key == 27) ? false : true; // return false if ESC was pressed
}

// ===================== MENU ===================== 

MenuResult run_menu(Ncurses_Win& win) {
    Menu::Section section = Menu::Section::MAIN_MENU;
    int choice = 0;
    bool running = true;

    while (running) {
        int n = (section == Menu::Section::MAIN_MENU) ?
                sizeof(Menu::mainMenuLabels)/sizeof(Menu::mainMenuLabels[0]) :
                sizeof(Menu::networkMenuLabels)/sizeof(Menu::networkMenuLabels[0]);

        werase(win.getWindow());

        print_centered(win.getWindow(), 0, "--------------- Welcome to Drone Simulation ---------------");

        for (int i = 0; i < n; ++i) {
            if (i == choice) wattron(win.getWindow(), A_REVERSE);

            mvwprintw(
                win.getWindow(), i + 2, 1, "%s",
                (section == Menu::Section::MAIN_MENU) ? Menu::mainMenuLabels[i] : Menu::networkMenuLabels[i]
            );

            if (i == choice) wattroff(win.getWindow(), A_REVERSE);
        }

        wrefresh(win.getWindow());

        int ch = getch();
        switch (ch) {
            case KEY_UP:   choice--; break;
            case KEY_DOWN: choice++; break;
            case KEY_RESIZE: win.resize(); break;
            case 27: // ESC
                running = false;
                break;
            case 10: // ENTER
                if (section == Menu::Section::MAIN_MENU) {
                    if (choice == 0) return {section, static_cast<int>(Menu::MainChoice::STANDALONE)};
                    if (choice == 1) {section = Menu::Section::NETWORK_MENU; choice = 0;}
                    if (choice == 2) return {section, static_cast<int>(Menu::MainChoice::EXIT_MAIN)};
                } else if (section == Menu::Section::NETWORK_MENU) {
                    if (choice == 0) return {section, static_cast<int>(Menu::NetworkChoice::SERVER)};
                    if (choice == 1) return {section, static_cast<int>(Menu::NetworkChoice::CLIENT)};
                    if (choice == 2) {section = Menu::Section::MAIN_MENU; choice = 0;}
                }
                break;
        }

        if (choice < 0) choice = n - 1;
        if (choice >= n) choice = 0;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return {Menu::Section::MAIN_MENU, static_cast<int>(Menu::MainChoice::EXIT_MAIN)};
}

// ===================== PROCESS ===================== 

pid_t launch_process(const char* name, const std::vector<const char*>& args, WatchDogProcName proc, BlackBoard& bb) {
    pid_t pid = fork();

    if (pid == 0) {
        execvp(args[0], const_cast<char* const*>(args.data()));

        logger.log(
            "exec failed: " + std::string(name) + " errno=" + std::to_string(errno),
            getpid(),
            Logger::LogLevel::ERROR
        );

        _exit(EXIT_FAILURE);
    }

    // Parent
    bb.setProcessPid(proc, pid);
    logger.log(std::string(name) + " started", pid, Logger::LogLevel::INFO);

    return pid;
}

// ===================== WATCHDOG ===================== 
void handleShutdownRequest(Pipe<char>& pipe) {
    if (pipe.receive_data() == "Q") {
        logger.log("Received shutdown request from Network", getpid(),
                    Logger::LogLevel::WARNING); 
        shutdownFlag = true;
    }

    if (pipe.receive_data() == "E") {
        logger.log("Received Connection failure from Network request to shutdown", getpid(),
                    Logger::LogLevel::ERROR); 
        shutdownFlag = true;
    }
}

void start_watchdog_heartbeat(Pipe<char>& pipe) {
    std::thread([&]() {
        while (!shutdownFlag) {
            pipe.send_data('M');
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        logger.log("Received SIGTERM, shutting down...", getpid(),
                    Logger::LogLevel::WARNING);
    }).detach();
}

// ===================== STANDALONE ===================== 

void run_standalone_mode(BlackBoard& bb) {

    bb.setProcessPid(WatchDogProcName::Master_Proc, getpid());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("GlobalTimer_Proc", {"./GlobalTimer_Proc", nullptr}, WatchDogProcName::GlobalTimer_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("Keyboard_Proc", {"konsole", "--hold", "-e", "./KeyBoard_Proc", nullptr}, WatchDogProcName::Keyboard_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("GameLoop_Proc", {"konsole", "--hold", "-e", "./GameLoop_Proc", nullptr}, WatchDogProcName::GameLoop_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("ItemSpawner_Proc", {"./ItemSpawner_Proc", nullptr}, WatchDogProcName::ItemSpawner_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    launch_process("WatchDog_Proc", {"./WatchDog_Proc", nullptr}, WatchDogProcName::WatchDog_Proc, bb);

    logger.log("Master_Proc started all children",  bb.getProcessPid(WatchDogProcName::Master_Proc), Logger::LogLevel::INFO);

}

// ===================== NETWORK ===================== 

void run_network_mode(BlackBoard& bb) {

    bb.setProcessPid(WatchDogProcName::Master_Proc, getpid());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("GlobalTimer_Proc", {"./GlobalTimer_Proc", nullptr}, WatchDogProcName::GlobalTimer_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("Keyboard_Proc", {"konsole", "--hold", "-e", "./KeyBoard_Proc", nullptr}, WatchDogProcName::Keyboard_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("GameLoop_Proc", {"konsole", "--hold", "-e", "./GameLoop_Proc", nullptr}, WatchDogProcName::GameLoop_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_process("ItemSpawner_Proc", {"./ItemSpawner_Proc", nullptr}, WatchDogProcName::ItemSpawner_Proc, bb);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    launch_process("NetworkGate_Proc", {"./NetworkGate_Proc", nullptr}, WatchDogProcName::NetworkGate_Proc, bb); //"konsole", "--hold", "-e", 

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    launch_process("WatchDog_Proc", {"./WatchDog_Proc", nullptr}, WatchDogProcName::WatchDog_Proc, bb); // watchdog skips spawner in network mode

    logger.log("Master_Proc started all children",  bb.getProcessPid(WatchDogProcName::Master_Proc), Logger::LogLevel::INFO);

}

// ===================== CHILD SUPERVISION ===================== 

void supervise_children() {
    int status;
    while (wait(&status) > 0) {
        if (WIFEXITED(status))
            logger.log("Child exited: " + std::to_string(WEXITSTATUS(status)),
                        getpid(), Logger::LogLevel::INFO);
        else if (WIFSIGNALED(status))
            logger.log("Child killed by signal: " + std::to_string(WTERMSIG(status)),
                        getpid(), Logger::LogLevel::WARNING);
    }
}