#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

constexpr float movSpeed = 50;

class Camera {
private:
    // General properties
    glm::vec3 position{0, 0, 0};  // center of camera
    glm::vec3 lookAt{0, 0, 10};
    int viewWidth;
    int viewHeight;
    int viewDepth;

    // perspective params
    float fov{};
    int nearPlane{};

    bool isPerspective;

public:
    // Orthographic camera
    Camera(int viewWidth, int viewHeight, int viewDepth, bool isPerspective) : viewWidth(viewWidth),
                                                                               viewHeight(viewHeight),
                                                                               viewDepth(viewDepth),
                                                                               isPerspective(isPerspective) {}

    void SetPerspective(float fov, int nearPlane) {
        this->fov = fov;
        this->nearPlane = nearPlane;
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

    glm::mat4 GetTransformMatrix() {
        // TODO: Implement perspective
        glm::mat4 res(1);

        // Build look at matrix (combine new basis axis and translation)
        // TODO: Handle edge case
        glm::vec3 upVec(0, -1, 0);
        glm::vec3 lookAtDir = glm::normalize(lookAt - position);
        glm::vec3 right = glm::normalize(glm::cross(lookAtDir, upVec));
        glm::vec3 camUp = glm::normalize(glm::cross(right, lookAtDir));
        glm::mat4 camMatrix(1);  // construct new axis, where 4th arg is the translation
        camMatrix[0] = glm::vec4(right, -position.x);
        camMatrix[1] = glm::vec4(-camUp, -position.y);
        camMatrix[2] = glm::vec4(lookAtDir, -position.z);
//        SDL_Log("final pos: %s", glm::to_string(camMatrix).c_str());
        res = camMatrix * res;

        // fit into canonical view box
        res = glm::scale(res, glm::vec3(2 / float(viewWidth), 2 / float(viewHeight), 1 / float(viewDepth)));

        return res;
    }
};
