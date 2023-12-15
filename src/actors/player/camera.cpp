#include <glm/gtx/string_cast.hpp>

#include "camera.hpp"
#include "components/control/move.hpp"
#include "core/input_system.hpp"
#include "core/engine.hpp"
#include "renderer/renderer.hpp"

constexpr float MOVEMENT_SPEED = 2.0;
constexpr float HOR_ANGLE_SPEED = 30;
constexpr float VERT_ANGLE_SPEED = 10;

void CameraActor::delayInit() {
    _moveComp = make_shared<MoveComponent>(getSelf());
    addComponent(_moveComp);
}

void CameraActor::updateActor(float deltaTime) {
    // Compute new camera from this actor
    getEngine()->getRenderer()->setUniform(MrtPushConstantData{
        getPerspectiveTransformMatrix(),
        glm::vec3{0, 3, 0},
        0,
    });

    // update ui
    getEngine()->getRenderer()->writeDebugUi(fmt::format("Cam Pos     : {:s}", glm::to_string(getPosition())));
    getEngine()->getRenderer()->writeDebugUi(fmt::format("Cam Rot     : {:s}", glm::to_string(getRotation())));
    getEngine()->getRenderer()->writeDebugUi(fmt::format("Cam Forward : {:s}", glm::to_string(getForward())));
    getEngine()->getRenderer()->writeDebugUi(fmt::format("Cam Right   : {:s}", glm::to_string(getRight())));
}

void CameraActor::actorInput(const struct InputState &state) {
    float forwardSpeed = 0.0f;
    float strafSpeed = 0.0f;
    float angularSpeed = 0.0f;
    float pitchSpeed = 0.0f;

    // wasd movement
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

    glm::vec2 mouseOffset = state.Mouse.getOffsetPosition();
    angularSpeed = -mouseOffset.x * HOR_ANGLE_SPEED;
    pitchSpeed = -mouseOffset.y * VERT_ANGLE_SPEED;

    _moveComp->setForwardSpeed(forwardSpeed);
    _moveComp->setStrafeSpeed(strafSpeed);
    _moveComp->setHorAngularSpeed(angularSpeed);
    _moveComp->setVertAngularSpeed(pitchSpeed);
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
    // formula from euler angle on three axis
//    glm::vec3 lookAtDir{
//            glm::sin(glm::radians(camYawAngle)) * glm::cos(glm::radians(camPitchAngle)),
//            glm::sin(glm::radians(camPitchAngle)),
//            glm::cos(glm::radians(camYawAngle)) * glm::cos(glm::radians(camPitchAngle)),
//    };
    glm::vec3 lookAtDir = glm::normalize(getForward());

    // use cross product to determine cam base axis
    glm::vec3 right = glm::normalize(glm::cross(upVec, lookAtDir));
    glm::vec3 camUp = glm::normalize(glm::cross(lookAtDir, right));
    glm::mat4 camMatrix{
            glm::vec4{right.x, camUp.x, lookAtDir.x, 0},
            glm::vec4{right.y, camUp.y, lookAtDir.y, 0},
            glm::vec4{right.z, camUp.z, lookAtDir.z, 0},
            glm::vec4{-glm::dot(getPosition(), right), -glm::dot(getPosition(), camUp), -glm::dot(getPosition(), lookAtDir), 1},
    };  // construct new axis, where 4th arg is the translation
    // REMEMBER translation is the projection of the lenght onto the NEW axis, so
    // you can't just straight up use position.x!! Must be projected into the new axis

    return camMatrix;
}

glm::mat4 CameraActor::getPerspectiveTransformMatrix() {
    // 1. Transform to camera space and rotation
    glm::mat4 view = getCamViewTransform();

    // TODO: refactor
    constexpr float nearDepth = 1;
    constexpr float viewDepth = 100;
    constexpr float viewWidth = 2000;
    constexpr float viewHeight = 900;
    constexpr float fovYInAngle = 60;

    // 2. use similar triangle to transform to near plane
    // Map z through a non-linear transformation but preserving depth ordering
    // solve m1 * z + m2 = z ^ 2, where it is true when z = n, z = f
    // y' = y * near / z, to represent division we use homogeneous component
    // note that glm multiplies using column matrices!
    glm::mat4 projection{0};
    projection[0][0] = float(nearDepth);
    projection[1][1] = float(nearDepth);
    projection[2][2] = float(nearDepth) + float(viewDepth);
    projection[3][2] = -(float(nearDepth) * float(viewDepth));
    projection[2][3] = 1;
    projection[3][3] = 0;

    // 3. scale both side, notice z is scaled depth
    // tan(delta / 2) = newH/2 / near
//    getEngine()->getRenderer()->
    float aspectRatio = float(viewWidth) / float(viewHeight);
    float nearHeight = glm::tan(glm::radians(fovYInAngle / 2)) * float(nearDepth) * 2;
    float nearWidth = nearHeight * aspectRatio;
    glm::mat4 sca(1);
    sca[0][0] = 2.0f / float(nearWidth);
    sca[1][1] = -2.0f / float(nearHeight);  // inverse to clip space y
    sca[2][2] = 1.0f / float(viewDepth);
    projection = sca * projection;

    return projection * view;
}