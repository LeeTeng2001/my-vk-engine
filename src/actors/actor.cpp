//#include "actor.hpp"
//#include "../Game.hpp"
//#include "../components/Component.hpp"
//#include "../core/InputSystem.hpp"
//
//Actor::Actor(Game *game) : mGame(game) {
//    mGame->AddActor(this);
//}
//
//Actor::~Actor() {
//    mGame->RemoveActor(this);
//
//    // Need to delete components, because ~Component calls RemoveComponent, need a different style loop
//    while (!mComponents.empty()) {
//        delete mComponents.back();
//    }
//}
//
//void Actor::Update(float deltaTime) {
//    if (mState == EActive) {
//        ComputeWorldTransform();
//        UpdateComponents(deltaTime);
//        UpdateActor(deltaTime);
//        ComputeWorldTransform();
//    }
//}
//
//void Actor::UpdateComponents(float deltaTime) {
//    for (auto comp: mComponents) {
//        comp->Update(deltaTime);
//    }
//}
//
//void Actor::UpdateActor(float deltaTime) {
//    // Actor specific update
//}
//
//void Actor::ProcessInput(const InputState &keyState) {
//    // process input for components, then actor specific
//    if (mState == EActive) {
//        for (auto comp: mComponents) {
//            comp->ProcessInput(keyState);
//        }
//        ActorInput(keyState);
//    }
//}
//
//void Actor::ActorInput(const InputState &keyState) {
//    // Actor specific input
//}
//
//void Actor::AddComponent(Component *component) {
//    // Find the insertion point in the sorted vector
//    // (The first element with an order higher than me)
//    int myOrder = component->GetUpdateOrder();
//    auto iter = mComponents.begin();
//    while (iter != mComponents.end()) {
//        if (myOrder < (*iter)->GetUpdateOrder())
//            break;
//        ++iter;
//    }
//
//    // Inserts element before position of iterator
//    mComponents.insert(iter, component);
//}
//
//void Actor::RemoveComponent(Component *component) {
//    auto iter = std::find(mComponents.begin(), mComponents.end(), component);
//    if (iter != mComponents.end()) {
//        mComponents.erase(iter);
//    }
//}
//
//void Actor::ComputeWorldTransform() {
//    if (mRecomputeWorldTransform) {
//        mRecomputeWorldTransform = false;
//
//        // Scale, then rotate, then translate
//        mWorldTransform = Matrix4::CreateScale(mScale);
//        mWorldTransform *= Matrix4::CreateFromQuaternion(mRotation);
//        mWorldTransform *= Matrix4::CreateTranslation(mPosition);
//
//        // Inform components world transform updated
//        for (auto comp: mComponents) {
//            comp->OnUpdateWorldTransform();
//        }
//    }
//}
//
//void Actor::RotateToNewForward(const Vector3 &forward) {
//    // Figure out difference between original (unit x) and new
//    float dot = Vector3::Dot(Vector3::UnitX, forward);
//    float angle = Math::Acos(dot);
//    if (dot > 0.9999f) {
//        // Facing down X
//        SetRotation(Quaternion::Identity);
//    }
//    else if (dot < -0.9999f) {
//        // Facing down -X
//        SetRotation(Quaternion(Vector3::UnitZ, Math::Pi));
//    } else {
//        // Rotate about axis from cross product
//        Vector3 axis = Vector3::Cross(Vector3::UnitX, forward);
//        axis.Normalize();
//        SetRotation(Quaternion(axis, angle));
//    }
//}
