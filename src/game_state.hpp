#pragma once
#include "camera.hpp"

struct Tile;

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

struct GameManager
{
private:
    GameState state = GameState::NONE;
    PlayerCam camera = PlayerCam();
    std::shared_ptr<Tile> selectedTile;

    GameManager() = default;
    ~GameManager() = default;
    GameManager(const GameManager &) = delete;
    GameManager &operator=(const GameManager &) = delete;

    static GameManager &GetInstance()
    {
        static GameManager instance;
        return instance;
    }

public:
    static GameState GetGameState() { return GetInstance().state; }
    static bool IsGameRunning() { return magic_enum::enum_flags_test(GetGameState(), GameState::RUNNING); }
    static bool IsGamePaused() { return magic_enum::enum_flags_test_any(GetGameState(), GameState::PAUSED | GameState::FORCE_PAUSED); }
    static void SetGameState(GameState mask, bool bitState = true)
    {
        if (bitState)
            GetInstance().state |= mask;
        else
            GetInstance().state &= ~mask;
    }

    static void ToggleGameState(GameState mask)
    {
        GetInstance().state ^= mask;
    }

    static PlayerCam &GetCamera() { return GetInstance().camera; }

    static const std::shared_ptr<Tile> &GetSelectedTile() { return GetInstance().selectedTile; }
    static void SetSelectedTile(const std::shared_ptr<Tile> &selection) { GetInstance().selectedTile = selection; }

    // Utility function for Screen to World space transformations
    static Vector2 GetWorldMousePos();
    static Vector2 ScreenToWorld(const Vector2 &screenPos);
    static Vector2Int ScreenToTile(const Vector2 &screenPos);
    static Vector2 WorldToScreen(const Vector2 &worldPos);
    static Vector2 WorldToScreen(const Vector2Int &worldPos);
    static Rectangle WorldToScreen(const Rectangle &worldRect);
};
