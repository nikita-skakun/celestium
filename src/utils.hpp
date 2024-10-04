#pragma once
#include "const.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <magic_enum_flags.hpp>
#include <magic_enum.hpp>
#include <memory>
#include <random>
#include <string>

using namespace magic_enum::bitwise_operators;

// Vector2 operator overloads
constexpr Vector2 operator+(const Vector2 &a, const Vector2 &b) noexcept
{
    return Vector2(a.x + b.x, a.y + b.y);
}

constexpr Vector2 operator-(const Vector2 &a, const Vector2 &b) noexcept
{
    return Vector2(a.x - b.x, a.y - b.y);
}

constexpr Vector2 &operator+=(Vector2 &a, const Vector2 &b) noexcept
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

constexpr Vector2 &operator-=(Vector2 &a, const Vector2 &b) noexcept
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

constexpr Vector2 operator*(const Vector2 &a, float b) noexcept
{
    return Vector2(a.x * b, a.y * b);
}

constexpr Vector2 operator/(const Vector2 &a, float b) noexcept
{
    return Vector2(a.x / b, a.y / b);
}

constexpr Vector2 &operator*=(Vector2 &a, float b) noexcept
{
    a.x *= b;
    a.y *= b;
    return a;
}
constexpr Vector2 &operator/=(Vector2 &a, float b) noexcept
{
    a.x /= b;
    a.y /= b;
    return a;
}

constexpr bool operator==(const Vector2 &a, const Vector2 &b) noexcept
{
    return a.x == b.x && a.y == b.y;
}

// Utility functions for Vector2
struct Vector2Hash
{
    constexpr std::size_t operator()(const Vector2 &v) const noexcept
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

constexpr Vector2 Vector2Normalize(const Vector2 &a) noexcept
{
    float length = std::sqrt(a.x * a.x + a.y * a.y);
    if (length == 0)
    {
        return a;
    }
    return a / length;
}

constexpr float Vector2LengthSq(const Vector2 &a) noexcept
{
    return a.x * a.x + a.y * a.y;
}

constexpr float Vector2Distance(const Vector2 &a, const Vector2 &b) noexcept
{
    return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

constexpr float Vector2DistanceSq(const Vector2 &a, const Vector2 &b) noexcept
{
    return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
}

constexpr bool IsVector2WithinBounds(const Vector2 &a, const Vector2 &boxStart, const Vector2 &boxEnd) noexcept
{
    return (a.x >= std::min(boxStart.x, boxEnd.x) && a.x <= std::max(boxStart.x, boxEnd.x) && a.y >= std::min(boxStart.y, boxEnd.y) && a.y <= std::max(boxStart.y, boxEnd.y));
}

constexpr Vector2 Vector2Round(const Vector2 &a) noexcept
{
    return Vector2(round(a.x), round(a.y));
}

constexpr Vector2 Vector2Floor(const Vector2 &a) noexcept
{
    return Vector2(floor(a.x), floor(a.y));
}

constexpr Vector2 Vector2Lerp(const Vector2 &a, const Vector2 &b, float i) noexcept
{
    return Vector2(a.x + (b.x - a.x) * i, a.y + (b.y - a.y) * i);
}

constexpr Vector2 Vector2Cap(const Vector2 &a, const Vector2 &b, float delta) noexcept
{
    if (Vector2DistanceSq(a, b) < delta * delta)
        return b;
    else
        return a + Vector2Normalize(b - a) * delta;
}

inline int Vector2ToRandomInt(const Vector2 &a, int min, int max) noexcept
{
    Vector2Hash vectorHash;
    std::size_t seed = vectorHash(a);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

constexpr std::string ToString(const Vector2 &a) noexcept
{
    return std::format("({:.2f}, {:.2f})", a.x, a.y);
}

// Utility functions for Vector2Int
struct Vector2Int
{
    int x, y;

    constexpr Vector2Int() : x(0), y(0) {}
    constexpr Vector2Int(int x, int y) : x(x), y(y) {}

    constexpr Vector2Int operator+(const Vector2Int &a) const noexcept
    {
        return Vector2Int(x + a.x, y + a.y);
    }

    constexpr Vector2Int operator-(const Vector2Int &a) const noexcept
    {
        return Vector2Int(x - a.x, y - a.y);
    }

    constexpr void operator+=(const Vector2Int &a) noexcept
    {
        x += a.x;
        y += a.y;
    }

    constexpr void operator-=(const Vector2Int &a) noexcept
    {
        x -= a.x;
        y -= a.y;
    }

    constexpr bool operator==(const Vector2Int &a) const noexcept
    {
        return x == a.x && y == a.y;
    }

    struct Hash
    {
        constexpr std::size_t operator()(const Vector2Int &v) const noexcept
        {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };
};

constexpr float Vector2IntDistanceSq(const Vector2Int &a, const Vector2Int &b) noexcept
{
    return float(b.x - a.x) * float(b.x - a.x) + float(b.y - a.y) * float(b.y - a.y);
}

inline int Vector2IntToRandomInt(const Vector2Int &a, int min, int max) noexcept
{
    Vector2Int::Hash hash;
    std::size_t seed = hash(a);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

constexpr Vector2Int ToVector2Int(const Vector2 &a) noexcept
{
    return Vector2Int((int)std::floor(a.x), (int)std::floor(a.y));
}

constexpr Vector2 ToVector2(const Vector2Int &a) noexcept
{
    return Vector2((float)(a.x), (float)(a.y));
}

constexpr std::string ToString(const Vector2Int &a) noexcept
{
    return std::format("({:}, {:})", a.x, a.y);
}

// Rectangle operator overloads
constexpr Rectangle operator*(const Rectangle &a, float b) noexcept
{
    return {a.x * b, a.y * b, a.width * b, a.height * b};
}

constexpr Rectangle &operator*=(Rectangle &a, float b) noexcept
{
    a.x *= b;
    a.y *= b;
    a.width *= b;
    a.height *= b;
    return a;
}

// Utility functions for Rectangle
constexpr Rectangle Vector2ToRect(const Vector2 &a, const Vector2 &b) noexcept
{
    float startX = std::min(a.x, b.x);
    float startY = std::min(a.y, b.y);
    return Rectangle(startX, startY, std::max(a.x, b.x) - startX, std::max(a.y, b.y) - startY);
}

// Utility functions for Line
constexpr float DistanceSqFromPointToLine(const Vector2 &a, const Vector2 &b, const Vector2 &p) noexcept
{
    Vector2 ab = b - a; // Vector from A to B
    Vector2 ap = p - a; // Vector from A to P

    float abLengthSquared = Vector2LengthSq(ab);

    // Project point P onto line AB, computing parameterized position t
    float t = (ap.x * ab.x + ap.y * ab.y) / abLengthSquared;

    // Clamp t to the range [0, 1] to ensure the point is on the segment
    t = std::clamp(t, 0.f, 1.f);

    // Compute the closest point on the line segment to P
    Vector2 closestPoint = a + ab * t;

    // Return the distance between the mouse and the closest point on the line segment
    return Vector2DistanceSq(p, closestPoint);
}

// Utility functions for Containers
template <typename Container, typename T>
constexpr bool Contains(const Container &container, const T &value) noexcept
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

// Utility functions for std::string
constexpr std::string StringToTitleCase(const std::string &a) noexcept
{
    std::string result = a;
    bool capitalizeNext = true;

    for (char &c : result)
    {
        if (std::isalpha(c))
        {
            if (capitalizeNext)
            {
                c = std::toupper(c);
                capitalizeNext = false;
            }
            else
                c = std::tolower(c);
        }
        else
            capitalizeNext = true;
    }

    return result;
}

constexpr void StringRemoveSpaces(std::string &s) noexcept
{
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
}