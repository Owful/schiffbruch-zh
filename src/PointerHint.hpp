#pragma once
#include <algorithm>
namespace PointerHint
{
    struct Rect{int x,y,w,h;};
    inline Rect Place(int mouseX,int mouseY,int width,int height)
    {
        const int x=mouseX+14+width<=958?mouseX+14:mouseX-14-width;
        return {std::clamp(x,2,958-width),std::clamp(mouseY+16,66,654-height),width,height};
    }
}
