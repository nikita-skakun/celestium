#pragma once
#include "utils.hpp"

struct SpriteSlice
{
    Rectangle sourceRect;
    Vector2 destOffset;

    SpriteSlice() : sourceRect(Rectangle()), destOffset(Vector2()) {}
    SpriteSlice(const Rectangle &sourceRect, const Vector2 &destOffset) : sourceRect(sourceRect), destOffset(destOffset) {}
    SpriteSlice(float x, float y, float width, float height, float offsetX, float offsetY)
        : sourceRect(Rectangle(x, y, width, height)), destOffset(Vector2(offsetX, offsetY)) {}
};

enum class SpriteCondition : u_int32_t
{
    NONE = 0,
    NORTH_SAME = 1 << 0,
    EAST_SAME = 1 << 1,
    SOUTH_SAME = 1 << 2,
    WEST_SAME = 1 << 3,
    NORTH_DIFFERENT = 1 << 4,
    EAST_DIFFERENT = 1 << 5,
    SOUTH_DIFFERENT = 1 << 6,
    WEST_DIFFERENT = 1 << 7,
    NORTH_EAST_SAME = 1 << 8,
    SOUTH_EAST_SAME = 1 << 9,
    SOUTH_WEST_SAME = 1 << 10,
    NORTH_WEST_SAME = 1 << 11,
    NORTH_EAST_DIFFERENT = 1 << 12,
    SOUTH_EAST_DIFFERENT = 1 << 13,
    SOUTH_WEST_DIFFERENT = 1 << 14,
    NORTH_WEST_DIFFERENT = 1 << 15,
};

template <>
struct magic_enum::customize::enum_range<SpriteCondition>
{
    static constexpr bool is_flags = true;
};

struct SliceWithConditions
{
    SpriteCondition conditions;
    SpriteSlice slice;

    SliceWithConditions(const SpriteCondition &conditions, const SpriteSlice &slice)
        : conditions(conditions), slice(slice) {}
};

struct SpriteDef
{
    virtual ~SpriteDef() = default;
};

struct BasicSpriteDef : public SpriteDef
{
    Vector2Int spriteOffset;

    BasicSpriteDef(const Vector2Int &offset) : spriteOffset(offset) {}
};

struct MultiSliceSpriteDef : public SpriteDef
{
    std::vector<SliceWithConditions> slices;

    MultiSliceSpriteDef(const std::vector<SliceWithConditions> &slices) : slices(slices) {}
};

struct Sprite
{
protected:
    Vector2Int offsetFromMainTile;

public:
    Sprite(const Vector2Int &offsetFromMainTile = Vector2Int()) : offsetFromMainTile(offsetFromMainTile) {}

    virtual ~Sprite() = default;
    virtual void Draw(const Vector2Int &position, const Color &tint) const = 0;
    constexpr const Vector2Int &GetOffsetFromMainTile() const { return offsetFromMainTile; }
};

struct BasicSprite : public Sprite
{
    Vector2Int spriteOffset;

    BasicSprite(const Vector2Int &spriteOffset, const Vector2Int &offsetFromMainTile = Vector2Int())
        : Sprite(offsetFromMainTile), spriteOffset(spriteOffset) {}

    void Draw(const Vector2Int &position, const Color &tint) const override;
};

struct MultiSliceSprite : public Sprite
{
    std::vector<SpriteSlice> slices;

    MultiSliceSprite(const std::vector<SpriteSlice> &slices, const Vector2Int &offsetFromMainTile = Vector2Int())
        : Sprite(offsetFromMainTile), slices(slices) {}

    void Draw(const Vector2Int &position, const Color &tint) const override;
};