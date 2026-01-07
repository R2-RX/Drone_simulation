#include "NetworkSocket.h"
#include "BlackBoard.h"
#include "DroneLogic.h"
#include "TargetLogic.h"
#include "ObstacleLogic.h"
#include "config.h"
#include "Logger.h"
#include "Pipe.h"

Logger logger(SYSTEM_WIDE_LOG);
BlackBoard blackboard;
Pipe<char> networkgate_pipe(NETWORKGATE_PIPE_WD);

// ------------------ RAII Socket ------------------

class SocketRAII {
    int sockfd;
public:
    explicit SocketRAII(int fd = -1) : sockfd(fd) {}
    ~SocketRAII() { if (sockfd >= 0) close(sockfd); }
    int fd() const { return sockfd; }
    void reset(int fd = -1) { if (sockfd >= 0) close(sockfd); sockfd = fd; }
};

//--------------------------------------------------

static volatile bool shutdownFlag = false;
static SocketRAII g_server_fd{-1};  // listening socket
static SocketRAII g_client_fd{-1};  // accepted client

// ------------------ Utilities ------------------

void sendShutdownSignal(Pipe<char>& pipe) {
        pipe.send_data('Q');
        logger.log("Sent local shutdown signal", 
                   getpid(), Logger::LogLevel::INFO);
       // std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

Point parseCoordinates(const std::string& msg) {
    std::string s = msg;
    // Remove "size" prefix if it exists
    if (s.find("size") == 0) {
        s = s.substr(4);
        // Trim leading spaces
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    }

    // Replace comma with space to unify formats
    std::replace(s.begin(), s.end(), ',', ' ');

    // Now split by space
    std::istringstream iss(s);
    double x = 0, y = 0;
    if (!(iss >> x >> y)) {
        logger.log("Invalid coordinate format: " + msg, getpid(), Logger::LogLevel::ERROR);
        shutdownFlag = true;
    }

    return { x, y };
}

void sendString(int sockfd, const std::string& msg) {
    std::string framed = msg + "\n";  // append newline as delimiter
    size_t totalSent = 0;

    while (totalSent < framed.size()) {
        int n = write(sockfd, framed.c_str() + totalSent, framed.size() - totalSent);
        if (n < 0) {
            logger.log("Failed to write to socket: " + msg, getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            return;
        }
        totalSent += n;
    }

    logger.log("Sent: " + msg, getpid(), Logger::LogLevel::INFO);
}


std::string recvString(int sockfd) {
    static std::string leftover;
    char buffer[256];

    while (!shutdownFlag) {
        // Check for a complete line
        size_t pos = leftover.find('\n');
        if (pos != std::string::npos) {
            std::string msg = leftover.substr(0, pos);
            leftover.erase(0, pos + 1);

            logger.log("Received: " + msg, getpid(), Logger::LogLevel::INFO);
            return msg;
        }

        ssize_t n = read(sockfd, buffer, sizeof(buffer));
        if (n == 0) {
            logger.log("Peer disconnected", getpid(), Logger::LogLevel::INFO);
            shutdownFlag = true;
            leftover.clear();
            return "";
        }

        if (n < 0) {
            logger.log("Failed to read from socket", getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            leftover.clear();
            return "";
        }

        leftover.append(buffer, n);

        // safety limit to avoid unbounded growth
        if (leftover.size() > 8192) {
            logger.log("Incoming line too long", getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            leftover.clear();
            return "";
        }
    }

    return "";  // fallback return, should never actually be used
}


// ------------------ Server Socket ------------------
int bindServerSocket(const std::string& ip = "", const std::string& startPort = NETWORK_PORT, int maxTries = 10) {
    std::string bindIp = ip;

    // If no IP provided, try to auto-detect WLAN IP
    if (bindIp.empty()) {
        bindIp = getWlanIp(logger); // uses the improved getWlanIp
        if (bindIp.empty()) {
            logger.log("No WLAN IP found, falling back to INADDR_ANY", getpid(), Logger::LogLevel::INFO);
        }
    }

    int sockfd = -1;
    int port = 0;

    try {
        port = std::stoi(startPort);
    } catch (...) {
        logger.log("Invalid NETWORK_PORT: " + startPort, getpid(), Logger::LogLevel::ERROR);
        shutdownFlag = true;
        return -1;
    }

    for (int i = 0; i < maxTries; ++i, ++port) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            logger.log("Failed to open server socket", getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            return -1;
        }

        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (bindIp.empty()) {
            serv_addr.sin_addr.s_addr = INADDR_ANY;
        } else {
            if (inet_pton(AF_INET, bindIp.c_str(), &serv_addr.sin_addr) <= 0) {
                logger.log("Invalid IP address: " + bindIp, getpid(), Logger::LogLevel::ERROR);
                close(sockfd);
                shutdownFlag = true;
                return -1;
            }
        }

        if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            logger.log("Server socket bound successfully on " + (bindIp.empty() ? "0.0.0.0" : bindIp) + ":" + std::to_string(port),
                       getpid(), Logger::LogLevel::INFO);
            return sockfd; // success
        }

        logger.log("Port " + std::to_string(port) + " busy, trying next port...", getpid(), Logger::LogLevel::INFO);
        close(sockfd); // close failed socket
    }

    logger.log("Failed to bind server socket after " + std::to_string(maxTries) + " attempts", getpid(), Logger::LogLevel::ERROR);
    shutdownFlag = true;
    return -1;
}


void startListening(int sockfd, int backlog = 5) {
    if (listen(sockfd, backlog) < 0) {
        logger.log("Listen failed on socket", getpid(), Logger::LogLevel::ERROR);
        close(sockfd);
        shutdownFlag = true;
        return;
    }
    logger.log("Server is listening...", getpid(), Logger::LogLevel::INFO);
}

// ------------------ Game Helpers ------------------

Point flipHorizontal(Point p, Point playArea) { 
    return { 2 * playArea.x - p.x, p.y };
}

Point flipVertical(Point p, Point playArea) {
    return { p.x, 2 * playArea.y - p.y };
}

Point scalePointToWindow(const Point& p, const Point& referenceSize, const Point& currentSize) {
    if (referenceSize.x == 0 || referenceSize.y == 0) {
        logger.log("Reference play area size cannot be zero", getpid(), Logger::LogLevel::ERROR);
        return p; // fallback, no scaling
    }

    double scaleX = currentSize.x / referenceSize.x;
    double scaleY = currentSize.y / referenceSize.y;

    Point scaled{
        p.x * scaleX,
        p.y * scaleY
    };

    logger.log(
        "Scaling point (" + std::to_string(p.x) + "," + std::to_string(p.y) + ") "
        "from reference size (" + std::to_string(referenceSize.x) + "," + std::to_string(referenceSize.y) + ") "
        "to current size (" + std::to_string(currentSize.x) + "," + std::to_string(currentSize.y) + ") "
        "=> scaled point (" + std::to_string(scaled.x) + "," + std::to_string(scaled.y) + ")",
        getpid(), Logger::LogLevel::INFO
    );

    return scaled;
}

// ------------------ Server Handshake ------------------

void serverHandshake(int clientfd) {
    sendString(clientfd, "ok");
    std::string ack = recvString(clientfd);
    if (ack != "ook") {
        logger.log("Client handshake failed: " + ack, getpid(), Logger::LogLevel::ERROR);
        shutdownFlag = true;
        return;
    }
    logger.log("Client handshake successful", getpid(), Logger::LogLevel::INFO);
}

// -----------------------------------------------------

void sendWindowDimensions(int socketfd, int width, int height) {
    sendString(socketfd, "size " + std::to_string(width) + "," + std::to_string(height));
    std::string ack = recvString(socketfd);
    if (ack != "sok") {
        logger.log("Window dimension acknowledgment failed", getpid(), Logger::LogLevel::ERROR);
        shutdownFlag = true;
        return;
    }
}

Point receiveWindowDimensions(int socketfd) {
    Point W_H = parseCoordinates(recvString(socketfd));
    sendString(socketfd, "sok"); // acknowledgment
    return W_H;
}

void createLogicForNewItems() {
    // Snapshot of current logic objects
    auto existing = blackboard.getAllLogicObjects();

    // Build a set of wrapped ItemData* pointers
    std::unordered_set<ItemData*> wrapped;
    wrapped.reserve(existing.size());
    for (auto* obj : existing) {
        if (!obj) continue;
        ItemData* d = obj->getItemData();
        if (d) wrapped.insert(d);
    }

    // For each shared memory item, if active and not wrapped, create logic
    for (int i = 0; i < MAX_ITEMS; ++i) {
        ItemData* item = blackboard.getItem(i);
        if (!item || !item->active) continue;
        if (wrapped.find(item) != wrapped.end()) continue; // already wrapped

        ItemLogic* logic = nullptr;
        switch (item->type) {
            case ItemData::ItemType::Drone:
                logic = blackboard.addLogicObject<DroneLogic>(item);
                break;
            case ItemData::ItemType::Target:
                logic = blackboard.addLogicObject<TargetLogic>(item);
                break;
            case ItemData::ItemType::Obstacle:
                logic = blackboard.addLogicObject<ObstacleLogic>(item);
                break;
            default:
                break;
        }

        // update wrapped
        if (logic) {
            ItemData* d = logic->getItemData();
            if (d) wrapped.insert(d);
        }
    }
}

void sendServerDrone(int socketfd) {
    std::pair<double,double> myPos{0,0};

    createLogicForNewItems();

    std::vector<ItemLogic*> objects = blackboard.getAllLogicObjects();
    for (auto* obj : objects) {
        PhysicsBody* phys_obj = dynamic_cast<PhysicsBody*>(obj);
        ItemData* data = phys_obj->getItemData();
        if (data->type == ItemData::ItemType::Drone) {
            myPos = phys_obj->getPosition();
        }
    }

    Point P = flipHorizontal({myPos.first, myPos.second},
                             {(double)blackboard.getPlayAreaSize().first, (double)blackboard.getPlayAreaSize().second});
    P = flipHorizontal(P,{(double)blackboard.getPlayAreaSize().first, (double)blackboard.getPlayAreaSize().second});

    std::string msg = std::to_string(P.x) + "," + std::to_string(P.y);
    sendString(socketfd, msg);

    std::string ack = recvString(socketfd);
    if (ack != "dok") {
        logger.log("Drone position acknowledgment failed: " + ack, getpid(), Logger::LogLevel::ERROR);
        shutdownFlag = true;
        return;
    }
}

void receiveServerDrone(int socketfd, Point WinRefSize) {
    Point server_drone_pos = parseCoordinates(recvString(socketfd));
    // Acknowledge receipt
    sendString(socketfd, "dok");

    Point localWinSize{(double)blackboard.getPlayAreaSize().first,(double)blackboard.getPlayAreaSize().second};
    createLogicForNewItems();
    
    // Flip and scale properly
    //server_drone_pos = flipHorizontal(server_drone_pos, localWinSize);
    //server_drone_pos = scalePointToWindow(server_drone_pos, WinRefSize, localWinSize);

    // Update obstacles
    std::vector<ItemLogic*> objects = blackboard.getAllLogicObjects();
    for (auto* obj : objects) {
        PhysicsBody* phys_obj = dynamic_cast<PhysicsBody*>(obj);
        ItemData* data = phys_obj->getItemData();
        if (data->type == ItemData::ItemType::Obstacle)
            phys_obj->setPosition(server_drone_pos.x, server_drone_pos.y);
    }
}

void sendClientDrone(int socketfd) {
    std::pair<double,double> myPos{0,0};

    createLogicForNewItems();

    std::vector<ItemLogic*> objects = blackboard.getAllLogicObjects();
    for (auto* obj : objects) {
        PhysicsBody* phys_obj = dynamic_cast<PhysicsBody*>(obj);
        ItemData* data = phys_obj->getItemData();
        if (data->type == ItemData::ItemType::Drone) {
            myPos = phys_obj->getPosition();
        }
    }

    Point P = flipHorizontal({myPos.first, myPos.second},
                             {(double)blackboard.getPlayAreaSize().first, (double)blackboard.getPlayAreaSize().second});
    P = flipHorizontal(P,{(double)blackboard.getPlayAreaSize().first, (double)blackboard.getPlayAreaSize().second});

    std::string msg = std::to_string(P.x) + "," + std::to_string(P.y);
    sendString(socketfd, msg);

    std::string ack = recvString(socketfd);
    if (ack != "pok") {
        logger.log("Drone position acknowledgment failed: " + ack, getpid(), Logger::LogLevel::ERROR);
        shutdownFlag = true;
        return;
    }
}

void receiveClientDrone(int socketfd, Point WinRefSize) {
    Point client_drone_pos = parseCoordinates(recvString(socketfd));
    // Acknowledge receipt
    sendString(socketfd, "pok");

    Point localWinSize{(double)blackboard.getPlayAreaSize().first,(double)blackboard.getPlayAreaSize().second};
    createLogicForNewItems();
    
    // Flip and scale properly
    //client_drone_pos = flipHorizontal(client_drone_pos, localWinSize);
    client_drone_pos = scalePointToWindow(client_drone_pos, WinRefSize, localWinSize);

    // Update obstacles
    std::vector<ItemLogic*> objects = blackboard.getAllLogicObjects();
    for (auto* obj : objects) {
        PhysicsBody* phys_obj = dynamic_cast<PhysicsBody*>(obj);
        ItemData* data = phys_obj->getItemData();
        if (data->type == ItemData::ItemType::Obstacle)
            phys_obj->setPosition(client_drone_pos.x, client_drone_pos.y);
    }
}

// ------------------ Client ------------------
void clientLoop(int sockfd, Point WinRefSize) {
    try {
        while (!shutdownFlag) {
            std::string msg = recvString(sockfd);

            if (msg == "drone") {
                receiveServerDrone(sockfd, WinRefSize);
            }
            else if (msg == "obst") {
                sendClientDrone(sockfd);
            }
            else if (msg == "q") {
                sendString(sockfd, "qok");
                logger.log("Quit signal received from server. Exiting client loop.", getpid(), Logger::LogLevel::INFO);
                shutdownFlag = true;
                return;
            }
            else {
                logger.log("Unknown message received: " + msg, getpid(), Logger::LogLevel::WARNING);
            }
        }
    } catch (const std::exception& e) {
        logger.log("Client loop error: " + std::string(e.what()), getpid(), Logger::LogLevel::ERROR);
    }
}

void runClient(const std::string& server_ip, const std::string& server_port = NETWORK_PORT) {
    try {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            logger.log("Failed to create socket", getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            return;
        }

        struct sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(std::stoi(server_port));
        if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
            logger.log("Invalid server IP: " + server_ip, getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            return;
        }

        if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            logger.log("Failed to connect to server", getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            return;
        }

        SocketRAII socket_(sockfd);

        // Handshake
        std::string msg = recvString(socket_.fd());
        if (msg != "ok") {
            logger.log("Server handshake failed: " + msg, getpid(), Logger::LogLevel::ERROR);
            shutdownFlag = true;
            return;
        }
        sendString(socket_.fd(), "ook");

        // Receive window dimensions
        Point WinRefSize = receiveWindowDimensions(socket_.fd());

        clientLoop(socket_.fd(), WinRefSize);
        logger.log("Client loop ended", getpid(), Logger::LogLevel::INFO);

    } catch (const std::exception& e) {
        logger.log(std::string("Client error: ") + e.what(), getpid(), Logger::LogLevel::ERROR);
    }
}

// ------------------ Server Main Loop ------------------
void serverLoop(int clientfd, Point WinRefSize) {
    bool running = true;
    while (running) {

        if (shutdownFlag) {
            sendString(clientfd, "q"); // tell client to quit

            // Wait for acknowledgment with a simple timeout
            auto start = std::chrono::steady_clock::now();
            bool gotAck = false;
            while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
                std::string resp = recvString(clientfd);
                if (resp == "qok") {
                    gotAck = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // avoid busy wait
            }

            if (!gotAck) {
                logger.log("No ack from client, forcing shutdown.", getpid(), Logger::LogLevel::WARNING);
            } else {
                logger.log("Client acknowledged quit signal.", getpid(), Logger::LogLevel::INFO);
            }

            running = false; // exit loop
            break;
        }

        // server running protocol
        try {
            sendString(clientfd, "drone");
            sendServerDrone(clientfd);

            sendString(clientfd, "obst");
            receiveClientDrone(clientfd, WinRefSize);
        } catch (const std::exception& e) {
            // Catch unexpected exceptions to prevent server crash
            logger.log(std::string("Exception in serverLoop: ") + e.what(),
                       getpid(), Logger::LogLevel::ERROR);
            running = false;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));   // small sleep to reduce CPU usage
    }

    logger.log("Server loop exited.", getpid(), Logger::LogLevel::INFO);
}


// ------------------ Run Server ------------------

int runServer() {
    try {

        g_server_fd.reset(bindServerSocket());
        if (g_server_fd.fd() < 0) return -1;

        startListening(g_server_fd.fd());

        // Close old client if exists
        if (g_client_fd.fd() >= 0) g_client_fd.reset();

        int fd = accept(g_server_fd.fd(), nullptr, nullptr);
        if (fd >= 0) {
            g_client_fd.reset(fd);
            logger.log("Client connected", getpid(), Logger::LogLevel::INFO);
        } else if (shutdownFlag) {
            logger.log("Server shutting down before client connected", getpid(), Logger::LogLevel::INFO);
            return -1;
        } else {
            logger.log("accept() failed", getpid(), Logger::LogLevel::ERROR);
            return -1;
        }


        serverHandshake(g_client_fd.fd());

        blackboard.waitForPermission();

        sendWindowDimensions(g_client_fd.fd(),
                             blackboard.getPlayAreaSize().first,
                             blackboard.getPlayAreaSize().second);
        Point WinRefSize = {(double)blackboard.getPlayAreaSize().first,
                             (double)blackboard.getPlayAreaSize().second}; //receiveWindowDimensions(client.fd()); // unchnaged for now

        serverLoop(g_client_fd.fd(), WinRefSize);
        logger.log("Server loop ended", getpid(), Logger::LogLevel::INFO);

    } catch (const std::exception& e) {
        logger.log(std::string("Server error: ") + e.what(), getpid(), Logger::LogLevel::ERROR);
        return -1;
    }
    return 0;
}

// ----------------- SIGNAL -----------------

void handle_sigterm(int signum) {
    shutdownFlag = true;

    if (g_client_fd.fd() >= 0) {
        shutdown(g_client_fd.fd(), SHUT_RDWR); // unblock any read()
        g_client_fd.reset();                   // then close
    }

    if (g_server_fd.fd() >= 0) {
        shutdown(g_server_fd.fd(), SHUT_RDWR); // unblock accept()
        g_server_fd.reset();                   // then close
    }
}


// ------------------ Main ------------------

int main() {
    signal(SIGTERM, handle_sigterm); // from watchdog
    signal(SIGINT, handle_sigterm); // Ctrl+C

    blackboard.setProcessPid(WatchDogProcName::NetworkGate_Proc, getppid());

    Menu::NetworkChoice side = blackboard.getNetworkSide();
    logger.log((side == Menu::NetworkChoice::SERVER) ? "Starting server mode" : "Starting client mode",
               getpid(), Logger::LogLevel::INFO);

    if (side == Menu::NetworkChoice::SERVER)
        runServer();
    else 
        runClient(blackboard.getIP(), blackboard.getPort());

    sendShutdownSignal(networkgate_pipe);
    return 0;
}