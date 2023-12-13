#pragma once

#include <SDL_log.h>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "VkResult false: %s %d \n", __FILE__, __LINE__);     \
		assert(res == VK_SUCCESS);																		\
	}																									\
}
