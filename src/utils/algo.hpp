#pragma once

#include "common.hpp"
#include <glm/gtx/string_cast.hpp>

namespace luna {
class HelperAlgo {
    public:
        std::array<int, 3> static getAxisOrder(const glm::vec3 &upAxis) {
            // TODO: implement rest
            if (upAxis == glm::vec3{0, 1, 0}) {
                return {0, 1, 2};
            } else if (upAxis == glm::vec3{0, 0, 1}) {
                return {1, 2, 0};
            } else {
                auto l = SLog::get();
                l->warn(fmt::format("unrecognised axis order {:s}, falling back to default",
                                    glm::to_string(upAxis)));
                return {0, 1, 2};
            }
        }
};
}  // namespace luna