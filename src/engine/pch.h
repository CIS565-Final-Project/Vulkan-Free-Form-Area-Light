#pragma once

#include <assert.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

#include <glm.hpp>

#define uPtr std::unique_ptr
#define sPtr std::shared_ptr;
#define wPtr std::weak_ptr;

#define mkU std::make_unique
#define mkS std::make_shared

#ifdef GLOBAL_USE_DOUBLE 
	#define Float double
	#define TINYOBJLOADER_USE_DOUBLE
#else
	#define Float float
#endif

#define DeclareWithGetFunc(accessible, type, prefix, name, ...) accessible:\
																	type prefix##_##name;\
																public: \
																	inline type __VA_ARGS__ & Get##name##() __VA_ARGS__ { return prefix##_##name; }\

#define DeclarePtrWithGetFunc(accessible, ptrType, type, prefix, name, ...) accessible:\
																	ptrType<type> prefix##_##name;\
																public: \
																	inline type __VA_ARGS__ * Get##name##() __VA_ARGS__ { return prefix##_##name.get(); }\

namespace VK_Renderer
{
	inline std::vector<char> ReadFile(const std::string& file)
	{
		std::ifstream in(file, std::ios::ate | std::ios::binary);
		std::vector<char> buffer;
		if (!in.is_open())
		{
			printf("Failed to Open %s", file.c_str());
			return buffer;
		}

		size_t file_size = in.tellg();
		in.seekg(0);
		buffer.resize(file_size);
		in.read(buffer.data(), file_size);

		return buffer;
	}
}