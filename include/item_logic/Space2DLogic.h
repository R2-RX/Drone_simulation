#ifndef Space2DLogic
#define Space2DLogic_H

#include <vector>
#include "PhysicsBody.h"

class Space2D {
public:
    Space2D() = default;
    ~Space2D() = default;

    // Resize play area
    void resize_to(double width_m, double height_m);

    // Wall reflection for a single PhysicsLogic object
    void Wall_Reflect(PhysicsBody* obj);

    // Update all physics objects in a container
    // Template function definitions must be in header
    template<typename Container>
    void updatePhysicsObjects(Container& physicsObjects, double dt) {
        for (auto* obj : physicsObjects) {
            obj->physical_interaction(dt);           
            obj->checkWallCollision(*this);  // handle wall collisions
        }
    }


private:
    double width_{0.0};
    double height_{0.0};
};

#endif