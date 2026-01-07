cmake_minimum_required(VERSION 3.10)
project(DroneProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/communication
    ${PROJECT_SOURCE_DIR}/include/input_devices
    ${PROJECT_SOURCE_DIR}/include/item_data
    ${PROJECT_SOURCE_DIR}/include/item_logic
    ${PROJECT_SOURCE_DIR}/include/logger
    ${PROJECT_SOURCE_DIR}/include/windowing
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncursesw)

# ===================================================================
# 1. Shared Library: All common logic, data, communication, etc.
# ===================================================================

add_library(DroneProject_lib SHARED
    src/communication/BlackBoard.cpp
    src/communication/Communication.cpp
    src/communication/NetworkSocket.cpp
    src/communication/Pipe.cpp
    src/communication/SharedMemoryData.cpp

    src/input_devices/Keyboard.cpp

    src/item_data/ItemData.cpp

    src/item_logic/DroneLogic.cpp
    src/item_logic/ItemLogic.cpp
    src/item_logic/ObstacleLogic.cpp
    src/item_logic/PhysicsBody.cpp
    src/item_logic/Space2DLogic.cpp
    src/item_logic/TargetLogic.cpp

    src/logger/Logger.cpp

    src/windowing/Ncurses_Win.cpp
)

target_link_libraries(DroneProject_lib ${NCURSES_LIBRARIES} pthread)

# ===================================================================
# 2. Process executables (each has its own main)
# ===================================================================

set(PROC_SOURCES
    src/Proc/Temp_Proc.cpp
    src/Proc/NetworkGate_Proc.cpp
    src/Proc/GameLoop_Proc.cpp
    src/Proc/ItemSpawner_Proc.cpp
    src/Proc/KeyBoard_Proc.cpp
    src/Proc/Master_Proc.cpp
    src/Proc/WatchDog_Proc.cpp
    src/Proc/GlobalTimer_Proc.cpp
)

# Additional sources *only for GameLoop_Proc*
set(RENDERING_EXTRA
    src/item_logic/DroneLogic.cpp
    src/item_logic/ItemLogic.cpp
    src/item_logic/PhysicsBody.cpp
    src/item_logic/ObstacleLogic.cpp
    src/item_logic/Space2DLogic.cpp
    src/item_logic/TargetLogic.cpp
    src/item_data/ItemData.cpp
)

# Create output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/processes)

foreach(PROC_SRC ${PROC_SOURCES})
    get_filename_component(EXE_NAME ${PROC_SRC} NAME_WE)

    message(STATUS "Building process executable: ${EXE_NAME}")

    if(EXE_NAME STREQUAL "GameLoop_Proc")
        add_executable(${EXE_NAME}
            ${PROC_SRC}
            ${RENDERING_EXTRA}
        )
    else()
        add_executable(${EXE_NAME}
            ${PROC_SRC}
        )
    endif()

    target_link_libraries(${EXE_NAME}
        DroneProject_lib
        ${NCURSES_LIBRARIES}
        pthread
    )

    set_target_properties(${EXE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/processes
    )
endforeach()
