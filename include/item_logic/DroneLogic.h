#ifndef DRONELOGIC_H
#define DRONELOGIC_H

#include "PhysicsBody.h"

class DroneLogic : public PhysicsBody {
public:
    explicit DroneLogic(ItemData& item_);
    ~DroneLogic() override = default;

    // Update drone's state each timestep (forces, movement, scoring)
    void update(double dt) override;

    // Handle collisions with other physics objects
    void on_collide_with(PhysicsBody& other) override;

    // Symbol for drawing
    ncursesChar_ symbol() const override { return '#'; }

};

#endif
