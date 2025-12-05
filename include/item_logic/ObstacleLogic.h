#ifndef OBSTACLELOGIC_H
#define OBSTACLELOGIC_H

#include "PhysicsBody.h"

class ObstacleLogic : public PhysicsBody {
public:
    explicit ObstacleLogic(ItemData& item_);
    ~ObstacleLogic() override;

    void update(double dt) override {}  // obstacles are static

    void on_collide_with(PhysicsBody& other);

    // For drawing
    // double scaledX(double scale) const { return data->Pos_x * scale; }
    // double scaledY(double scale) const { return data->Pos_y * scale; }
    ncursesChar_ symbol() const override { return 'O'; }
};

#endif