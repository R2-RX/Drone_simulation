#ifndef Space2DLogic_H
#define Space2DLogic_H

#include <vector>
#include <cmath>
#include "PhysicsBody.h"
#include "BlackBoard.h"

class Space2D {
public:
    Space2D() = default;
    ~Space2D() = default;

    // Resize the play area and rescale object positions based on the new size
    void resize_to(double new_width_, double new_height_);
    void rescalePositions(BlackBoard& BB, double new_width_, double new_height_);

    // Wall reflection for a single PhysicsBody object
    void Wall_Reflect(PhysicsBody* obj);

    // Update all physics objects in a container
    template<typename Container>
    void updatePhysicsObjects(Container& physicsObjects, double dt) {
        for (auto* obj : physicsObjects) {
            obj->physical_interaction(dt);
            obj->checkWallCollision(*this);  // Handle wall collisions
        }
    }

private:
    double width_{0.0};    // Current width of the space
    double height_{0.0};   // Current height of the space

    double old_width_{0.0};    // Previous width of the space (for resizing)
    double old_height_{0.0};   // Previous height of the space (for resizing)

};

#endif // Space2DLogic_H
