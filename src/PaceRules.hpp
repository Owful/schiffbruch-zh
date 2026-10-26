#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
namespace PaceRules
{
    inline unsigned halfMinute=0,zoomStep=2;
    inline int workMinutes(int minutes)
    {if(minutes<=0)return 0;const unsigned total=unsigned(minutes)+halfMinute;halfMinute=total%2;return int(total/2);}
    inline float resourceChange(float amount){return amount<0?amount*.5f:amount;}
    inline void reset(){halfMinute=0;zoomStep=2;}
    inline void save(std::ostream& out){const std::uint8_t bytes[]={std::uint8_t(halfMinute),std::uint8_t(zoomStep)};out.write("PAC1",4);out.write(reinterpret_cast<const char*>(bytes),2);}
    inline bool load(std::istream& in)
    {std::array<char,4> magic{};std::uint8_t bytes[2]{};if(!in.read(magic.data(),4)||magic!=std::array<char,4>{'P','A','C','1'}||!in.read(reinterpret_cast<char*>(bytes),2)||bytes[0]>1||bytes[1]>8)return false;halfMinute=bytes[0];zoomStep=bytes[1];return true;}
}
