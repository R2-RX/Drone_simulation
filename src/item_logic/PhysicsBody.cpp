#include "PhysicsBody.h"
#include "Space2DLogic.h" 
#include <cmath>


// Constructor
PhysicsBody::PhysicsBody(ItemData& item_) : ItemLogic(item_) {}

// Initialize body state
void PhysicsBody::initialize(double initial_x, double initial_y) {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    data->Pos_x = initial_x;
    data->Pos_y = initial_y;
    data->Vel_x = 0.0;
    data->Vel_y = 0.0;
    data->Force_x = 0.0;
    data->Force_y = 0.0;
    data->distance_traveled = 0.0;
    data->mass = 1.0;
    data->active = true;
}
// -------------------- Position & Velocity --------------------
void PhysicsBody::setPosition(double x, double y) {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    data->Pos_x = x;
    data->Pos_y = y;
}

std::pair<double, double> PhysicsBody::getPosition() const {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    return {data->Pos_x, data->Pos_y};
}

void PhysicsBody::setVelocity(double vx, double vy) {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    data->Vel_x = vx;
    data->Vel_y = vy;
}

std::pair<double,double> PhysicsBody::getVelocity() const {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    return { data->Vel_x, data->Vel_y };
}

// -------------------- Force Handling --------------------
void PhysicsBody::resetForces() {
    std::lock_guard<std::mutex> lock(data_mutex);
    data->Force_x = 0.0;
    data->Force_y = 0.0;
}

// Apply thrust
void PhysicsBody::apply_thrust(double fx, double fy) {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    data->Force_x += fx;
    data->Force_y += fy;
}

// Compute repulsive force from obstacles
void PhysicsBody::computeRepulsiveForce(const std::vector<ItemData*>& obstacles, double rho0) {
    double fx_total = 0.0;
    double fy_total = 0.0;

    std::lock_guard<std::mutex> lock(item_data_mutex); // lock self
    for (const auto* ob : obstacles) {
        std::lock_guard<std::mutex> ob_lock(obstacle_mutex); // lock obstacle
        if (!ob->active) continue;

        double dx = data->Pos_x - ob->Pos_x;
        double dy = data->Pos_y - ob->Pos_y;
        double rho = std::sqrt(dx*dx + dy*dy);

        if (rho > rho0 || rho < EPSILON) continue;

        double inv = 1.0 / rho;
        double ux = dx * inv;
        double uy = dy * inv;
        double term = (1.0 / rho - 1.0 / rho0);
        double scale = data->repl_coef * term * (inv * inv);

        fx_total += scale * ux;
        fy_total += scale * uy;
    }

    data->Force_x += fx_total;
    data->Force_y += fy_total;
}

// Compute attractive force toward target
void PhysicsBody::computeAttractiveForce(const ItemData& target) {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    std::lock_guard<std::mutex> target_lock(target_mutex); // read target safely

    double dx = data->Pos_x - target.Pos_x;
    double dy = data->Pos_y - target.Pos_y;

    data->Force_x += -data->attr_coef * dx;
    data->Force_y += -data->attr_coef * dy;
}

// Physics update: forces -> acceleration -> velocity -> position
void PhysicsBody::physical_interaction(double dt) {
    std::lock_guard<std::mutex> lock(item_data_mutex);

    const double inv_mass = (data->mass > EPSILON) ? 1.0 / data->mass : EPSILON;

    // Apply viscous drag
    double drag_x = -data->visc_damp_coef * data->Vel_x;
    double drag_y = -data->visc_damp_coef * data->Vel_y;

    // Total forces
    double Fx = data->Force_x + drag_x;
    double Fy = data->Force_y + drag_y;

    // Accelerations
    double ax = Fx * inv_mass;
    double ay = Fy * inv_mass;

    // Update velocity
    data->Vel_x += ax * dt;
    data->Vel_y += ay * dt;

    // Update position
    data->Pos_x += data->Vel_x * dt;
    data->Pos_y += data->Vel_y * dt;

    // Track distance traveled for drones
    if (data->type == ItemData::ItemType::Drone) {
        data->distance_traveled += std::hypot(data->Vel_x * dt, data->Vel_y * dt);
    }

    // Reset forces for next step
    // data->Force_x = 0.0;
    // data->Force_y = 0.0;
}

// Wall collision
void PhysicsBody::checkWallCollision(Space2D& space) {
    std::lock_guard<std::mutex> lock(item_data_mutex);
    space.Wall_Reflect(this);
}
