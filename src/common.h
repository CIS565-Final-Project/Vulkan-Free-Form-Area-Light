#pragma once

#include <assert.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>

constexpr unsigned int Max_Physical_Device_Count = 2;

#define uPtr std::unique_ptr
#define sPtr std::shared_ptr;
#define wPtr std::weak_ptr;

#define mkU std::make_unique
#define mkS std::make_shared
