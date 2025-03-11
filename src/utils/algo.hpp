#pragma once

#include "common.hpp"
#include <glm/gtx/string_cast.hpp>
#include <fstream>

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

        static std::vector<char> readFile(const std::string &filename) {
            std::ifstream f(filename, std::ios::ate | std::ios::binary);

            if (!f.is_open()) {
                std::string errMsg = "Failed to open f: " + filename;
                throw std::runtime_error(errMsg);
            }

            size_t fileSize = (size_t)f.tellg();
            std::vector<char> buffer(fileSize);
            f.seekg(0);
            f.read(buffer.data(), fileSize);

            return buffer;
        }
};
}  // namespace luna