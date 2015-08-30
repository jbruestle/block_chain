
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <tuple>
#include <algorithm>
#include <exception>

using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::enable_shared_from_this;
using std::string;
using std::to_string;
using std::pair;
using std::array;
using std::vector;
using std::map;
using std::unordered_map;
using std::set;
using std::tuple;
using std::max;
using std::min;
using std::swap;
using std::function;
using std::runtime_error;

// Std forgot this
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Simple replacement for boost operators for comparable values
template<typename T>
class comparable 
{
public:
     friend bool operator<=(const T& x, const T& y) { return !(y < x); }
     friend bool operator>=(const T& x, const T& y) { return !(x < y); }
     friend bool operator>(const T& x, const T& y)  { return y < x; }
     friend bool operator!=(const T& x, const T& y)  { return !(x == y); }
};

