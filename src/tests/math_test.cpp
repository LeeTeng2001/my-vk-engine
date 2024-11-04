#include <gtest/gtest.h>

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/string_cast.hpp>

#include "../utils/common.hpp"

// TODO: Fix test

glm::mat4 getPerspectiveProjectionFromFormula(float fovy, float aspect, float near, float far) {
    const float tanHalfFovy = tan(fovy / 2.0f);
    glm::mat4 projectionMatrix = glm::mat4{0};
    projectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovy);
    projectionMatrix[1][1] = 1.0f / (tanHalfFovy);
    projectionMatrix[2][2] = -(far + near) / (far - near);
    projectionMatrix[2][3] = -1.0f;
    projectionMatrix[3][2] = -(2 * far * near) / (far - near);
    return projectionMatrix;
}

// Cross product using left hand rules
TEST(MathBehaviouTestSuite, CrossProductExpectation) {
    struct TestInfo {
            glm::vec3 l;
            glm::vec3 r;
            glm::vec3 expect;
    };

    vector<TestInfo> testList{
        {
            {1, 0, 0},
            {0, 1, 0},
            {0, 0, 1},
        },
        {
            {0, 1, 0},
            {-1, 0, 0},
            {0, 0, 1},
        },
        {
            {0, 0, 1},
            {-1, 0, 0},
            {0, -1, 0},
        },
    };

    for (const auto &item : testList) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(glm::cross(item.l, item.r), item.expect, 0.001f)))
            << "l   : " << glm::to_string(item.l) << std::endl
            << "r   : " << glm::to_string(item.r) << std::endl
            << "exp : " << glm::to_string(item.expect) << std::endl
            << "act : " << glm::to_string(glm::cross(item.l, item.r)) << std::endl;
    }
}

// rotation using quaternion
TEST(MathBehaviouTestSuite, QuaterninoRotation) {
    struct TestInfo {
            glm::vec4 forward;
            glm::quat rot;
            glm::vec4 expect;
    };

    vector<TestInfo> testList{
        {
            {1, 0, 0, 1},
            glm::angleAxis(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)),
            {0, 0, -1, 1},
        },
        {
            {0, 0, 1, 1},
            glm::angleAxis(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)),
            {1, 0, 0, 1},
        },
        {
            {0, 0, 1, 1},
            glm::angleAxis(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f)),
            {0, -1, 0, 1},
        },
    };

    for (const auto &item : testList) {
        EXPECT_TRUE(glm::all(
            glm::epsilonEqual(glm::mat4_cast(item.rot) * item.forward, item.expect, 0.001f)))
            << "for : " << glm::to_string(item.forward) << std::endl
            << "exp : " << glm::to_string(item.expect) << std::endl
            << "act : " << glm::to_string(glm::mat4_cast(item.rot) * item.forward) << std::endl;
    }
}

// TEST(PerspectiveTransformTestSuite, PerspectiveTransformFormula){
//     float fovInRadian = glm::radians(60.f);
//     float aspect = 1920.f / 1080.f;
//     float near = 10;
//     float far = 10000;
//
//     glm::mat4 glmS = glm::perspective(fovInRadian, aspect, near, far);
//     glm::mat4 formulaS = getPerspectiveProjectionFromFormula(fovInRadian,
//     aspect, near, far);
//     // glmS[1][1] *= -1;
//
//     for (int i = 0; i < 4; ++i) {
//         EXPECT_TRUE(glm::all(glm::epsilonEqual(glmS[i], formulaS[i],
//         0.001f)))
//                             << "row : " << i << std::endl
//                             << "glm : " << glm::to_string(glmS) << std::endl
//                             << "form: " << glm::to_string(formulaS) <<
//                             std::endl;
//     }
// }

// TEST(PerspectiveTransformTestSuite, PitchYawToForwardVec){
//     struct testSetup {
//         float yawAngle;
//         float pitchAngle;
//         glm::vec3 forward;
//     };
//     std::vector<testSetup> t{
//             {0, 0, glm::vec3{0, 0, 1}},
//             {0, 45, glm::vec3{0, 0.707107, 0.707107}},
//             {0, -45, glm::vec3{0, -0.707107, 0.707107}},
//             {90, 0, glm::vec3{1, 0, 0}},
//             {180, 0, glm::vec3{0, 0, -1}},
//             {270, 0, glm::vec3{-1, 0, 0}},
//     };
//
//     for (const auto &item: t) {
//         // calculate forward vector from two rotation angle
//         glm::vec3 lookAtDir{
//                 glm::sin(glm::radians(item.yawAngle)) *
//                 glm::cos(glm::radians(item.pitchAngle)),
//                 glm::sin(glm::radians(item.pitchAngle)),
//                 glm::cos(glm::radians(item.yawAngle)) *
//                 glm::cos(glm::radians(item.pitchAngle)),
//         };
//         EXPECT_TRUE(glm::all(glm::epsilonEqual(lookAtDir, item.forward,
//         0.001f)))
//                             << "pitch, yaw : " << item.pitchAngle << ", " <<
//                             item.yawAngle << std::endl
//                             << "exp : " << glm::to_string(item.forward) <<
//                             std::endl
//                             << "res : " << glm::to_string(lookAtDir) <<
//                             std::endl;
//     }
// }

// TEST(PerspectiveTransformTestSuite, PerspectiveTransform){
//     Camera cam{1920, 1080, 1000, true};
//     cam.SetPerspective(60, 10);
//
//     glm::mat4 tGlm = cam.GetPerspectiveTransformMatrix(true);
//     glm::mat4 tHand = cam.GetPerspectiveTransformMatrix(false);
//
//     // TODO: Fix self implementation perspective
//     // ref:
//     https://stackoverflow.com/questions/8115352/glmperspective-explanation
//     for (int i = 0; i < 4; ++i) {
//         EXPECT_TRUE(glm::all(glm::epsilonEqual(tGlm[i], tHand[i], 0.001f)))
//                             << "glm : " << glm::to_string(tGlm) << std::endl
//                             << "mine: " << glm::to_string(tHand) <<
//                             std::endl;
//     }
// }
//
// TEST(PerspectiveTransformTestSuite, OrthoTransform){
//     Camera cam{1920, 1080, 1000, true};
//     cam.SetPerspective(60, 10);
//
//     glm::mat4 tGlm = cam.GetOrthographicTransformMatrix(true);
//     glm::mat4 tHand = cam.GetOrthographicTransformMatrix(false);
//
//     for (int i = 0; i < 4; ++i) {
//         EXPECT_TRUE(glm::all(glm::epsilonEqual(tGlm[i], tHand[i], 0.001f)))
//                             << "glm : " << glm::to_string(tGlm) << std::endl
//                             << "mine: " << glm::to_string(tHand) <<
//                             std::endl;
//     }
// }

// TEST(PerspectiveTransformTestSuite, GLMLookAt){
//     glm::vec3 position{0, 0, -10};
//     glm::mat4 view1 = glm::translate(glm::mat4(1.f), -position);
//     glm::mat4 view2 = glm::lookAt(position, glm::vec3{0, 0, 0}, glm::vec3{0,
//     -1, 0});
//
//     for (int i = 0; i < 4; ++i) {
//         EXPECT_TRUE(glm::all(glm::epsilonEqual(view1[i], view2[i], 0.001f)))
//                             << "row      : " << i << std::endl
//                             << "translate: " << glm::to_string(view1) <<
//                             std::endl
//                             << "lookat   : " << glm::to_string(view2) <<
//                             std::endl;
//     }
// }
