#ifndef TargetLogic_H
#define TargetLogic_H

#include "PhysicsBody.h"
#include "BlackBoard.h"

class TargetLogic : public PhysicsBody {
public:
    explicit TargetLogic(ItemData& item_);
    ~TargetLogic() override;

    void update(double dt) override {}  // obstacles are static

    void on_collide_with(PhysicsBody& other);

    // For drawing
    // double scaledX(double scale) const { return data->Pos_x * scale; }
    // double scaledY(double scale) const { return data->Pos_y * scale; }
    //ncursesChar_ symbol() const override { return '*'; }

    ncursesChar_ symbol() const override {
        return '*'; // regular target
    }
};

#endif