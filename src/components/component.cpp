#include "component.hpp"

#include "core/engine.hpp"
#include "actors/actor.hpp"

Component::Component(shared_ptr<Engine> engine, int ownerId, int updateOrder) :
    _engine(std::move(engine)), _ownerId(ownerId), _updateOrder(updateOrder) {
}

bool Component::operator<(const Component &rhs) const {
    return _updateOrder < rhs._updateOrder;
}

shared_ptr<Actor> Component::getOwner() {
    if (_parentCache == nullptr) {
        _parentCache = _engine->getActor(_ownerId);
    }
    return _parentCache;
}

Component::~Component() = default;
