#pragma once

#include "utils/common.hpp"

class Actor {
public:
    // Only update in Active state, Will remove in EDead.
    enum State {
        EActive, EPause, EDead
    };

    // Constructor that takes in a dependency, we try to avoid
    explicit Actor() = default;
    virtual ~Actor();

    // Update related (world, components, specific)
    void update(float deltaTime);
    void updateComponents(float deltaTime);
    virtual void updateActor(float deltaTime);

    // Process input for actor and components
    void processInput(const struct InputState& keyState);
    virtual void actorInput(const struct InputState& keyState);

    // Setter
    void setPosition(const glm::vec3& pos) { _position = pos; _recomputeWorldTransform = true; }
    void setScale(float scale) { _scale = scale; _recomputeWorldTransform = true; }
    void setRotation(const glm::quat &rotation) { _rotation = rotation; _recomputeWorldTransform = true; }
    void setState(State state) { _state = state; }

//    // Getter
//    [[nodiscard]] const Vector3& GetPosition() const { return mPosition; }
//    [[nodiscard]] float GetScale() const { return mScale; }
//    [[nodiscard]] const Quaternion& GetRotation() const { return mRotation; }
//    [[nodiscard]] State GetState() const { return mState; }
//    [[nodiscard]] const Matrix4& GetWorldTransform() const { return mWorldTransform; }
//    class Game* GetGame() { return mGame; }

    // Helper function
    void computeWorldTransform();
//    void RotateToNewForward(const Vector3& forward);

//    // Add/remove component
//    void AddComponent(class Component* component);
//    void RemoveComponent(class Component* component);

private:
    // Actor state & components
    State _state = EActive;
    bool _recomputeWorldTransform = true;  // when our transform change we need to recalculate
//    vector<class Component*> mComponents;
//    class Game *mGame;

    // Transform related
    glm::mat4 _worldTransform{};
    glm::vec3 _position{};
    glm::quat _rotation{};
    float _scale = 1;
};



