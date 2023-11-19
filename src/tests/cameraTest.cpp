#include <gtest/gtest.h>
#include <glm/gtc/epsilon.hpp>
#include "camera.hpp"


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

//TEST(AbsoluteDateTestSuite, IncorrectDate){ // 12/0/2020 -> 0
//    GregorianDate gregDate;
//    gregDate.SetMonth(12);
//    gregDate.SetDay(0);
//    gregDate.SetYear(2020);
//
//    ASSERT_EQ(gregDate.getAbsoluteDate(),0);
//}