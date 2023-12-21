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

    // calculate new state
    virtual void update(float deltaTime) {};
    // post update happens when all actor update and components are processed, should only push result and not calculate new result
    virtual void postUpdate() {};
    virtual void processInput(const struct InputState& keyState) {};

    // Getter
    [[nodiscard]] shared_ptr<Actor> getOwner() { return _owner.lock(); }
    [[nodiscard]] int getUpdateOrder() const { return _updateOrder; }
    [[nodiscard]] bool getEnabled() const { return _enable; }

    // Setter
    void setEnable(bool enable) { _enable = enable; }

    bool operator<(const Component &rhs) const;

private:
    bool _enable = true;
    weak_ptr<Actor> _owner;
    int _updateOrder;
};
