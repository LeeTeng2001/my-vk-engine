#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <SDL_keycode.h>

constexpr float movSpeed = 100;

class Camera {
private:
    // General properties
    glm::vec3 position{0, -3, -10};  // center of camera
    glm::vec3 lookAt{0, -1, 0};
    int viewWidth;
    int viewHeight;
    int viewDepth;

    // perspective params
    float fov{};  // angle between the upper and lower sides of the viewing frustum.
    int nearDepth{};

    bool isPerspective;

public:
    // Orthographic camera
    Camera(int viewWidth, int viewHeight, int viewDepth, bool isPerspective) : viewWidth(viewWidth),
                                                                               viewHeight(viewHeight),
                                                                               viewDepth(viewDepth),
                                                                               isPerspective(isPerspective) {}

    void SetPerspective(float fov, int nearPlane) {
        this->fov = fov;
        this->nearDepth = nearPlane;
    }

    void Set(glm::vec3 pos, glm::vec3 newLookAt) {
        position = pos;
        lookAt = newLookAt;
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

    glm::mat4 GetCamTransform() {
        // transform to camera space
        // Build look at matrix (combine new basis axis and translation)
        // The rotation is inverse of R = transpose of R
        glm::vec3 upVec(0, -1, 0);
        glm::vec3 lookAtDir = glm::normalize(lookAt - position);
        glm::vec3 right = glm::normalize(glm::cross(lookAtDir, upVec));
        glm::vec3 camUp = glm::normalize(glm::cross(right, lookAtDir));
        glm::mat4 camMatrix{
                {},
                {},
                {},
                {},
        };  // construct new axis, where 4th arg is the translation
        camMatrix[0] = glm::vec4(right, -position.x);
        camMatrix[1] = glm::vec4(camUp, -position.y);
        camMatrix[2] = glm::vec4(lookAtDir, -position.z);

        return camMatrix;
    }

    glm::mat4 GetPerspectiveTransformMatrix(bool useGlm = true) {
        if (useGlm) {  // use glm implementation to calculate transform
            // remember glm uses a column based matrix
            glm::mat4 view = glm::translate(glm::mat4(1.f), position);
            glm::mat4 projection = glm::perspective(glm::radians(fov), float(viewWidth) / float(viewHeight), float(nearDepth), float(viewDepth));
            projection[1][1] *= -1;  // invert y
            return projection * view;
        } else {
            glm::mat4 res(1);

            // 1. transform to origin
            res[3][0] = position.x;
            res[3][1] = position.y;
            res[3][2] = position.z;

            // 2. use similar triangle to transform to near plane
            // Map z through a non-linear transformation but preserving depth ordering
            // solve m1 * z + m2 = z ^ 2, where it is true when z = n, z = f
            // y' = y * near / z, to represent division we use homogeneous component
            glm::mat4 s(1);
            s[0][0] = float(nearDepth);
            s[1][1] = float(nearDepth);
            s[2][2] = float(nearDepth) + float(viewDepth);
            s[3][2] = -(float(nearDepth) * float(viewDepth));
            s[2][3] = 1;
            s[3][3] = 0;
            res = s * res;

            // 3. scale both side, notice z is untouched
            // tan(delta / 2) = newH/2 / near
            float aspectRatio = float(viewWidth) / float(viewHeight);
            float nearHeight = glm::tan(glm::radians(fov / 2)) * float(nearDepth) * 2;
            float nearWidth = nearHeight * aspectRatio;
            glm::mat4 sca(1);
            sca[0][0] = 2 / nearWidth;
            sca[1][1] = 2 / nearHeight;
            sca[2][2] = 1 / float(viewDepth);
            res = sca * res;

            return res;
        }
    }

    glm::mat4 GetOrthographicTransformMatrix(bool useGlm = true) {
        if (useGlm) {
            // we don't need to invert y manually after calculation because we're passing in inverted y directly
            glm::mat4 view = glm::translate(glm::mat4(1.f), position);
            glm::mat4 projection = glm::ortho(-float(viewWidth)/2, float(viewWidth)/2, float(viewWidth)/2, -float(viewWidth)/2, float(nearDepth), float(viewDepth));
            return projection * view;
        } else {
            glm::mat4 res(1);
            res = GetCamTransform() * res;

            // fit into canonical view box
            // last one is 1/d because vulkan uses [0, 1] instead of [-1, 1] depth
            glm::mat4 sca(1);
            sca[0][0] = 2 / float(viewWidth);
            sca[1][1] = 2 / float(viewHeight);
            sca[2][2] = 1 / float(viewDepth);
            res = sca * res;

            return res;
        }
    }
};
