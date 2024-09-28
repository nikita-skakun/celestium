#pragma once
#include "const.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fmt/core.h>
#include <iostream>
#include <magic_enum_flags.hpp>
#include <magic_enum.hpp>
#include <memory>
#include <random>
#include <string>

using namespace magic_enum::bitwise_operators;

// Vector2 operator overloads
constexpr Vector2 operator+(const Vector2 &a, const Vector2 &b)
{
    return Vector2(a.x + b.x, a.y + b.y);
}

constexpr Vector2 operator-(const Vector2 &a, const Vector2 &b)
{
    return Vector2(a.x - b.x, a.y - b.y);
}

constexpr Vector2 &operator+=(Vector2 &a, const Vector2 &b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

constexpr Vector2 &operator-=(Vector2 &a, const Vector2 &b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

constexpr Vector2 operator*(const Vector2 &a, float b)
{
    return Vector2(a.x * b, a.y * b);
}

constexpr Vector2 operator/(const Vector2 &a, float b)
{
    return Vector2(a.x / b, a.y / b);
}

constexpr Vector2 &operator*=(Vector2 &a, float b)
{
    a.x *= b;
    a.y *= b;
    return a;
}
constexpr Vector2 &operator/=(Vector2 &a, float b)
{
    a.x /= b;
    a.y /= b;
    return a;
}

constexpr bool operator==(const Vector2 &a, const Vector2 &b)
{
    return a.x == b.x && a.y == b.y;
}

inline std::ostream &operator<<(std::ostream &os, const Vector2 &a)
{
    os << "{" << a.x << ", " << a.y << "}";
    return os;
}

// Utility functions for Vector2
struct Vector2Hash
{
    inline std::size_t operator()(const Vector2 &v) const
    {
        return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1);
    }
};

struct Vector2Equal
{
    constexpr bool operator()(const Vector2 &a, const Vector2 &b) const noexcept
    {
        return (a.x == b.x && a.y == b.y);
    }
};

constexpr Vector2 Vector2Normalize(const Vector2 &a)
{
    float length = std::sqrt(a.x * a.x + a.y * a.y);
    if (length == 0)
    {
        return a;
    }
    return a / length;
}

constexpr float Vector2Distance(const Vector2 &a, const Vector2 &b)
{
    return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

constexpr float Vector2DistanceSq(const Vector2 &a, const Vector2 &b)
{
    return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
}

constexpr bool IsVector2WithinBounds(const Vector2 &a, const Vector2 &boxStart, const Vector2 &boxEnd)
{
    return (a.x >= std::min(boxStart.x, boxEnd.x) && a.x <= std::max(boxStart.x, boxEnd.x) && a.y >= std::min(boxStart.y, boxEnd.y) && a.y <= std::max(boxStart.y, boxEnd.y));
}

constexpr Vector2 Vector2Round(const Vector2 &a)
{
    return Vector2(round(a.x), round(a.y));
}

constexpr Vector2 Vector2Floor(const Vector2 &a)
{
    return Vector2(floor(a.x), floor(a.y));
}

constexpr Vector2 Vector2Lerp(const Vector2 &a, const Vector2 &b, float i)
{
    return Vector2(a.x + (b.x - a.x) * i, a.y + (b.y - a.y) * i);
}

constexpr Vector2 Vector2Cap(const Vector2 &a, const Vector2 &b, float delta)
{
    if (Vector2DistanceSq(a, b) < delta * delta)
        return b;
    else
        return a + Vector2Normalize(b - a) * delta;
}

inline int Vector2ToRandomInt(const Vector2 &a, int min, int max)
{
    Vector2Hash vectorHash;
    std::size_t seed = vectorHash(a);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

inline std::string ToString(const Vector2 &a)
{
    return fmt::format("({:.2f}, {:.2f})", a.x, a.y);
}

// Utility functions for Vector2Int
struct Vector2Int
{
    int x, y;

    constexpr Vector2Int() : x(0), y(0) {}
    constexpr Vector2Int(int x, int y) : x(x), y(y) {}

    constexpr Vector2Int operator+(const Vector2Int &a) const
    {
        return Vector2Int(x + a.x, y + a.y);
    }

    constexpr Vector2Int operator-(const Vector2Int &a) const
    {
        return Vector2Int(x - a.x, y - a.y);
    }

    constexpr Vector2Int &operator+=(Vector2Int &a)
    {
        x += a.x;
        y += a.y;
        return *this;
    }

    constexpr Vector2Int &operator-=(Vector2Int &a)
    {
        x -= a.x;
        y -= a.y;
        return *this;
    }

    constexpr bool operator==(const Vector2Int &a) const
    {
        return x == a.x && y == a.y;
    }

    struct Hash
    {
        inline std::size_t operator()(const Vector2Int &v) const
        {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };
};

constexpr float Vector2IntDistanceSq(const Vector2Int &a, const Vector2Int &b)
{
    return float(b.x - a.x) * float(b.x - a.x) + float(b.y - a.y) * float(b.y - a.y);
}

inline int Vector2IntToRandomInt(const Vector2Int &a, int min, int max)
{
    Vector2Int::Hash hash;
    std::size_t seed = hash(a);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

constexpr Vector2Int ToVector2Int(const Vector2 &a)
{
    return Vector2Int((int)std::floor(a.x), (int)std::floor(a.y));
}

constexpr Vector2 ToVector2(const Vector2Int &a)
{
    return Vector2((float)(a.x), (float)(a.y));
}

inline std::string ToString(const Vector2Int &a)
{
    return fmt::format("({:}, {:})", a.x, a.y);
}

// Rectangle operator overloads
constexpr Rectangle operator*(const Rectangle &a, float b)
{
    return {a.x * b, a.y * b, a.width * b, a.height * b};
}

constexpr Rectangle &operator*=(Rectangle &a, float b)
{
    a.x *= b;
    a.y *= b;
    a.width *= b;
    a.height *= b;
    return a;
}

// Utility functions for Rectangle
constexpr Rectangle Vector2ToRect(const Vector2 &a, const Vector2 &b)
{
    float startX = std::min(a.x, b.x);
    float startY = std::min(a.y, b.y);
    return Rectangle(startX, startY, std::max(a.x, b.x) - startX, std::max(a.y, b.y) - startY);
}

// Utility functions for Containers
template <typename Container, typename T>
constexpr bool Contains(const Container &container, const T &value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

// Utility functions for std::string
constexpr std::string ToTitleCase(const std::string &a)
{
    std::string result = a;
    bool capitalizeNext = true;

    for (size_t i = 0; i < result.length(); ++i)
    {
        if (std::isalpha(result[i]))
        {
            if (capitalizeNext)
            {
                result[i] = std::toupper(result[i]);
                capitalizeNext = false;
            }
            else
            {
                result[i] = std::tolower(result[i]);
            }
        }
        else
            capitalizeNext = true;
    }

    return result;
}