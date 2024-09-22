#include "utils.h"

Vector2 operator+(const Vector2 &a, const Vector2 &b)
{
    return {a.x + b.x, a.y + b.y};
}

Vector2 operator-(const Vector2 &a, const Vector2 &b)
{
    return {a.x - b.x, a.y - b.y};
}

Vector2 &operator+=(Vector2 &a, const Vector2 &b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

Vector2 &operator-=(Vector2 &a, const Vector2 &b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

Vector2 operator*(const Vector2 &a, float b)
{
    return {a.x * b, a.y * b};
}

Vector2 operator/(const Vector2 &a, float b)
{
    return {a.x / b, a.y / b};
}

Vector2 &operator*=(Vector2 &a, float b)
{
    a.x *= b;
    a.y *= b;
    return a;
}
Vector2 &operator/=(Vector2 &a, float b)
{
    a.x /= b;
    a.y /= b;
    return a;
}

bool operator==(const Vector2 &a, const Vector2 &b)
{
    return a.x == b.x && a.y == b.y;
}

std::ostream &operator<<(std::ostream &os, const Vector2 &a)
{
    os << "{" << a.x << ", " << a.y << "}";
    return os;
}

Vector2 Vector2Normalize(const Vector2 &a)
{
    float length = std::sqrt(a.x * a.x + a.y * a.y);
    if (length == 0)
    {
        return a;
    }
    return a / length;
}

float Vector2Distance(const Vector2 &a, const Vector2 &b)
{
    return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

float Vector2DistanceSq(const Vector2 &a, const Vector2 &b)
{
    return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
}

bool IsVector2WithinBounds(const Vector2 &a, const Vector2 &boxStart, const Vector2 &boxEnd)
{
    return (a.x >= std::min(boxStart.x, boxEnd.x) && a.x <= std::max(boxStart.x, boxEnd.x) && a.y >= std::min(boxStart.y, boxEnd.y) && a.y <= std::max(boxStart.y, boxEnd.y));
}

Vector2 Vector2Round(const Vector2 &a)
{
    return Vector2(round(a.x), round(a.y));
}

Vector2 Vector2Floor(const Vector2 &a)
{
    return Vector2(floor(a.x), floor(a.y));
}

int Vector2ToRandomInt(const Vector2 &a, int min, int max)
{
    Vector2Hash vectorHash;
    std::size_t seed = vectorHash(a);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

std::string ToString(const Vector2 &a)
{
    return fmt::format("({:.2f}, {:.2f})", a.x, a.y);
}

std::string ToString(const Vector2Int &a)
{
    return fmt::format("({:}, {:})", a.x, a.y);
}

Rectangle operator*(const Rectangle &a, float b)
{
    return {a.x * b, a.y * b, a.width * b, a.height * b};
}

Rectangle &operator*=(Rectangle &a, float b)
{
    a.x *= b;
    a.y *= b;
    a.width *= b;
    a.height *= b;
    return a;
}

Rectangle Vector2ToRect(const Vector2 &a, const Vector2 &b)
{
    float startX = std::min(a.x, b.x);
    float startY = std::min(a.y, b.y);
    return Rectangle{startX, startY, std::max(a.x, b.x) - startX, std::max(a.y, b.y) - startY};
}

std::string ToTitleCase(const std::string &a)
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