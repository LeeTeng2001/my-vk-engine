#include <gtest/gtest.h>
#include <glm/gtc/epsilon.hpp>
#include "camera.hpp"

// TODO: Fix test

glm::mat4 getPerspectiveProjectionFromFormula(float fovy, float aspect, float near, float far) {
    const float tanHalfFovy = tan(fovy / 2.0f);
    glm::mat4 projectionMatrix = glm::mat4{ 0 };
    projectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovy);
    projectionMatrix[1][1] = 1.0f / (tanHalfFovy);
    projectionMatrix[2][2] = - (far + near) / (far - near);
    projectionMatrix[2][3] = - 1.0f;
    projectionMatrix[3][2] = - (2 * far * near) / (far - near);
    return projectionMatrix;
}

TEST(PerspectiveTransformTestSuite, PerspectiveTransformFormula){
    float fovInRadian = glm::radians(60.f);
    float aspect = 1920.f / 1080.f;
    float near = 10;
    float far = 10000;

    glm::mat4 glmS = glm::perspective(fovInRadian, aspect, near, far);
    glm::mat4 formulaS = getPerspectiveProjectionFromFormula(fovInRadian, aspect, near, far);
    // glmS[1][1] *= -1;

    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(glmS[i], formulaS[i], 0.001f)))
                            << "row : " << i << std::endl
                            << "glm : " << glm::to_string(glmS) << std::endl
                            << "form: " << glm::to_string(formulaS) << std::endl;
    }
}
TEST(PerspectiveTransformTestSuite, PerspectiveTransform){
    Camera cam{1920, 1080, 1000, true};
    cam.SetPerspective(60, 10);

    glm::mat4 tGlm = cam.GetPerspectiveTransformMatrix(true);
    glm::mat4 tHand = cam.GetPerspectiveTransformMatrix(false);

    // TODO: Fix self implementation perspective
    // ref: https://stackoverflow.com/questions/8115352/glmperspective-explanation
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(tGlm[i], tHand[i], 0.001f)))
                            << "glm : " << glm::to_string(tGlm) << std::endl
                            << "mine: " << glm::to_string(tHand) << std::endl;
    }
}

TEST(PerspectiveTransformTestSuite, OrthoTransform){
    Camera cam{1920, 1080, 1000, true};
    cam.SetPerspective(60, 10);

    glm::mat4 tGlm = cam.GetOrthographicTransformMatrix(true);
    glm::mat4 tHand = cam.GetOrthographicTransformMatrix(false);

    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(tGlm[i], tHand[i], 0.001f)))
                            << "glm : " << glm::to_string(tGlm) << std::endl
                            << "mine: " << glm::to_string(tHand) << std::endl;
    }
}

TEST(PerspectiveTransformTestSuite, GLMLookAt){
    glm::vec3 position{0, 0, -10};
    glm::mat4 view1 = glm::translate(glm::mat4(1.f), -position);
    glm::mat4 view2 = glm::lookAt(position, glm::vec3{0, 0, 0}, glm::vec3{0, -1, 0});

    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(view1[i], view2[i], 0.001f)))
                            << "row      : " << i << std::endl
                            << "translate: " << glm::to_string(view1) << std::endl
                            << "lookat   : " << glm::to_string(view2) << std::endl;
    }
}
