#include "component.hpp"
#include "core/input_system.hpp"
#include "actors/actor.hpp"

Component::Component(Actor *owner, int updateOrder) : _owner(owner), _updateOrder(updateOrder) {
}

bool Component::operator<(const Component &rhs) const {
    return _updateOrder < rhs._updateOrder;
}

Component::~Component() = default;
