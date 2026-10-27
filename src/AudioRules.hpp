#pragma once
#include <algorithm>
#include <cmath>
namespace AudioRules
{
    inline float gain(int asset,int request,int bus)
    {return std::clamp(asset,0,100)*std::clamp(request,0,100)*std::clamp(bus,0,100)/10000.f;}
    inline float distanceGain(float distance,float radius)
    {const float weight=std::clamp(1-distance/radius,0.f,1.f);return weight*weight;}
    inline float fade(float current,float target,float seconds)
    {return current+(target-current)*(1-std::exp(-std::clamp(seconds,0.f,.25f)/.35f));}
    struct AnimationGate
    {
        int pose=-1,phase=-1;
        bool observe(int nextPose,int nextPhase,bool active)
        {
            if(!active){pose=phase=-1;return false;}
            const bool event=pose!=nextPose||nextPhase<phase;
            pose=nextPose;phase=nextPhase;return event;
        }
    };
}
