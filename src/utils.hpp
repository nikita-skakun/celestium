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
Vector2 operator+(const Vector2 &a, const Vector2 &b) noexcept;
Vector2 operator-(const Vector2 &a, const Vector2 &b) noexcept;
Vector2 &operator+=(Vector2 &a, const Vector2 &b) noexcept;
Vector2 &operator-=(Vector2 &a, const Vector2 &b) noexcept;
Vector2 operator*(const Vector2 &a, float b) noexcept;
Vector2 operator/(const Vector2 &a, float b) noexcept;
Vector2 &operator*=(Vector2 &a, float b) noexcept;
Vector2 &operator/=(Vector2 &a, float b) noexcept;
bool operator==(const Vector2 &a, const Vector2 &b) noexcept;

// Utility functions for Vector2
struct Vector2Hash
{
    std::size_t operator()(const Vector2 &v) const noexcept;
};

Vector2 Vector2Normalize(const Vector2 &a) noexcept;
float Vector2LengthSq(const Vector2 &a) noexcept;
float Vector2Distance(const Vector2 &a, const Vector2 &b) noexcept;
float Vector2DistanceSq(const Vector2 &a, const Vector2 &b) noexcept;
float Vector2Manhattan(const Vector2 &a, const Vector2 &b) noexcept;
Vector2 Vector2Round(const Vector2 &a) noexcept;
Vector2 Vector2Floor(const Vector2 &a) noexcept;
Vector2 Vector2Lerp(const Vector2 &a, const Vector2 &b, float i) noexcept;
Vector2 Vector2Cap(const Vector2 &a, const Vector2 &b, float delta) noexcept;
Vector2 Vector2ScreenScale(const Vector2 &a) noexcept;
int Vector2ToRandomInt(const Vector2 &a, int min, int max) noexcept;
std::string ToString(const Vector2 &a, int precision = 2) noexcept;

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

float Vector2IntDistanceSq(const Vector2Int &a, const Vector2Int &b) noexcept;
bool Vector2IntTouching(const Vector2Int &a, const Vector2Int &b) noexcept;
int Vector2IntManhattan(const Vector2Int &a, const Vector2Int &b) noexcept;
int Vector2IntChebyshev(const Vector2Int &a, const Vector2Int &b) noexcept;
int Vector2IntToRandomInt(const Vector2Int &a, int min, int max) noexcept;
Vector2Int ToVector2Int(const Vector2 &a) noexcept;
Vector2 ToVector2(const Vector2Int &a) noexcept;
Vector2 GetScreenSize();
std::string ToString(const Vector2Int &a) noexcept;

// Rectangle operator overloads
Rectangle operator*(const Rectangle &a, float b) noexcept;
Rectangle operator*(const Rectangle &a, const Vector2 &b) noexcept;
Rectangle &operator*=(Rectangle &a, float b) noexcept;
Rectangle &operator*=(Rectangle &a, const Vector2 &b) noexcept;
bool operator==(const Rectangle &a, const Rectangle &b) noexcept;

// Utility functions for Rectangle
Rectangle Vector2ToBoundingBox(const Vector2 &a, const Vector2 &b) noexcept;
Rectangle Vector2ToRect(const Vector2 &pos, const Vector2 &size) noexcept;
Vector2 RectToPos(const Rectangle &rect) noexcept;
Vector2 RectToSize(const Rectangle &rect) noexcept;
bool IsVector2WithinRect(const Rectangle &rect, const Vector2 &point) noexcept;
std::string ToString(const Rectangle &rect, int precision = 2) noexcept;

// Utility functions for Line
float DistanceSqFromPointToLine(const Vector2 &a, const Vector2 &b, const Vector2 &p) noexcept;

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

inline std::string StringToMacroCase(const std::string &s) noexcept
{
    std::string out;
    out.reserve(s.size());

    auto is_alnum = [](char c)
    { return std::isalnum(static_cast<unsigned char>(c)); };

    char prev = '\0';
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (c == '_' || !is_alnum(c))
        {
            if (!out.empty() && out.back() != '_')
                out.push_back('_');
            prev = '_';
            continue;
        }

        bool curIsUpper = std::isupper(static_cast<unsigned char>(c));
        bool curIsLower = std::islower(static_cast<unsigned char>(c));
        bool prevIsUpper = prev && std::isupper(static_cast<unsigned char>(prev));
        bool prevIsLower = prev && std::islower(static_cast<unsigned char>(prev));
        bool prevIsDigit = prev && std::isdigit(static_cast<unsigned char>(prev));
        char next = (i + 1 < s.size()) ? s[i + 1] : '\0';
        bool nextIsLower = next && std::islower(static_cast<unsigned char>(next));

        if (!out.empty())
        {
            // Insert separator for transitions like lower->Upper, digit->alpha,
            // or acronym end (UPPER UPPER lower -> UPPER_UPPER_lower)
            if ((prevIsLower && curIsUpper) ||
                (prevIsDigit && (curIsUpper || curIsLower)) ||
                (prevIsUpper && curIsUpper && nextIsLower))
            {
                if (out.back() != '_')
                    out.push_back('_');
            }
        }

        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        prev = c;
    }

    // Trim leading/trailing underscores
    size_t start = 0;
    while (start < out.size() && out[start] == '_')
        ++start;
    size_t end = out.size();
    while (end > start && out[end - 1] == '_')
        --end;

    return (start == 0 && end == out.size()) ? out : out.substr(start, end - start);
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
Color RandomColor();