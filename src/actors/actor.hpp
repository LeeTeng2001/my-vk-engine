#pragma once

#include "utils/common.hpp"
#include <set>

class Actor {
public:
    // Only update in Active state, Will remove in EDead.
    enum State {
        EActive, EPause, EDead
    };

    // Constructor that takes in a dependency, we try to avoid
    explicit Actor() = default;
    virtual ~Actor();

    // Update related (world, components, actor specific)
    void update(float deltaTime);
    void updateComponents(float deltaTime);
    virtual void updateActor(float deltaTime) {};

    // Process input for actor and components
    void processInput(const struct InputState& keyState);
    virtual void actorInput(const struct InputState& keyState) {};

    // Setter
    void setPosition(const glm::vec3& pos) { _position = pos; _recomputeWorldTransform = true; }
    void setScale(float scale) { _scale = scale; _recomputeWorldTransform = true; }
    void setRotation(const glm::quat &rotation) { _rotation = rotation; _recomputeWorldTransform = true; }
    void setState(State state) { _state = state; }

    // Getter
    [[nodiscard]] const glm::vec3& getPosition() const { return _position; }
    [[nodiscard]] float getScale() const { return _scale; }
    [[nodiscard]] const glm::quat& getRotation() const { return _rotation; }
    [[nodiscard]] State getState() const { return _state; }
    [[nodiscard]] const glm::mat4& getWorldTransform() const { return _worldTransform; }
//    class Game* getGame() { return mGame; }

    // Helper function
    void computeWorldTransform();
//    void RotateToNewForward(const Vector3& forward);
    void addComponent(class Component *component);
    void removeComponent(class Component* component);

private:
    // Actor state & components
    State _state = EActive;
    bool _recomputeWorldTransform = true;  // when our transform change we need to recalculate
    std::multiset<class Component*> _components;
//    class Game *mGame;

    // Transform related
    glm::mat4 _worldTransform{};
    glm::vec3 _position{};
    glm::quat _rotation{};
    float _scale = 1;
};



