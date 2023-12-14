#pragma once

#include <glm/gtx/string_cast.hpp>
#include <SDL_keycode.h>
#include <SDL_events.h>

#include "utils/common.hpp"

constexpr float movSpeed = 3;
constexpr float rotateSpeedAngle = 10;

class Camera {
private:
    // General properties
    glm::vec3 position{0, 0, -5};  // center of camera
    // look into z
    float camPitchAngle = 0; // head updown [-85, 85], ++ up
    float camYawAngle = 0; // head rightleft [0, 360], ++ to the right, clockwise

    // states
    bool shouldMoveCam = false;
    int moveForwardState = 0; // 1 forward, -1 backward
    int moveRightState = 0; // 1 right, -1 left

    int viewWidth;
    int viewHeight;
    int viewDepth;

    // perspective params
    float fovYInAngle{};  // angle between the upper and lower sides of the viewing frustum.
    int nearDepth{};

    // update params

public:
    Camera(int viewWidth, int viewHeight,int nearDepth, int viewDepth, float fov) : viewWidth(viewWidth),
                                                                                    viewHeight(viewHeight),
                                                                                    viewDepth(viewDepth), fovYInAngle(fov),
                                                                                    nearDepth(nearDepth) {}

    [[nodiscard]] glm::vec3 GetCamPos() { return position; }

    void ProcessInputEvent(float delta, const SDL_Event *e) {
        switch (e->type) {
            case SDL_EVENT_KEY_DOWN:
                switch (e->key.keysym.sym) {
                    case SDLK_LSHIFT:
                        shouldMoveCam = true;
                        break;
                    case SDLK_w:
                        moveForwardState = 1;
                        break;
                    case SDLK_s:
                        moveForwardState = -1;
                        break;
                    case SDLK_d:
                        moveRightState = 1;
                        break;
                    case SDLK_a:
                        moveRightState = -1;
                        break;
                }
                break;
            case SDL_EVENT_KEY_UP:
                switch (e->key.keysym.sym) {
                    case SDLK_LSHIFT:
                        shouldMoveCam = false;
                        break;
                    case SDLK_w:
                        moveForwardState = 0;
                        break;
                    case SDLK_s:
                        moveForwardState = 0;
                        break;
                    case SDLK_d:
                        moveRightState =0;
                        break;
                    case SDLK_a:
                        moveRightState = 0;
                        break;
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (!shouldMoveCam) {
                    return;
                }
                camYawAngle = (camYawAngle + e->motion.xrel * rotateSpeedAngle * delta);
                // SDL uses top down y coord, so minus it
                camPitchAngle = (camPitchAngle - e->motion.yrel * rotateSpeedAngle * delta);
                camPitchAngle = glm::clamp(camPitchAngle, -85.0f, 85.0f);
                while (camYawAngle > 360) {
                    camYawAngle -= 360;
                }
                while (camYawAngle < 0) {
                    camYawAngle += 360;
                }
                break;
        }

        // update cam position
        if (shouldMoveCam) {
            glm::vec3 lookAtDir{
                    glm::sin(glm::radians(camYawAngle)) * glm::cos(glm::radians(camPitchAngle)),
                    glm::sin(glm::radians(camPitchAngle)),
                    glm::cos(glm::radians(camYawAngle)) * glm::cos(glm::radians(camPitchAngle)),
            };
            if (moveForwardState != 0) {
                position += lookAtDir * delta * movSpeed * float(moveForwardState);
            }
            if (moveRightState != 0) {
                glm::vec3 r = -glm::cross(lookAtDir, glm::vec3{0, 1, 0});
                position += r * delta * movSpeed * float(moveRightState);
            }
        }
    }

    // our world coordinate is x right, y up, z in
    // get the object in camera space
    glm::mat4 GetCamViewTransform() {
        // transform to camera space
        // Build look at matrix (combine new basis axis and translation)
        // The rotation is inverse of R = transpose of R
        // carefully study the cross product axis! thus we need to have -lookAtDir
        glm::vec3 upVec(0, 1, 0);

        // calculate forward vector from two rotation angle, already normalised
        glm::vec3 lookAtDir{
                glm::sin(glm::radians(camYawAngle)) * glm::cos(glm::radians(camPitchAngle)),
                glm::sin(glm::radians(camPitchAngle)),
                glm::cos(glm::radians(camYawAngle)) * glm::cos(glm::radians(camPitchAngle)),
        };

        // use cross product to determine cam base axis
        glm::vec3 right = glm::normalize(glm::cross(-lookAtDir, upVec));
        glm::vec3 camUp = glm::normalize(glm::cross(right, -lookAtDir));
        glm::mat4 camMatrix{
                glm::vec4{right.x, camUp.x, lookAtDir.x, 0},
                glm::vec4{right.y, camUp.y, lookAtDir.y, 0},
                glm::vec4{right.z, camUp.z, lookAtDir.z, 0},
                glm::vec4{-glm::dot(position, right), -glm::dot(position, camUp), -glm::dot(position, lookAtDir), 1},
        };  // construct new axis, where 4th arg is the translation
        // REMEMBER translation is the projection of the lenght onto the NEW axis, so
        // you can't just straight up use position.x!! Must be projected into the new axis

        return camMatrix;
    }

    glm::mat4 GetPerspectiveTransformMatrix() {
        // 1. Transform to camera space and rotation
        glm::mat4 view = GetCamViewTransform();

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
        float aspectRatio = float(viewWidth) / float(viewHeight);
        float nearHeight = glm::tan(glm::radians(fovYInAngle / 2)) * float(nearDepth) * 2;
        float nearWidth = nearHeight * aspectRatio;
        glm::mat4 sca(1);
        sca[0][0] = 2.0f / float(nearWidth);
        sca[1][1] = -2.0f / float(nearHeight);  // inverse to clip space y
        sca[2][2] = 1.0f / float(viewDepth);
        projection = sca * projection;

        // Debug
//            vector<glm::vec4> pointList = {
//                    {2, 2, -2, 1},
//                    {-2, 2, -2, 1},
//                    {2, -2, -2, 1},
//            };
//            SDL_Log("view %s", glm::to_string(view).c_str());
//            for (auto mockPoint: pointList) {
//                SDL_Log("p %s", glm::to_string(mockPoint).c_str());
//                mockPoint = view * mockPoint;
//                SDL_Log("p *view %s", glm::to_string(mockPoint).c_str());
//                mockPoint = projection * mockPoint;
//                SDL_Log("p *proj %s", glm::to_string(mockPoint).c_str());
//                mockPoint /= mockPoint.w;
//                SDL_Log("p *scale %s", glm::to_string(mockPoint).c_str());
//            }

        return projection * view;
    }

    glm::mat4 GetOrthographicTransformMatrix() {
        // 1. Transform to camera space and rotation
        glm::mat4 view = GetCamViewTransform();

        // 2. fit into canonical view box by scaling
        // last one is 1/d because vulkan uses [0, 1] instead of [-1, 1] depth
        glm::mat4 projection(1);
        projection[0][0] = 2.0f / float(viewWidth);
        projection[1][1] = -2.0f / float(viewHeight);  // inverse to clip space y
        projection[2][2] = 1.0f / float(viewDepth);

        return projection * view;
    }
};
