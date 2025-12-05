#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <unordered_set>
#include <algorithm>
#include "Ncurses_Win.h"
#include "Keyboard.h"
#include "config.h"
#include "Pipe.h"
#include "DroneLogic.h"
#include "TargetLogic.h"
#include "ObstacleLogic.h"
#include "PhysicsBody.h"
#include "BlackBoard.h"


BlackBoard blackboard;
Ncurses_Win app;
Space2D plane;

static PhysicsBody* my_drone = nullptr;

const double FIXED_DELTA = 1.0 / 30.0;

void createLogicForNewItems();
void findFirstDrone();
void Update(double dt, Pair_* cmd);
void renderAll();
void write_item_info_to_file(const std::string& filename, BlackBoard& BB, int fps, int ups, double alpha);

//----------------------------------------------------------------
int main() {
    remove("test.txt"); 
    int rows, cols;
    getmaxyx(app.getWindow(), rows, cols);
    plane.resize_to(cols, rows);
    blackboard.setPlayAreaSize(cols, rows);
    auto [reference_width, reference_height] = blackboard.getPlayAreaSize();
    blackboard.setSpawnPermission(true);

    // Create logic objects for items that exist at startup (if any)
    createLogicForNewItems();
    // Set my_drone if present
    findFirstDrone();

    Pipe<Pair_> keyboard_pipe(KEYBOARD_Data_PIPE);

    int64_t previous = blackboard.getGlobalTime();
    double accumulator = 0.0;
    int frames = 0, updates = 0;
    int fps = 0, ups = 0;
    double alpha = 0.0;
    int64_t fps_timer = blackboard.getGlobalTime();

    // -------------------- Main Loop --------------------
    while (true) {
        int ch = getch();
        if (ch == 27) break; // ESC

        getmaxyx(app.getWindow(), rows, cols);

        // Ensure we create logic for newly spawned items every frame (idempotent)
        createLogicForNewItems();
        // If we don't have a drone yet, try to find one
        if (!my_drone) findFirstDrone();

        int64_t now = blackboard.getGlobalTime();
        double frameTime = static_cast<double>(now - previous) * 1e-9;
        previous = now;
        accumulator += frameTime;

        while (accumulator >= FIXED_DELTA) {
            Pair_* cmd = keyboard_pipe.receive_data();
            Update(FIXED_DELTA, cmd);
            accumulator -= FIXED_DELTA;
            updates++;
        }

        //alpha = accumulator / FIXED_DELTA;

        renderAll();
        frames++;

        // FPS / UPS counter
        double elapseTime = static_cast<double>(now - fps_timer) * 1e-9;
        if (elapseTime >= 1.0) {
            fps = frames;
            ups = updates;
            frames = updates = 0;
            fps_timer = now;
        }

        // Handle window resize
        if (ch == KEY_RESIZE) {
            app.resize();
            plane.resize_to(cols, rows);
            blackboard.setPlayAreaSize(cols, rows);
            blackboard.rescalePositions(cols, rows, reference_width, reference_height);
        }
        std::string str = LIVE_MONITORING;
        write_item_info_to_file(str, blackboard, fps, ups, alpha);
        // small sleep to avoid busy spinning if desired
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

//----------------------------------------------------------------------------------------------
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

void findFirstDrone() {
    my_drone = nullptr;
    auto objects = blackboard.getAllLogicObjects();
    for (auto* obj : objects) {
        if (!obj) continue;
        ItemData* data = obj->getItemData();
        if (!data || !data->active) continue;
        if (data->type == ItemData::ItemType::Drone) {
            my_drone = dynamic_cast<PhysicsBody*>(obj);
            if (my_drone) return;
        }
    }
}

// -------------------- Update --------------------

void Update(double dt, Pair_* cmd) {
    blackboard.updateDroneStats(dt);

    if (cmd && my_drone) {
        my_drone->apply_thrust(cmd->x, cmd->y);

        if (cmd->x == 0.0 && cmd->y == 0.0) 
            my_drone->resetForces();  // sets Fx = Fy = 0
    }

    // Reflect only if drone exists
    if (my_drone) {
        plane.Wall_Reflect(my_drone);
    }

    std::vector<ItemLogic*> objects = blackboard.getAllLogicObjects();
    std::vector<PhysicsBody*> to_remove;

    // Find the first active target if current_target is null or inactive
    if (!blackboard.current_target || !blackboard.current_target->active) {
        for (auto* obj : objects) {
            if (!obj) continue;
            ItemData* data = obj->getItemData();
            if (!data || !data->active) continue;
            if (data->type == ItemData::ItemType::Target) {
                blackboard.current_target = data;
                break;
            }
        }
    }

    for (auto* obj : objects) {
        PhysicsBody* phys_obj = dynamic_cast<PhysicsBody*>(obj);
        if (!phys_obj) continue;

        if (my_drone && my_drone != phys_obj) {
            my_drone->on_collide_with(*phys_obj);
        }

        std::vector<ItemData*> obstacles;
        for (auto* other : objects) {
            if (!other) continue;
            ItemData* other_data = other->getItemData();
            if (!other_data || !other_data->active) continue;
            if (other_data->type == ItemData::ItemType::Obstacle)
                obstacles.push_back(other_data);
        }

        ItemData* data = phys_obj->getItemData();
        if (data) {
            if (data->type == ItemData::ItemType::Drone) {
                // Repulsion from obstacles + physical integration
                phys_obj->computeRepulsiveForce(obstacles, 10.0);
                //  Attraction to current target
                //  if (blackboard.current_target)
                //  phys_obj->computeAttractiveForce(*blackboard.current_target);
                phys_obj->physical_interaction(dt);
            }
        }

        if (!data || !data->active) {
            to_remove.push_back(phys_obj);
        }
    }

    // Remove inactive objects safely
    for (auto* obj : to_remove) {
        if (!obj) continue;
        if (obj == my_drone) my_drone = nullptr;
        blackboard.removeLogicObject(obj);
    }
}

// -------------------- Rendering --------------------
void renderAll() {
    app.drawAll(plane, app.getWindow(), blackboard, 1.0);
}

// -------------------- Logging --------------------
void write_item_info_to_file(const std::string& filename, BlackBoard& BB, int fps, int ups, double alpha) {
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) return;

    const auto& logic_objects = BB.getAllLogicObjects();
    for (auto* obj : logic_objects) {
        ItemData* item = obj->getItemData();
        if (!item) continue;
        if (item->type == ItemData::ItemType::Target) continue;
        if (item->type == ItemData::ItemType::Obstacle) continue;

        file << "Item Data:\n";
        file << "---------------------------------\n";
        file << "Type: " << type_to_string(item->type) << "\n";
        file << std::fixed << std::setprecision(2);
        file << "Position: (" << item->Pos_x << ", " << item->Pos_y << ")\n";
        file << "Velocity: (" << item->Vel_x << ", " << item->Vel_y << ")\n";
        file << "Force: (" << item->Force_x << ", " << item->Force_y << ")\n";
        file << "Mass: " << item->mass << "\n";
        file << "Viscous Damping Coef: " << item->visc_damp_coef << "\n";
        file << "Obstacle Repulsion Coef: " << item->repl_coef << "\n";
        file << "Target Attractiveness Coef: " << item->attr_coef << "\n";
        file << "Time Stamp: " << item->timeStamp << "\n";
        file << "Distance Traveled: " << item->distance_traveled << "\n";
        file << "Score: " << item->score << "\n";
        file << "Hit Targets: " << item->number_of_hit_targets << "\n";
        file << "Hit Obstacles: " << item->number_of_hit_obstacles << "\n";
        file << "Active: " << (item->active ? "Yes" : "No") << "\n";
        file << "---------------------------------\n\n";
    }

    file << "FPS: " << fps << "   UPS: " << ups << "   alpha=" << alpha << "\n\n";
    file.close();
}