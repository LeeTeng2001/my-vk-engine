//#pragma once
//
//class Actor {
//public:
//    // Only update in Active state, Will remove in EDead.
//    enum State {
//        EActive, EPause, EDead
//    };
//
//    // Constructor that takes in a dependency, we try to avoid
//    explicit Actor(class Game *game);
//    virtual ~Actor();
//
//    // Update called from game
//    void Update(float deltaTime);
//    // Update all components attached to this actor
//    void UpdateComponents(float deltaTime);
//    // Any actor specific update code
//    virtual void UpdateActor(float deltaTime);
//
//    // Process input for actor and components
//    void ProcessInput(const struct InputState& keyState);
//    virtual void ActorInput(const struct InputState& keyState);
//
//    // Setter
//    void SetPosition(const Vector3& pos) { mPosition = pos; mRecomputeWorldTransform = true; }
//    void SetScale(float scale) { mScale = scale; mRecomputeWorldTransform = true; }
//    void SetRotation(const Quaternion &rotation) { mRotation = rotation; mRecomputeWorldTransform = true; }
//    void SetState(State state) { mState = state; }
//
//    // Getter
//    [[nodiscard]] const Vector3& GetPosition() const { return mPosition; }
//    [[nodiscard]] float GetScale() const { return mScale; }
//    [[nodiscard]] const Quaternion& GetRotation() const { return mRotation; }
//    [[nodiscard]] State GetState() const { return mState; }
//    [[nodiscard]] const Matrix4& GetWorldTransform() const { return mWorldTransform; }
//    class Game* GetGame() { return mGame; }
//
//    // Computation property
//    [[nodiscard]] Vector3 GetForward() const { return Vector3::Transform(Vector3::UnitX, mRotation); }
//    [[nodiscard]] Vector3 GetRight() const { return Vector3::Transform(Vector3::UnitY, mRotation); }
//
//    // Helper function
//    void ComputeWorldTransform();
//    void RotateToNewForward(const Vector3& forward);
//
//    // Add/remove component
//    void AddComponent(class Component* component);
//    void RemoveComponent(class Component* component);
//
//private:
//    // Actor state
//    State mState = EActive;
//    bool mRecomputeWorldTransform = true;  // when our transform change we need to recalculate
//
//    // Transform
//    Matrix4 mWorldTransform;
//    Vector3 mPosition = Vector3::Zero;  // Centre position
//    Quaternion mRotation = Quaternion::Identity;  // Rotation in Quaternion
//    float mScale = 1;  // Uniform scale of actor
//
//    // Components
//    vector<class Component*> mComponents;
//    class Game *mGame;
//};
//
//
//
