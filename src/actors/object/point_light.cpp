#include "point_light.hpp"
#include "core/engine.hpp"
#include "renderer/renderer.hpp"
#include "components/anim/tween.hpp"

void PointLightActor::delayInit() {
}

void PointLightActor::updateActor(float deltaTime) {
    getEngine()->getRenderer()->setLightInfo(getLocalPosition(), _color, _radius);
}
