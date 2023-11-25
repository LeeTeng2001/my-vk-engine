#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>
#include <SDL_keycode.h>

constexpr float movSpeed = 100;

class Camera {
private:
    // General properties
    glm::vec3 position{0, 2, -10};  // center of camera
//    glm::quat rotation{};           // rotation in quaternion
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

    void SetCameraPosLookAt(glm::vec3 pos, glm::vec3 newLookAt) {
        position = pos;
        // Do the math to calculation quatenium
//        lookAt = newLookAt;
    }

    void HandleKey(float delta, SDL_Keycode keyPressed) {
        switch (keyPressed) {
            case SDLK_LEFT:
                position.x -= delta * movSpeed;
                break;
            case SDLK_RIGHT:
                position.x += delta * movSpeed;
                break;
            case SDLK_DOWN:
                position.y += delta * movSpeed;
                break;
            case SDLK_UP:
                position.y -= delta * movSpeed;
                break;
        }
//        SDL_Log("%s, %f", glm::to_string(position).c_str(), delta * movSpeed);
    }

    void HandleMouseMotion(float xRel, float yRel) {

    }

    // our world coordinate is x right, y up, z in
    // get the object in camera space
    glm::mat4 GetCamViewTransform() {
        // transform to camera space
        // Build look at matrix (combine new basis axis and translation)
        // The rotation is inverse of R = transpose of R
        // carefully study the cross product axis! thus we need to have -lookAtDir
        glm::vec3 upVec(0, 1, 0);
        glm::vec3 lookAt(0, 0, 0);
        glm::vec3 lookAtDir = glm::normalize(lookAt - position);
        glm::vec3 right = glm::normalize(glm::cross(-lookAtDir, upVec));
        glm::vec3 camUp = glm::normalize(glm::cross(right, -lookAtDir));
        glm::mat4 camMatrix{
                glm::vec4{right.x, camUp.x, lookAtDir.x, 0},
                glm::vec4{right.y, camUp.y, lookAtDir.y, 0},
                glm::vec4{right.z, camUp.z, lookAtDir.z, 0},
                glm::vec4{-position.x, -position.y, -position.z, 1},
        };  // construct new axis, where 4th arg is the translation

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
