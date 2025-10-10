#pragma once
#include "const.hpp"
#include <algorithm>
#include <format>
#include <magic_enum/magic_enum_flags.hpp>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <random>

using namespace magic_enum::bitwise_operators;

// Utility functions for numbers

/**
 * @brief Computes an evenly spaced index based on a given value and index.
 *
 * @param value A double value used to compute the index.
 * @param index An integer index used in the computation.
 * @return An integer representing the evenly spaced index.
 */
constexpr int GetEvenlySpacedIndex(double value, int index) noexcept
{
    return static_cast<int>((value - std::floor(value)) * index);
}

/**
 * @brief Generates a random integer within a specified range.
 *
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return A random integer between min and max, inclusive.
 */
inline int RandomIntWithRange(int min, int max) noexcept
{
    if (max <= min)
        return min;
    thread_local static std::random_device rd; // TODO: Switch to shared seed for multiplayer
    thread_local static std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

/**
 * @brief Oscillates a value between 0 and the specified length.
 *
 * @param time The current time or value to oscillate.
 * @param length The length of the oscillation period.
 * @return The oscillated value.
 */
constexpr double Oscillate(double time, double length) noexcept
{
    return std::fabs(std::fmod(time, length * 2.) - length);
}

/**
 * @brief Clamps a value between a minimum and maximum value.
 *
 * @param value The value to clamp.
 * @param min The minimum value.
 * @param max The maximum value.
 * @return The clamped value.
 */
template <typename T>
constexpr int Floor(T value) noexcept
{
    return static_cast<int>(value) - ((value >= 0) ? 0 : 1);
}

/**
 * @brief Determines if an event occurs based on a chance per second and delta time.
 *
 * @param chancePerSecond The probability of the event occurring per second.
 * @param deltaTime The time elapsed since the last check, in seconds.
 * @return true if the event occurs, false otherwise.
 */
inline bool CheckIfEventHappens(double chancePerSecond, double deltaTime) noexcept
{
    thread_local static std::random_device rd; // TODO: Switch to shared seed for multiplayer
    thread_local static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double expectedEvents = chancePerSecond * deltaTime;

    if (expectedEvents >= 1.0)
        return true;

    return dis(gen) < expectedEvents;
}

/**
 * @brief Converts a float value to a string with a specified precision.
 *
 * @param value The float value to convert.
 * @param precision The number of decimal places to include in the string.
 * @return A string representation of the float value.
 */
constexpr std::string ToString(float value, int precision = 2) noexcept
{
    return std::format("{:.{}f}", value, precision);
}

// Utility functions for booleans

/**
 * @brief Sets a bit in a value to a specified state.
 *
 * @tparam T The type of the value.
 * @param value The value to modify.
 * @param bitState The state to set the bit to.
 * @param mask The mask of the bit to set.
 */
template <typename T>
constexpr void SetBit(T &value, bool bitState, T mask) noexcept
{
    if (bitState)
        value |= mask;
    else
        value &= ~mask;
}

/**
 * @brief Toggles a bit in a value.
 *
 * @tparam T The type of the value.
 * @param value The value to modify.
 * @param mask The mask of the bit to toggle.
 */
template <typename T>
constexpr void ToggleBit(T &value, T mask) noexcept
{
    value ^= mask;
}

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
    float lengthSq = a.x * a.x + a.y * a.y;
    if (lengthSq == 0)
    {
        return a;
    }
    return a / std::sqrt(lengthSq);
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

constexpr Vector2 Vector2ScreenScale(const Vector2 &a) noexcept
{
    return Vector2(a.x / 1920., a.y / 1080.);
}

inline int Vector2ToRandomInt(const Vector2 &a, int min, int max) noexcept
{
    Vector2Hash vectorHash;
    std::size_t seed = vectorHash(a);
    thread_local static std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

constexpr std::string ToString(const Vector2 &a, int precision = 2) noexcept
{
    return "(" + ToString(a.x, precision) + ", " + ToString(a.y, precision) + ")";
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
};

namespace std
{
    template <>
    struct hash<Vector2Int>
    {
        constexpr std::size_t operator()(const Vector2Int &v) const noexcept
        {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };
}

constexpr float Vector2IntDistanceSq(const Vector2Int &a, const Vector2Int &b) noexcept
{
    return float(b.x - a.x) * float(b.x - a.x) + float(b.y - a.y) * float(b.y - a.y);
}

constexpr bool Vector2IntTouching(const Vector2Int &a, const Vector2Int &b) noexcept
{
    return (a.x == b.x && std::abs(a.y - b.y) == 1) || (a.y == b.y && std::abs(a.x - b.x) == 1);
}

inline int Vector2IntToRandomInt(const Vector2Int &a, int min, int max) noexcept
{
    std::hash<Vector2Int> hash;
    std::size_t seed = hash(a);
    thread_local static std::mt19937 generator(seed);
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

inline Vector2 GetScreenSize()
{
    int monitor = GetCurrentMonitor();
    return Vector2((float)GetMonitorWidth(monitor), (float)GetMonitorHeight(monitor));
}

constexpr std::string ToString(const Vector2Int &a) noexcept
{
    return std::format("({}, {})", a.x, a.y);
}

// Rectangle operator overloads
constexpr Rectangle operator*(const Rectangle &a, float b) noexcept
{
    return {a.x * b, a.y * b, a.width * b, a.height * b};
}

constexpr Rectangle operator*(const Rectangle &a, const Vector2 &b) noexcept
{
    return {a.x * b.x, a.y * b.y, a.width * b.x, a.height * b.y};
}

constexpr Rectangle &operator*=(Rectangle &a, float b) noexcept
{
    a.x *= b;
    a.y *= b;
    a.width *= b;
    a.height *= b;
    return a;
}

constexpr Rectangle &operator*=(Rectangle &a, const Vector2 &b) noexcept
{
    a.x *= b.x;
    a.y *= b.y;
    a.width *= b.x;
    a.height *= b.y;
    return a;
}

constexpr bool operator==(const Rectangle &a, const Rectangle &b) noexcept
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

// Utility functions for Rectangle
constexpr Rectangle Vector2ToBoundingBox(const Vector2 &a, const Vector2 &b) noexcept
{
    float startX = std::min(a.x, b.x);
    float startY = std::min(a.y, b.y);
    return Rectangle(startX, startY, std::max(a.x, b.x) - startX, std::max(a.y, b.y) - startY);
}

constexpr Rectangle Vector2ToRect(const Vector2 &pos, const Vector2 &size) noexcept
{
    return Rectangle(pos.x, pos.y, size.x, size.y);
}

constexpr Vector2 RectToPos(const Rectangle &rect) noexcept
{
    return Vector2(rect.x, rect.y);
}

constexpr Vector2 RectToSize(const Rectangle &rect) noexcept
{
    return Vector2(rect.width, rect.height);
}

constexpr bool IsVector2WithinRect(const Rectangle &rect, const Vector2 &point) noexcept
{
    return (point.x >= rect.x && point.x <= (rect.x + rect.width) &&
            point.y >= rect.y && point.y <= (rect.y + rect.height));
}

constexpr std::string ToString(const Rectangle &rect, int precision = 2) noexcept
{
    return "(" + ToString(rect.x, precision) + ", " + ToString(rect.y, precision) + ", " +
           ToString(rect.width, precision) + ", " + ToString(rect.height, precision) + ")";
}

// Utility functions for Line
constexpr float DistanceSqFromPointToLine(const Vector2 &a, const Vector2 &b, const Vector2 &p) noexcept
{
    Vector2 ab = b - a; // Vector from A to B
    Vector2 ap = p - a; // Vector from A to P

    float abLengthSquared = Vector2LengthSq(ab);

    // Project point P onto line AB, computing parameterized position t
    float t = (ap.x * ab.x + ap.y * ab.y) / abLengthSquared;
    if (t < 0)
        t = 0;
    else if (t > 1)
        t = 1;

    // Compute the closest point on the line segment to P
    Vector2 closestPoint = a + ab * t;

    // Return the distance between the point and the closest point on the line segment
    return Vector2DistanceSq(p, closestPoint);
}

// Utility functions for Containers
template <typename Container, typename T>
constexpr bool Contains(const Container &container, const T &value) noexcept
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

// Overload for containers with find method (like std::map, std::unordered_map, std::set, etc.)
template <typename Container, typename Key>
    requires requires(const Container &c, const Key &k) { c.find(k); }
constexpr std::optional<typename Container::const_iterator> Find(const Container &container, const Key &key) noexcept
{
    auto it = container.find(key);
    if (it != container.end())
    {
        return it;
    }
    return std::nullopt;
}

// Overload for containers without find method (like std::vector, std::list, etc.)
template <typename Container, typename T>
    requires(!requires(const Container &c, const T &k) { c.find(k); })
constexpr std::optional<typename Container::const_iterator> Find(const Container &container, const T &value) noexcept
{
    auto it = std::find(container.begin(), container.end(), value);
    if (it != container.end())
    {
        return it;
    }
    return std::nullopt;
}

// Utility functions for std::string
constexpr std::string StringToTitleCase(const std::string &a) noexcept
{
    std::string result;
    result.reserve(a.size());
    bool capitalizeNext = true;

    for (char c : a)
    {
        if (std::isalpha(c))
        {
            if (capitalizeNext)
            {
                result += std::toupper(c);
                capitalizeNext = false;
            }
            else
            {
                result += std::tolower(c);
            }
        }
        else
        {
            result += c;
            capitalizeNext = true;
        }
    }

    return result;
}

constexpr void StringRemoveSpaces(std::string &s) noexcept
{
    std::erase_if(s, ::isspace);
}

constexpr std::string MacroCaseToName(const std::string &s) noexcept
{
    std::string name = s;
    std::replace(name.begin(), name.end(), '_', ' ');
    return StringToTitleCase(name);
}

template <typename T>
constexpr std::string EnumToName(const T &enumValue)
{
    return MacroCaseToName(std::string(magic_enum::enum_name(enumValue)));
}

// Utility functions for Color
inline Color RandomColor()
{
    int h = RandomIntWithRange(0, 359);
    int sInt = RandomIntWithRange(60, 90);
    int vInt = RandomIntWithRange(70, 100);

    float hue = static_cast<float>(h);
    float sat = static_cast<float>(sInt) / 100.f;
    float val = static_cast<float>(vInt) / 100.f;

    float c = val * sat;
    float hh = hue / 60.f;
    float x = c * (1.f - std::fabs(std::fmod(hh, 2.f) - 1.f));
    float r1 = 0, g1 = 0, b1 = 0;

    if (hh < 1.f)
    {
        r1 = c;
        g1 = x;
        b1 = 0;
    }
    else if (hh < 2.f)
    {
        r1 = x;
        g1 = c;
        b1 = 0;
    }
    else if (hh < 3.f)
    {
        r1 = 0;
        g1 = c;
        b1 = x;
    }
    else if (hh < 4.f)
    {
        r1 = 0;
        g1 = x;
        b1 = c;
    }
    else if (hh < 5.f)
    {
        r1 = x;
        g1 = 0;
        b1 = c;
    }
    else
    {
        r1 = c;
        g1 = 0;
        b1 = x;
    }

    float m = val - c;
    unsigned char r = static_cast<unsigned char>(std::clamp((r1 + m) * 255.f, 0.f, 255.f));
    unsigned char g = static_cast<unsigned char>(std::clamp((g1 + m) * 255.f, 0.f, 255.f));
    unsigned char b = static_cast<unsigned char>(std::clamp((b1 + m) * 255.f, 0.f, 255.f));

    return Color{r, g, b, 255};
}