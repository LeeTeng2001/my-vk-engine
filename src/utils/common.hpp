#pragma once

#include <functional>
#include <stack>
#include <vector>
#include <array>
#include <string>
#include <filesystem>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "log.hpp"

using std::function;
using std::stack;
using std::array;
using std::vector;
using std::string;

namespace fs = std::filesystem;
