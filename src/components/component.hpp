#pragma once

// components is unaware of owner actor
// actor should be responsible for linking / unreferencing components

class Component {
public:
    // (the lower the update order, the earlier the component updates)
    explicit Component(class Actor* owner, int updateOrder = 100);
    virtual ~Component();

    // update related
    virtual void update(float deltaTime) {};
    virtual void processInput(const struct InputState& keyState) {};

    // Notify when parent's being updated
    virtual void onUpdateWorldTransform() {};

    // Getter
    [[nodiscard]] class Actor* getOwner() { return _owner; }
    [[nodiscard]] int getUpdateOrder() const { return _updateOrder; }

    bool operator<(const Component &rhs) const;

protected:
    class Actor* _owner;
    int _updateOrder;
};
