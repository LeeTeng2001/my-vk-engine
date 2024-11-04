#pragma once

#include <set>
#include <unordered_set>
#include <glm/gtx/euler_angles.hpp>

#include "utils/common.hpp"

namespace luna {

class Component;
class Engine;

// TODO: actor update priority order, maybe child inherit parent +1?

class Actor {
    public:
        // Only update in Active state, Will remove in EDead.
        enum State { EActive, EPause, EDead };

        // delay init after proper reference is obtained from engine
        explicit Actor() = default;
        virtual ~Actor();

        virtual void delayInit() = 0;
        void delayInit(int actorId, const std::shared_ptr<Engine>& engine) {
            _actorWorldId = actorId;
            _engine = engine;
            delayInit();
        }

        // Debug Display related
        virtual std::string displayName() = 0;
        const std::string& debugDisplayName() {
            if (!_cacheDisplayName.empty()) {
                return _cacheDisplayName;
            }
            _cacheDisplayName = fmt::format("{:s}({:d})", displayName(), _actorWorldId);
            return _cacheDisplayName;
        };

        // Update related (world, components, actor specific)
        void update(float deltaTime);
        void updateComponents(float deltaTime);
        virtual void updateActor(float deltaTime) {};

        // Process input for actor and components
        void processInput(const struct InputState& keyState);
        virtual void actorInput(const struct InputState& keyState) {};

        // Setter
        void setLocalPosition(const glm::vec3& pos) {
            _position = pos;
            _recomputeLocalTransform = true;
        }
        void setWorldPosition(const glm::vec3& pos);
        void setScale(float scale) {
            _scale = scale;
            _recomputeLocalTransform = true;
        }
        void setRotation(const glm::quat& rotation) {
            _rotation = rotation;
            _recomputeLocalTransform = true;
        }
        void setState(State state) { _state = state; }
        void setLock(bool lock) { _locked = lock; };
        void setParent(int parentId);
        void setDebugUiExpand(bool expand) { _debugUiExpanded = expand; };

        // Getter
        [[nodiscard]] const glm::vec3& getLocalPosition() const { return _position; }
        [[nodiscard]] glm::vec3 getWorldPosition() const;
        [[nodiscard]] glm::vec3 getForward() const {
            return glm::mat4_cast(_rotation) * glm::vec4(0, 0, -1, 1);
        };
        [[nodiscard]] glm::vec3 getRight() const {
            return glm::normalize(glm::cross(getForward(), glm::vec3{0, 1, 0}));
        };
        [[nodiscard]] glm::vec3 getUp() const {
            return glm::normalize(glm::cross(getRight(), getForward()));
        };
        [[nodiscard]] float getScale() const { return _scale; }
        [[nodiscard]] const glm::quat& getRotation() const { return _rotation; }
        [[nodiscard]] State getState() const { return _state; }
        [[nodiscard]] const glm::mat4& getLocalTransform();
        [[nodiscard]] glm::mat4 getWorldTransform();
        [[nodiscard]] std::shared_ptr<Engine> getEngine() { return _engine; }
        [[nodiscard]] int getId() { return _actorWorldId; }
        [[nodiscard]] int getParentId() { return _parentId; }
        [[nodiscard]] int getDebugUiExpand() { return _debugUiExpanded; }
        [[nodiscard]] const std::unordered_set<int>& getChildrenIdList() { return _childrenIdList; }

        // Helper function
        void addComponent(const std::shared_ptr<Component>& component);
        void removeComponent(const std::shared_ptr<Component>& component);

        template <class T>
        std::shared_ptr<T> getComponent();

    private:
        void addChild(int childId);
        void removeChild(int childId);

        // Actor state & components
        State _state = EActive;
        bool _recomputeLocalTransform = true;  // when our transform change we need to recalculate
        std::multiset<std::shared_ptr<Component>> _components;
        std::shared_ptr<Engine> _engine;
        int _actorWorldId = -1;

        // debug ui
        bool _debugUiExpanded = false;
        std::string _cacheDisplayName;

        // Hierarchy
        bool _locked = false;  // locked means we cannot further modify the structure of the actor
                               // (component/child)
        int _parentId = -1;
        std::unordered_set<int> _childrenIdList;

        // Transform related
        glm::mat4 _cacheWorldTransform{};
        glm::mat4 _localTransform{};
        glm::vec3 _position{};
        glm::quat _rotation = glm::angleAxis(glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f));
        float _scale = 1;
};

}  // namespace luna

// https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
#include "actor.tpp"
