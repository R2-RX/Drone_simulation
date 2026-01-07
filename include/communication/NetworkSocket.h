#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <unordered_set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>

#include <cerrno>
#include <string>
#include <type_traits>
#include <system_error>
#include <cmath>
#include <semaphore.h>
#include <thread>
#include <fcntl.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

#include "Logger.h"
#include "config.h"

std::string getWlanIp(Logger& logger, const std::string& iface = "wlo1") {
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        logger.log("getifaddrs failed", getpid(), Logger::LogLevel::ERROR);
        return "";
    }

    std::string wlan_ip = "";
    std::string iface_to_use = iface;

    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;

        // If default iface not found, pick the first interface starting with "wl"
        if (iface_to_use.empty() || iface_to_use == ifa->ifa_name || (iface == "wlo1" && strncmp(ifa->ifa_name, "wl", 2) == 0)) {
            char ip[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, ip, sizeof(ip)) != nullptr) {
                wlan_ip = ip;
                iface_to_use = ifa->ifa_name; // record actual interface used
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return wlan_ip;
}


#endif // NET_SOCKET_H
