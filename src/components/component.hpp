#pragma once

#include "utils/common.hpp"

// components is unaware of owner actor
// actor should be responsible for linking / unreferencing components

class Actor;
class Engine;

class Component {
public:
    // (the lower the update order, the earlier the component updates)
    explicit Component(weak_ptr<Actor> owner, int updateOrder = 100);
    virtual ~Component();

    // update related
    virtual void update(float deltaTime) {};
    virtual void processInput(const struct InputState& keyState) {};

    // Notify when parent's being updated
    virtual void onUpdateWorldTransform() {};

    // Getter
    [[nodiscard]] shared_ptr<Actor> getOwner() { return _owner; }
    [[nodiscard]] int getUpdateOrder() const { return _updateOrder; }
    [[nodiscard]] bool getEnabled() const { return _enable; }

    // Setter
    void setEnable(bool enable) { _enable = enable; }

    bool operator<(const Component &rhs) const;

private:
    bool _enable = true;
    shared_ptr<Actor> _owner;
    int _updateOrder;
};
