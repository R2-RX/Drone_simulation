#include "Space2DLogic.h"

// Resize the play area
void Space2D::resize_to(double width_m, double height_m) {
    width_ = width_m;
    height_ = height_m;
}

// Wall reflection for a single object
void Space2D::Wall_Reflect(PhysicsBody* obj) {
    // Get position & velocity
    auto pos = obj->getPosition();
    auto vel = obj->getVelocity();

    double x = pos.first;
    double y = pos.second;
    double vx = vel.first;
    double vy = vel.second;

    // Reflect X axis
    if (x <= 0.0) { x = -x; vx = -vx; }
    else if (x > width_) { x = 2*width_ - x; vx = -vx; }

    // Reflect Y axis
    if (y <= 0.0) { y = -y; vy = -vy; }
    else if (y > height_) { y = 2*height_ - y; vy = -vy; }

    // Update back
    obj->setPosition(x, y);
    obj->setVelocity(vx, vy);
}
