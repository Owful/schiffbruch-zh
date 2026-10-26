#pragma once
namespace Interaction
{
    enum class Intent { Any, FoodWater, Eat, Drink, Chop, ResumeBuild };
    bool Request(Intent intent);
    bool Click(int screenX,int screenY);
    void Update();
    void Cancel();
    void StopSafely();
}
