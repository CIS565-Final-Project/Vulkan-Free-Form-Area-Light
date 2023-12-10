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
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <limits>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <format>
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

#define DeclareWithGetSetFunc(accessible, type, prefix, name, ...) accessible:\
																	type prefix##_##name;\
																public: \
																	inline void Set##name##(type const&& value) { prefix##_##name = value; }\
																	inline type __VA_ARGS__ & Get##name##() __VA_ARGS__ { return prefix##_##name; }\

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

	template <typename T>
	std::vector<T> RemoveDuplicates(std::vector<T> const& datas) {
		std::unordered_set<T> seen;
		std::vector<T> result;
		result.reserve(datas.size());
		for (const auto& value : datas) {
			if (seen.insert(value).second) {
				result.push_back(value);
			}
		}

		return result;
	}
}