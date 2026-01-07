#include "PhysicsBody.h"
#include "Space2DLogic.h" 
#include <cmath>

// Constructor
PhysicsBody::PhysicsBody(ItemData& item_) : ItemLogic(item_) {}

// Initialize body state
void PhysicsBody::initialize(double initial_x, double initial_y) {
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
    std::lock_guard<std::mutex> lock(data_mutex);
    data->Pos_x = x;
    data->Pos_y = y;
}

std::pair<double, double> PhysicsBody::getPosition() const {
    return {data->Pos_x, data->Pos_y};
}

void PhysicsBody::setVelocity(double vx, double vy) {
    std::lock_guard<std::mutex> lock(data_mutex);
    data->Vel_x = vx;
    data->Vel_y = vy;
}

std::pair<double,double> PhysicsBody::getVelocity() const {
    return { data->Vel_x, data->Vel_y };
}

// -------------------- Force and Velocity Handling --------------------
void PhysicsBody::resetForces() {
    //std::lock_guard<std::mutex> lock(data_mutex);
    data->Force_x = 0.0;
    data->Force_y = 0.0;
}

void PhysicsBody::resetVelocity() {
    //std::lock_guard<std::mutex> lock(data_mutex);
    data->Vel_x = 0.0;
    data->Vel_y = 0.0;
}
// Apply thrust
void PhysicsBody::apply_thrust(double fx, double fy) {
    data->Force_x += fx;
    data->Force_y += fy;
}

// Compute repulsive force from obstacles only along the 8 main 2D directions
void PhysicsBody::computeRepulsiveForce(const std::vector<ItemData*>& obstacles, double rho0) {
    double fx_total = 0.0;
    double fy_total = 0.0;

    // Define 8 main directions as unit vectors
    const std::vector<std::pair<double, double>> directions = {
        { 1, 0},   // right
        {-1, 0},   // left
        { 0, 1},   // up
        { 0,-1},   // down
        { 1, 1},   // up-right
        { 1,-1},   // down-right
        {-1, 1},   // up-left
        {-1,-1}    // down-left
    };

    for (const auto* ob : obstacles) {
        if (!ob->active) continue;

        double dx = data->Pos_x - ob->Pos_x;
        double dy = data->Pos_y - ob->Pos_y;
        double rho = std::sqrt(dx*dx + dy*dy);

        if (rho > rho0 || rho < EPSILON) continue;

        // Compute unit vector from obstacle to self
        double inv = 1.0 / rho;
        double ux = dx * inv;
        double uy = dy * inv;

        // Find nearest main direction
        double max_dot = -1.0;
        double qx = 0.0, qy = 0.0;
        for (auto [dir_x, dir_y] : directions) {
            // Normalize diagonal directions
            if (std::abs(dir_x) + std::abs(dir_y) == 2) {
                dir_x *= 0.70710678; // 1/sqrt(2)
                dir_y *= 0.70710678;
            }

            double dot = ux * dir_x + uy * dir_y;
            if (dot > max_dot) {
                max_dot = dot;
                qx = dir_x;
                qy = dir_y;
            }
        }

        // Compute repulsion magnitude
        double term = (1.0 / rho - 1.0 / rho0);
        double scale = data->repl_coef * term * (inv * inv);

        // Add to total force along quantized direction
        fx_total += scale * qx;
        fy_total += scale * qy;
    }

    // Boost by mass so heavy drones feel stronger repulsion
    fx_total *= this->getItemData()->mass * 10;
    fy_total *= this->getItemData()->mass * 10;

    data->Force_x += fx_total;
    data->Force_y += fy_total;
}

// Compute repulsive force from obstacles in the exact direction to each obstacle
// void PhysicsBody::computeRepulsiveForce(const std::vector<ItemData*>& obstacles, double rho0) {
//     double fx_total = 0.0;
//     double fy_total = 0.0;

//      // lock self
//     for (const auto* ob : obstacles) {
//         if (!ob->active) continue;

//         double dx = data->Pos_x - ob->Pos_x;
//         double dy = data->Pos_y - ob->Pos_y;
//         double rho = std::sqrt(dx*dx + dy*dy);

//         if (rho > rho0 || rho < EPSILON) continue;

//         double inv = 1.0 / rho;
//         double ux = dx * inv;
//         double uy = dy * inv;
//         double term = (1.0 / rho - 1.0 / rho0);
//         double scale = data->repl_coef * term * (inv * inv);

//         fx_total += scale * ux;
//         fy_total += scale * uy;
//     }

//     // Boost by mass so heavy drones feel stronger repulsion
//     fx_total *= this->getItemData()->mass * 10;
//     fy_total *= this->getItemData()->mass * 10;

//     data->Force_x += fx_total;
//     data->Force_y += fy_total;
// }

// Compute attractive force toward target
void PhysicsBody::computeAttractiveForce(const ItemData& target) {

    double dx = data->Pos_x - target.Pos_x;
    double dy = data->Pos_y - target.Pos_y;

    data->Force_x += -data->attr_coef * dx;
    data->Force_y += -data->attr_coef * dy;
}

// Physics update: forces -> acceleration -> velocity -> position
void PhysicsBody::physical_interaction(double dt) {

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
    space.Wall_Reflect(this);
}

// Rescale the position of the PhysicsBody based on the scaling factors
void PhysicsBody::rescale(double scaleX, double scaleY) {
    auto [initX, initY] = getPosition();
    double scaledX = initX * scaleX;
    double scaledY = initY * scaleY;
    setPosition(scaledX, scaledY);  // Apply the scaled position to the object
}