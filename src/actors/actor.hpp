#pragma once

#include "utils/common.hpp"
#include <set>
#include <glm/gtx/euler_angles.hpp>

class Component;
class Engine;

class Actor {
public:
    // Only update in Active state, Will remove in EDead.
    enum State {
        EActive, EPause, EDead
    };

    // delay init after proper reference is obtained
    explicit Actor() = default;
    virtual ~Actor();

    virtual void delayInit() = 0;
    void delayInit(const shared_ptr<Actor>& self, const shared_ptr<Engine> &engine) { _selfPtr = self; _engine = engine; delayInit(); }

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
    [[nodiscard]] const glm::vec3& getLocalPosition() const { return _position; }
    [[nodiscard]] glm::vec3 getForward() const { return glm::mat4_cast(_rotation) * glm::vec4(0, 0, 1, 1); };
    [[nodiscard]] glm::vec3 getRight() const { return glm::normalize(glm::cross(glm::vec3{0, 1, 0}, getForward())); };
    [[nodiscard]] glm::vec3 getUp() const { return glm::normalize(glm::cross(getRight(), getForward())); };
    [[nodiscard]] float getScale() const { return _scale; }
    [[nodiscard]] const glm::quat& getRotation() const { return _rotation; }
    [[nodiscard]] State getState() const { return _state; }
    [[nodiscard]] const glm::mat4& getWorldTransform() const { return _worldTransform; }
    [[nodiscard]] shared_ptr<Engine> getEngine() { return _engine; }
    [[nodiscard]] weak_ptr<Actor> getSelf() { return _selfPtr; }

    // Helper function
    void computeWorldTransform();
//    void RotateToNewForward(const Vector3& forward);
    void addComponent(const shared_ptr<Component>& component);
    void removeComponent(const shared_ptr<Component>& component);

private:
    // Actor state & components
    State _state = EActive;
    bool _recomputeWorldTransform = true;  // when our transform change we need to recalculate
    std::multiset<shared_ptr<Component>> _components;
    shared_ptr<Engine> _engine;
    weak_ptr<Actor> _selfPtr;

    // Transform related
    glm::mat4 _worldTransform{};
    glm::vec3 _position{};
    glm::quat _rotation = glm::angleAxis(glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f));
    float _scale = 1;
};



