#include "AudioRules.hpp"
#include <cmath>
#include <iostream>
int main()
{
    int errors=0;const auto check=[&](bool okay,const char* name){if(!okay){++errors;std::cerr<<name<<'\n';}};
    check(std::abs(AudioRules::gain(80,50,75)-30)<.001f,"asset, requested and category gains all contribute");
    check(AudioRules::gain(80,100,0)==0&&AudioRules::gain(0,100,100)==0,"muted buses and assets remain silent");
    check(AudioRules::gain(200,200,200)==100,"gain cannot exceed the supported range");
    check(AudioRules::distanceGain(0,6)==1&&AudioRules::distanceGain(6,6)==0&&AudioRules::distanceGain(9,6)==0,"nearby and distant ambience have defined endpoints");
    check(AudioRules::distanceGain(2,6)>AudioRules::distanceGain(4,6),"walking away reduces ambience");
    float level=0;for(int i=0;i<60;++i)level=AudioRules::fade(level,60,1.f/60);
    check(level>55&&level<60,"ambient sound fades in without jumping or overshooting");
    for(int i=0;i<120;++i)level=AudioRules::fade(level,0,1.f/60);
    check(level<.3f&&level>=0,"leaving a source or losing focus fades it out");
    AudioRules::AnimationGate gate;
    check(gate.observe(4,0,true),"first animation frame starts one cue");
    for(int i=0;i<240;++i)check(!gate.observe(4,0,true),"drawing the same frame cannot retrigger sound");
    check(!gate.observe(4,1,true)&&!gate.observe(4,2,true)&&gate.observe(4,0,true),"a new animation cycle permits another impact");
    check(gate.observe(5,0,true)&&!gate.observe(5,0,true),"changing actions starts one new cue");
    check(!gate.observe(5,0,false)&&gate.observe(5,0,true),"stopping and starting an action resets its cue");
    return errors?1:0;
}
