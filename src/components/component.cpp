#include "component.hpp"
#include "core/input_system.hpp"
#include "actors/actor.hpp"

Component::Component(weak_ptr<Actor> owner, int updateOrder) : _owner(owner.lock()), _updateOrder(updateOrder) {
}

bool Component::operator<(const Component &rhs) const {
    return _updateOrder < rhs._updateOrder;
}

Component::~Component() = default;
