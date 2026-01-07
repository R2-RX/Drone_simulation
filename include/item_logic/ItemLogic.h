#ifndef ITEMLOGIC_H
#define ITEMLOGIC_H

#include "ItemData.h"
#include <ncurses.h>
#include <mutex>
#include <locale.h>

typedef chtype ncursesChar_;

class PhysicsBody; // Forward declaration

class ItemLogic {
protected:
    ItemData* data;
    mutable std::mutex data_mutex;  //mutalbe to allow locking in const methods

public:
    int bb_index = -1;           // Slot index in BlackBoard pool
    uint32_t bb_generation = 0;  // Generation stamp for safety (BlackBoard)

public:
    explicit ItemLogic(ItemData& item_);
    virtual ~ItemLogic() = default;

    ItemData* getItemData() const { return data; }

    // Primary update method (scaling or physics can happen here)
    virtual void update(double dt) = 0;

    // Collision handling
    virtual void on_collide_with(PhysicsBody& other) = 0;

    // Scaled positions for rendering
    double scaledX(double scale) const { return data->Pos_x * scale; }
    double scaledY(double scale) const { return data->Pos_y * scale; }
    virtual void initialize(double initial_x, double initial_y) = 0;
    // Symbol for ncurses display
    virtual ncursesChar_ symbol() const = 0;

    // Reset all item data to default
    void reset();

    std::mutex& get_mutex() const { return data_mutex; }

    void deactivate() { data->active = false; }
    void activate() { data->active = true; }
};

#endif // ITEMLOGIC_H
