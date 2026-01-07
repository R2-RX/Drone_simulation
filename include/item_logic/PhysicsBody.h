#ifndef PHYSICSBODY_H
#define PHYSICSBODY_H

#include <mutex>
#include <vector>
#include <utility>
#include "ItemLogic.h"

class Space2D; // Forward declaration

class PhysicsBody : public ItemLogic {
public:
    PhysicsBody(ItemData& item_);
    ~PhysicsBody() = default;

    // Initialization
    void initialize(double initial_x, double initial_y) override;

    void setPosition(double x, double y);
    std::pair<double, double> getPosition() const;

    void setVelocity(double vx, double vy);
    std::pair<double,double> getVelocity() const;

    double getMass() const { return data->mass; }
    bool isActive() const { return data->active; }

    // Forces
    double getFx() const { return data->Force_x; }
    double getFy() const { return data->Force_y; }
    
    void resetForces();
    void resetVelocity();
    void apply_thrust(double fx, double fy);
    void computeRepulsiveForce(const std::vector<ItemData*>& obstacles, double rho0);
    void computeAttractiveForce(const ItemData& target);

    // Physics update
    void physical_interaction(double dt);

    // Wall collision
    void checkWallCollision(Space2D& space);

    // Rescale the position of the PhysicsBody based on the scaling factors
    void rescale(double scaleX, double scaleY);

private:
    const double EPSILON = 1e-9;  // To avoid division by zero
    std::mutex obstacle_mutex;
    std::mutex target_mutex;
};

#endif 
