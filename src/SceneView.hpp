#pragma once
namespace SceneView
{
    struct Point{int x,y;};struct Rect{int left,top,right,bottom;};
    float Scale();Rect Crop();Point ToScene(int x,int y);Point ToScreen(int x,int y);
    void Zoom(float delta,int anchorX,int anchorY);void Apply();
}
