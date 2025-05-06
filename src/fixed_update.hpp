#pragma once
#include <condition_variable>
#include <mutex>

extern std::mutex updateMutex;
extern std::condition_variable fixedUpdateCondition;

void FixedUpdate(double &timeSinceFixedUpdate);
