#pragma once
#include <algorithm>
#include <cmath>

namespace MapProjection
{
    constexpr int Columns=60,Rows=60,Width=3360,Height=1700;
    constexpr int OriginX=1652,StepX=28,StepY=14,Elevation=16;
    struct Point{int x,y;};
    struct Coordinate{double x,y;};
    inline Point TileOrigin(int x,int y,int height=0)
    {return {OriginX+StepX*(x-y),StepY*(x+y)-Elevation*height};}
    inline Coordinate TileCenter(double x,double y,int height=0)
    {return {OriginX+27+StepX*(x-y),29+StepY*(x+y)-Elevation*height};}
    inline Coordinate Grid(double worldX,double worldY)
    {
        const double a=(worldX-OriginX-27)/StepX,b=(worldY-29)/StepY;
        return {(a+b)/2,(b-a)/2};
    }
    inline bool Inside(double worldX,double worldY)
    {const auto p=Grid(worldX,worldY);return p.x>=-.5&&p.y>=-.5&&p.x<Columns-.5&&p.y<Rows-.5;}
    struct Frame
    {
        double left=0,top=0,scale=1;
        static Frame Fit(int x,int y,int width,int height)
        {
            const double s=std::min(double(width)/Width,double(height)/Height);
            return {x+(width-Width*s)/2,y+(height-Height*s)/2,s};
        }
        Point project(double x,double y)const{return {int(std::lround(left+x*scale)),int(std::lround(top+y*scale))};}
        Coordinate unproject(int x,int y)const{return {(x-left)/scale,(y-top)/scale};}
    };
}
