#pragma once
namespace Interaction
{
    enum class Intent { Any, FoodWater, Eat, Drink, Chop, ResumeBuild,OpenCache,FeedFire,Hunt };
    struct Point {int x=-1,y=-1; explicit operator bool()const{return x>=0&&y>=0;} };
    void ClearSelection();
    void SelectFire(int x,int y);
    Point FireTarget();
    bool Target(int x,int y,Intent intent);
    bool Request(Intent intent);
    bool Click(int screenX,int screenY);
    void Update();
    void Cancel();
    void StopSafely();
}
