#pragma once
#include "ui_element.hpp"

struct UiManager
{
private:
    UiManager() = default;
    ~UiManager() = default;
    UiManager(const UiManager &) = delete;
    UiManager &operator=(const UiManager &) = delete;

public:
    static UiManager &GetInstance()
    {
        static UiManager instance;
        return instance;
    }
};