#include "Space2DLogic.h"

// Resize the play area
void Space2D::resize_to(double new_width_, double new_height_) {
    // Update old_width_ and old_height_ before resizing
    old_width_ = width_;     
    old_height_ = height_;

    // Update to the new dimensions
    width_ = new_width_;         
    height_ = new_height_;    
}

// rescale the positions of all objects in the space based on the new size
void Space2D::rescalePositions(BlackBoard& BB, double new_width_, double new_height_) {
    // Get the previous size of the space (stored in BlackBoard)
    auto [old_width_, old_height_] = BB.getPlayAreaSize();

    // Calculate the scaling factors for X and Y axes based on the new dimensions
    double scaleX = static_cast<double>(new_width_) / old_width_;
    double scaleY = static_cast<double>(new_height_) / old_height_;

    // Ensure that scaling is done only if the window size has changed
    if (new_width_ != old_width_ || new_height_ != old_height_) {
        // Iterate over all objects in the BlackBoard and apply rescaling to their positions
        auto all_objects = BB.getAllLogicObjects();  // Get all logic objects
        
        for (auto* obj : all_objects) {
            if (!obj) continue;  // Skip if the object is null

            // Apply rescaling to each object
            PhysicsBody* phys_obj = dynamic_cast<PhysicsBody*>(obj); 
            if (phys_obj) {
                phys_obj->rescale(scaleX, scaleY);  // Apply the rescaling to position
            }
        }
    }

    // After resizing, update the dimensions stored in the BlackBoard
    BB.setPlayAreaSize(new_width_, new_height_);
}


void Space2D::Wall_Reflect(PhysicsBody* obj) {
    // Constants
    const double epsilon = 1e-6;
    const double min_velocity = 0.1;

    // Get position & velocity
    auto pos = obj->getPosition();
    auto vel = obj->getVelocity();
    double x = pos.first;
    double y = pos.second;
    double vx = vel.first;
    double vy = vel.second;

    // Handle X axis reflection
    if (x <= epsilon || x > width_) {
        vx = -vx;
        if (x <= epsilon) x = -x;
        else x = 2 * width_ - x;
    }

    // Handle Y axis reflection
    if (y <= epsilon || y >= height_) {
        vy = -vy;
        if (y <= epsilon) y = -y;
        else y = 2 * height_ - y;
    }

    // Prevent small velocities from causing continuous reflection
    if (fabs(vx) < min_velocity) vx = 0;
    if (fabs(vy) < min_velocity) vy = 0;

    // Update position and velocity
    obj->setPosition(x, y);
    obj->setVelocity(vx, vy);
}