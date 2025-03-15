#include "camera.hpp"
#include "components/control/move.hpp"
#include "core/input/input_system.hpp"
#include "core/engine.hpp"
#include "core/renderer/renderer.hpp"

#include <glm/gtx/string_cast.hpp>

namespace luna {

constexpr float MOVEMENT_SPEED = 2.0;
constexpr float HOR_ANGLE_SPEED = 30;
constexpr float VERT_ANGLE_SPEED = 10;
constexpr float MIN_ANGLE_PITCH = -85;
constexpr float MAX_ANGLE_PITCH = 85;

void CameraActor::delayInit() {
    _moveComp = std::make_shared<MoveComponent>(getEngine(), getId());
    addComponent(_moveComp);
}

void CameraActor::updateActor(float deltaTime) {
    // set new camera rotation
    if (_moveComp->getEnabled()) {
        glm::vec2 mouseOffset = getEngine()->getInputSystem()->getState().Mouse.getOffsetPosition();
        //        if (mouseOffset.x < 20 && mouseOffset.y < 20) {
        _yawAngle = (_yawAngle - mouseOffset.x * HOR_ANGLE_SPEED * deltaTime);
        _pitchAngle = glm::clamp(_pitchAngle - mouseOffset.y * VERT_ANGLE_SPEED * deltaTime,
                                 MIN_ANGLE_PITCH, MAX_ANGLE_PITCH);
        while (_yawAngle > 360) {
            _yawAngle -= 360;
        }
        while (_yawAngle < 0) {
            _yawAngle += 360;
        }

        // set rotation, yaw first then pitch
        setRotation(glm::eulerAngleYX(glm::radians(_yawAngle), glm::radians(_pitchAngle)));
        //        }
    }

    // Compute new camera from this actor
    getEngine()->getRenderer()->setViewMatrix(getCamViewTransform());
    getEngine()->getRenderer()->setProjectionMatrix(getPerspectiveTransformMatrix());
    getEngine()->getRenderer()->setCamPos(getLocalPosition());

    // update ui
    getEngine()->getRenderer()->writeDebugUi(
        fmt::format("Cam Pos     : {:s}", glm::to_string(getLocalPosition())));
    getEngine()->getRenderer()->writeDebugUi(
        fmt::format("Cam Rot     : {:s}", glm::to_string(getRotation())));
    getEngine()->getRenderer()->writeDebugUi(
        fmt::format("Cam Rot(eul): {:s}", glm::to_string(eulerAngles(getRotation()))));
    getEngine()->getRenderer()->writeDebugUi(
        fmt::format("Cam Forward : {:s}", glm::to_string(getForward())));
    getEngine()->getRenderer()->writeDebugUi(
        fmt::format("Cam Right   : {:s}", glm::to_string(getRight())));
}

void CameraActor::actorInput(const struct InputState &state) {
    float forwardSpeed = 0.0f;
    float strafSpeed = 0.0f;

    // wasd movement
    if (state.Keyboard.getKeyState(SDL_SCANCODE_P) == EPressed) {
        auto l = SLog::get();
        l->info("toggling cam movement enabled");
        _moveComp->setEnable(!_moveComp->getEnabled());
    }
    if (state.Keyboard.getKeyState(SDL_SCANCODE_L) == EPressed) {
        auto l = SLog::get();
        l->info("toggling mouse relative movement");
        getEngine()->getInputSystem()->setRelativeMouseMode(!state.Mouse.isRelative());
    }
    if (state.Keyboard.getKeyState(SDL_SCANCODE_W)) {
        forwardSpeed += MOVEMENT_SPEED;
    }
    if (state.Keyboard.getKeyState(SDL_SCANCODE_S)) {
        forwardSpeed -= MOVEMENT_SPEED;
    }
    if (state.Keyboard.getKeyState(SDL_SCANCODE_A)) {
        strafSpeed -= MOVEMENT_SPEED;
    }
    if (state.Keyboard.getKeyState(SDL_SCANCODE_D)) {
        strafSpeed += MOVEMENT_SPEED;
    }

    _moveComp->setForwardSpeed(forwardSpeed);
    _moveComp->setStrafeSpeed(strafSpeed);
}

// our world coordinate is x right, y up, z in
// get the object in camera space
glm::mat4 CameraActor::getCamViewTransform() {
    // transform to camera space
    // Build look at matrix (combine new basis axis and translation)
    // The rotation is inverse of R = transpose of R
    // carefully study the cross product axis! thus we need to have -lookAtDir
    glm::vec3 upVec(0, 1, 0);

    // calculate forward vector from two rotation angle, already normalised
    glm::vec3 lookAtDir = getForward();

    // use cross product to determine cam base axis, RH rules
    glm::vec3 right = glm::normalize(glm::cross(lookAtDir, upVec));
    glm::vec3 camUp = glm::normalize(glm::cross(right, lookAtDir));
    // NOTICE z is inverted cuz we're constructing new basis, and -z as new basis of inverse lookAt
    glm::mat4 camMatrix{
        glm::vec4{right.x, camUp.x, -lookAtDir.x, 0},
        glm::vec4{right.y, camUp.y, -lookAtDir.y, 0},
        glm::vec4{right.z, camUp.z, -lookAtDir.z, 0},
        glm::vec4{-glm::dot(getLocalPosition(), right), -glm::dot(getLocalPosition(), camUp),
                  glm::dot(getLocalPosition(), lookAtDir), 1},
    };  // construct new axis, where 4th arg is the translation
    // REMEMBER translation is the projection of the lenght onto the NEW axis, so
    // you can't just straight up use position.x!! Must be projected into the new axis

    return camMatrix;
}

glm::mat4 CameraActor::getPerspectiveTransformMatrix() {
    //    // 1. Transform to camera space and rotation
    //    glm::mat4 view = getCamViewTransform();

    // TODO: refactor, should probably cache the value
    const int viewWidth = getEngine()->getRenderer()->getRenderConfig().windowWidth;
    const int viewHeight = getEngine()->getRenderer()->getRenderConfig().windowHeight;

    // transform world coord to view coord
    // World: up:+y, right: +x, forward:-z
    // Clip:  up:-y, right: +x, forward:+z
    glm::mat4 viewSpaceTransform{1};
    viewSpaceTransform[0][0] = 1;
    viewSpaceTransform[1][1] = -1;
    viewSpaceTransform[2][2] = -1;
    viewSpaceTransform[3][3] = 1;

    // 2. use similar triangle to transform to near plane
    // Map z through a non-linear transformation but preserving depth ordering
    // solve m1 * z + m2 = z ^ 2, where it is true when z = n, z = f
    // y' = y * near / z, to represent division we use homogeneous component
    // note that glm multiplies using column matrices!
    glm::mat4 projection{0};
    projection[0][0] = _nearDepth;
    projection[1][1] = _nearDepth;
    projection[2][2] = _nearDepth + _farDepth;
    projection[3][2] = -(_nearDepth * _farDepth);
    projection[2][3] = 1;
    projection[3][3] = 0;

    // 3. scale both side, notice z is scaled depth
    // tan(delta / 2) = newH/2 / near
    //    getEngine()->getRenderer()->
    float aspectRatio = float(viewWidth) / float(viewHeight);
    float nearHeight = glm::tan(glm::radians(_fovYInAngle / 2)) * _nearDepth * 2;
    float nearWidth = nearHeight * aspectRatio;
    glm::mat4 sca(1);
    sca[0][0] = 2.0f / float(nearWidth);
    sca[1][1] = 2.0f / float(nearHeight);  // inverse to clip space y
    sca[2][2] = 1.0f / float(_farDepth);
    projection = sca * projection * viewSpaceTransform;

    return projection;
}

glm::mat4 CameraActor::getOrthographicTransformMatrix() {
    // TODO: refactor, should probably cache the value
    const int viewWidth = getEngine()->getRenderer()->getRenderConfig().windowWidth;
    const int viewHeight = getEngine()->getRenderer()->getRenderConfig().windowHeight;

    // transform world coord to view coord
    // World: up:+y, right: +x, forward:-z
    // Clip:  up:-y, right: +x, forward:+z
    glm::mat4 viewSpaceTransform{1};
    viewSpaceTransform[0][0] = 1;
    viewSpaceTransform[1][1] = -1;
    viewSpaceTransform[2][2] = -1;
    viewSpaceTransform[3][3] = 1;

    // 2. fit into canonical view box by scaling
    // last one is 1/d because vulkan uses [0, 1] instead of [-1, 1] depth
    glm::mat4 projection(1);
    projection[0][0] = 2.0f / float(viewWidth);
    projection[1][1] = 2.0f / float(viewHeight);  // inverse to clip space y
    projection[2][2] = 1.0f / _farDepth;

    return projection;
}

}  // namespace luna