#pragma once
#include "const.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fmt/core.h>
#include <iostream>
#include <random>
#include <string>

struct Vector2Int;

// Vector2 operator overloads
Vector2 operator+(const Vector2 &a, const Vector2 &b);
Vector2 operator-(const Vector2 &a, const Vector2 &b);
Vector2 &operator+=(Vector2 &a, const Vector2 &b);
Vector2 &operator-=(Vector2 &a, const Vector2 &b);
Vector2 operator*(const Vector2 &a, float b);
Vector2 operator/(const Vector2 &a, float b);
Vector2 &operator*=(Vector2 &a, float b);
Vector2 &operator/=(Vector2 &a, float b);
bool operator==(const Vector2 &a, const Vector2 &b);
std::ostream &operator<<(std::ostream &os, const Vector2 &a);

// Utility functions for Vector2
Vector2 Vector2Normalize(const Vector2 &a);
float Vector2Distance(const Vector2 &a, const Vector2 &b);
float Vector2DistanceSq(const Vector2 &a, const Vector2 &b);
bool IsVector2WithinBounds(const Vector2 &a, const Vector2 &boxStart, const Vector2 &boxEnd);
Vector2 Vector2Round(const Vector2 &a);
Vector2 Vector2Floor(const Vector2 &a);
Vector2 Vector2Lerp(const Vector2 &a, const Vector2 &b, float i);
Vector2 Vector2Cap(const Vector2 &a, const Vector2 &b, float delta);
int Vector2ToRandomInt(const Vector2 &a, int min, int max);
Vector2 ToVector2(const Vector2Int &a);
std::string ToString(const Vector2 &a);

struct Vector2Hash
{
    std::size_t operator()(const Vector2 &v) const
    {
        return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1);
    }
};

struct Vector2Equal
{
    bool operator()(const Vector2 &a, const Vector2 &b) const noexcept
    {
        return (a.x == b.x && a.y == b.y);
    }
};

// Utility functions for Vector2Int
struct Vector2Int
{
    int x, y;

    Vector2Int() : x(0), y(0) {}
    Vector2Int(int x, int y) : x(x), y(y) {}

    Vector2Int operator+(const Vector2Int &a) const;
    Vector2Int operator-(const Vector2Int &a) const;
    Vector2Int &operator+=(Vector2Int &a);
    Vector2Int &operator-=(Vector2Int &a);
    bool operator==(const Vector2Int &a) const;

    struct Hash
    {
        std::size_t operator()(const Vector2Int &v) const
        {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };
};

float Vector2IntDistanceSq(const Vector2Int &a, const Vector2Int &b);
int Vector2IntToRandomInt(const Vector2Int &a, int min, int max);
Vector2Int ToVector2Int(const Vector2 &a);
std::string ToString(const Vector2Int &a);

// Rectangle operator overloads
Rectangle
operator*(const Rectangle &a, float b);
Rectangle &operator*=(Rectangle &a, float b);

// Utility functions for Rectangle
Rectangle Vector2ToRect(const Vector2 &a, const Vector2 &b);

// Utility functions for Containers
template <typename Container, typename T>
bool Contains(const Container &container, const T &value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

// Utility functions for std::string
std::string ToTitleCase(const std::string &a);