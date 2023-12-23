#pragma once

#include <utility>
#include <memory>
#include <functional>
#include <stack>
#include <vector>
#include <array>
#include <string>
#include <filesystem>
#define GLM_FORCE_RADIANS
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
using std::make_unique;
using std::make_shared;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

namespace fs = std::filesystem;
