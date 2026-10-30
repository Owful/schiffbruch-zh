#pragma once
#include "MapProjection.hpp"
namespace MapView
{
    MapProjection::Frame MiniFrame();
    MapProjection::Frame OverviewFrame();
    void BuildMini();
    void DrawMini();
    void DrawOverview();
    bool Focus(int x,int y,const MapProjection::Frame& frame);
}
