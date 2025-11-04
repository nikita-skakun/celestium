#include "utils.hpp"
#include <raylib.h>
#include <cmath>
#include <random>
#include <functional>

Vector2 operator+(const Vector2 &a, const Vector2 &b) noexcept
{
    return Vector2(a.x + b.x, a.y + b.y);
}

Vector2 operator-(const Vector2 &a, const Vector2 &b) noexcept
{
    return Vector2(a.x - b.x, a.y - b.y);
}

Vector2 &operator+=(Vector2 &a, const Vector2 &b) noexcept
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

Vector2 &operator-=(Vector2 &a, const Vector2 &b) noexcept
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

Vector2 operator*(const Vector2 &a, float b) noexcept
{
    return Vector2(a.x * b, a.y * b);
}

Vector2 operator/(const Vector2 &a, float b) noexcept
{
    return Vector2(a.x / b, a.y / b);
}

Vector2 &operator*=(Vector2 &a, float b) noexcept
{
    a.x *= b;
    a.y *= b;
    return a;
}

Vector2 &operator/=(Vector2 &a, float b) noexcept
{
    a.x /= b;
    a.y /= b;
    return a;
}

bool operator==(const Vector2 &a, const Vector2 &b) noexcept
{
    return a.x == b.x && a.y == b.y;
}

std::size_t Vector2Hash::operator()(const Vector2 &v) const noexcept
{
    return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1);
}

Vector2 Vector2Normalize(const Vector2 &a) noexcept
{
    float lengthSq = a.x * a.x + a.y * a.y;
    if (lengthSq == 0)
    {
        return a;
    }
    return a / std::sqrt(lengthSq);
}

float Vector2LengthSq(const Vector2 &a) noexcept
{
    return a.x * a.x + a.y * a.y;
}

float Vector2Distance(const Vector2 &a, const Vector2 &b) noexcept
{
    return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

float Vector2DistanceSq(const Vector2 &a, const Vector2 &b) noexcept
{
    return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
}

float Vector2Manhattan(const Vector2 &a, const Vector2 &b) noexcept
{
    return std::abs(b.x - a.x) + std::abs(b.y - a.y);
}

Vector2 Vector2Round(const Vector2 &a) noexcept
{
    return Vector2(round(a.x), round(a.y));
}

Vector2 Vector2Floor(const Vector2 &a) noexcept
{
    return Vector2(floor(a.x), floor(a.y));
}

Vector2 Vector2Lerp(const Vector2 &a, const Vector2 &b, float i) noexcept
{
    return Vector2(a.x + (b.x - a.x) * i, a.y + (b.y - a.y) * i);
}

Vector2 Vector2Cap(const Vector2 &a, const Vector2 &b, float delta) noexcept
{
    if (Vector2DistanceSq(a, b) < delta * delta)
        return b;
    else
        return a + Vector2Normalize(b - a) * delta;
}

Vector2 Vector2ScreenScale(const Vector2 &a) noexcept
{
    return Vector2(a.x / 1920., a.y / 1080.);
}

int Vector2ToRandomInt(const Vector2 &a, int min, int max) noexcept
{
    Vector2Hash vectorHash;
    std::size_t seed = vectorHash(a);
    thread_local static std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

std::string ToString(const Vector2 &a, int precision) noexcept
{
    return "(" + ToString(a.x, precision) + ", " + ToString(a.y, precision) + ")";
}

// Vector2Int helpers (moved from header)

float Vector2IntDistanceSq(const Vector2Int &a, const Vector2Int &b) noexcept
{
    return float(b.x - a.x) * float(b.x - a.x) + float(b.y - a.y) * float(b.y - a.y);
}

bool Vector2IntTouching(const Vector2Int &a, const Vector2Int &b) noexcept
{
    return (a.x == b.x && std::abs(a.y - b.y) == 1) || (a.y == b.y && std::abs(a.x - b.x) == 1);
}

int Vector2IntManhattan(const Vector2Int &a, const Vector2Int &b) noexcept
{
    return std::abs(b.x - a.x) + std::abs(b.y - a.y);
}

int Vector2IntChebyshev(const Vector2Int &a, const Vector2Int &b) noexcept
{
    return std::max(std::abs(b.x - a.x), std::abs(b.y - a.y));
}

int Vector2IntToRandomInt(const Vector2Int &a, int min, int max) noexcept
{
    std::hash<Vector2Int> hash;
    std::size_t seed = hash(a);
    thread_local static std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

Vector2Int ToVector2Int(const Vector2 &a) noexcept
{
    return Vector2Int((int)std::floor(a.x), (int)std::floor(a.y));
}

Vector2 ToVector2(const Vector2Int &a) noexcept
{
    return Vector2((float)(a.x), (float)(a.y));
}

Vector2 GetScreenSize()
{
    int monitor = GetCurrentMonitor();
    return Vector2((float)GetMonitorWidth(monitor), (float)GetMonitorHeight(monitor));
}

std::string ToString(const Vector2Int &a) noexcept
{
    return std::format("({}, {})", a.x, a.y);
}

// Rectangle helpers (moved from header)

Rectangle operator*(const Rectangle &a, float b) noexcept
{
    return {a.x * b, a.y * b, a.width * b, a.height * b};
}

Rectangle operator*(const Rectangle &a, const Vector2 &b) noexcept
{
    return {a.x * b.x, a.y * b.y, a.width * b.x, a.height * b.y};
}

Rectangle &operator*=(Rectangle &a, float b) noexcept
{
    a.x *= b;
    a.y *= b;
    a.width *= b;
    a.height *= b;
    return a;
}

Rectangle &operator*=(Rectangle &a, const Vector2 &b) noexcept
{
    a.x *= b.x;
    a.y *= b.y;
    a.width *= b.x;
    a.height *= b.y;
    return a;
}

bool operator==(const Rectangle &a, const Rectangle &b) noexcept
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

Rectangle Vector2ToBoundingBox(const Vector2 &a, const Vector2 &b) noexcept
{
    float startX = std::min(a.x, b.x);
    float startY = std::min(a.y, b.y);
    return Rectangle(startX, startY, std::max(a.x, b.x) - startX, std::max(a.y, b.y) - startY);
}

Rectangle Vector2ToRect(const Vector2 &pos, const Vector2 &size) noexcept
{
    return Rectangle(pos.x, pos.y, size.x, size.y);
}

Vector2 RectToPos(const Rectangle &rect) noexcept
{
    return Vector2(rect.x, rect.y);
}

Vector2 RectToSize(const Rectangle &rect) noexcept
{
    return Vector2(rect.width, rect.height);
}

bool IsVector2WithinRect(const Rectangle &rect, const Vector2 &point) noexcept
{
    return (point.x >= rect.x && point.x <= (rect.x + rect.width) &&
            point.y >= rect.y && point.y <= (rect.y + rect.height));
}

std::string ToString(const Rectangle &rect, int precision) noexcept
{
    return "(" + ToString(rect.x, precision) + ", " + ToString(rect.y, precision) + ", " +
           ToString(rect.width, precision) + ", " + ToString(rect.height, precision) + ")";
}

float DistanceSqFromPointToLine(const Vector2 &a, const Vector2 &b, const Vector2 &p) noexcept
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

Color RandomColor()
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

    return Color(r, g, b, 255);
}
