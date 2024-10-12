#pragma once
#include "utils.hpp"

enum class GameState : u_int8_t
{
    NONE = 0,
    RUNNING = 1 << 0,
    PAUSED = 1 << 1,
    FORCE_PAUSED = 1 << 2,
};

template <>
struct magic_enum::customize::enum_range<GameState>
{
    static constexpr bool is_flags = true;
};

constexpr bool IsGameRunning(GameState state) { return magic_enum::enum_flags_test(state, GameState::RUNNING); }
constexpr bool IsGamePaused(GameState state) { return magic_enum::enum_flags_test_any(state, GameState::PAUSED | GameState::FORCE_PAUSED); }

constexpr void SetBit(GameState &state, bool bitValue, GameState mask)
{
    if (bitValue)
        state |= mask;
    else
        state &= ~mask;
}

constexpr void ToggleBit(GameState &state, GameState mask)
{
    state ^= mask;
}