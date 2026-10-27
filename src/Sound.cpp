#include "Sound.hpp"
#include "AudioRules.hpp"
#include "Action.hpp"
#include "Crafting.hpp"
#include "ExpeditionRules.hpp"
#include "State.hpp"
#include <array>
#include <cmath>
#include <fstream>

namespace Sound
{
    namespace
    {
        std::vector<sf::SoundBuffer> buffers;
        std::vector<sf::Sound> players;
        std::array<bool,COUNT> loaded{};
        std::array<std::string,COUNT> paths{};
        std::array<float,COUNT> levels{},requested{},lastPlayed{};
        std::array<int,3> mix{55,68,80};
        sf::Clock clock;
        AudioRules::AnimationGate animation;
        int supply=-1,character=NOTHING,characterPose=-1,variation=0;
        float stepTime=0;
        bool hasFocus=true,actionsAllowed=true;

        bool valid(int id){return id>NOTHING&&id<COUNT&&int(players.size())==COUNT&&loaded[id];}
        bool ui(int id){return id==CLICK||id==CLICK2;}
        Bus bus(int id)
        {
            if(id==LOGO||id==OUTRO)return Music;
            if(id==STORM||id==RAIN||id==FOREST||id==SURF||id==RIVER||id==FIRE)return Ambience;
            return Effects;
        }
        void saveSettings()
        {
            std::ofstream file("audio-settings.cfg");
            file<<"AUDIO1 "<<(Soundzustand>0?1:0)<<' '<<mix[0]<<' '<<mix[1]<<' '<<mix[2]<<'\n';
        }
        bool isTree(int object){return (object>=BAUM1&&object<=BAUM4DOWN)||object==BAUMGROSS||object==HAUS1||object==HAUS2||object==HAUS3;}
        void environment(std::array<float,COUNT>& target)
        {
            if(Guy.Pos.x<0||Guy.Pos.y<0||Guy.Pos.x>=MAXXKACH||Guy.Pos.y>=MAXYKACH)return;
            float sea=0,river=0,forest=0,fire=0;
            for(int y=std::max(0,int(Guy.Pos.y)-8);y<=std::min(int(MAXYKACH)-1,int(Guy.Pos.y)+8);++y)
                for(int x=std::max(0,int(Guy.Pos.x)-8);x<=std::min(int(MAXXKACH)-1,int(Guy.Pos.x)+8);++x)
                {
                    const auto& tile=Scape[x][y];if(!tile.Entdeckt)continue;
                    const float dx=float(x-Guy.Pos.x),dy=float(y-Guy.Pos.y),distance=std::sqrt(dx*dx+dy*dy);
                    const int object=tile.Objekt;
                    if(object==MEERWELLEN)sea=std::max(sea,AudioRules::distanceGain(distance,9));
                    if(object>=FLUSS1&&object<=SCHLEUSE6)river=std::max(river,AudioRules::distanceGain(distance,6));
                    if(isTree(object))forest=std::max(forest,AudioRules::distanceGain(distance,7));
                    if(object==FEUER)fire=std::max(fire,AudioRules::distanceGain(distance,5));
                }
            const auto weather=ExpeditionRules::weather(Tag);
            const bool wet=weather!=ExpeditionRules::Weather::Clear;
            target[SURF]=sea*62;target[RIVER]=river*65;target[FIRE]=fire*65;
            target[FOREST]=forest*(wet?20.f:Nacht?15.f:55.f);
            if(wet)target[RAIN]=weather==ExpeditionRules::Weather::Storm?66.f:42.f;
            if(weather==ExpeditionRules::Weather::Storm)target[STORM]=48;
        }
    }

    void Init()
    {
        players.clear();buffers.clear();buffers.resize(COUNT);players.resize(COUNT);
        loaded.fill(false);levels.fill(0);requested.fill(0);lastPlayed.fill(-1000);
        animation={};supply=-1;character=NOTHING;characterPose=-1;stepTime=0;
        std::ifstream file("audio-settings.cfg");std::string magic;int enabled=1,a=55,b=68,c=80;
        if(file>>magic>>enabled>>a>>b>>c&&magic=="AUDIO1")
        {mix={std::clamp(a,0,100),std::clamp(b,0,100),std::clamp(c,0,100)};Soundzustand=enabled?1:0;}
    }
    void LoadSound(short id)
    {
        if(id<=0||id>=COUNT||int(buffers.size())!=COUNT)return;
        if(loaded[id]&&paths[id]==Wav[id].Dateiname)return;
        if(loaded[id])StopSound(id);
        loaded[id]=buffers[id].loadFromFile(Wav[id].Dateiname);
        if(!loaded[id]){sf::err()<<"Could not load audio: "<<Wav[id].Dateiname<<'\n';return;}
        players[id].setBuffer(buffers[id]);players[id].setLoop(Wav[id].Loop);players[id].setVolume(0);
        paths[id]=Wav[id].Dateiname;
    }
    void PlaySound(short id,short volume)
    {
        if(ui(id))return;
        if(!valid(id)||Soundzustand<=0||!hasFocus||(!actionsAllowed&&!ui(id)))return;
        // Environmental loops are driven once per frame by player position, never by drawing tiles.
        if(Wav[id].Loop)return;
        const float now=clock.getElapsedTime().asSeconds();
        if(now-lastPlayed[id]<(ui(id)?.055f:.11f)||players[id].getStatus()==sf::Sound::Playing)return;
        requested[id]=float(std::clamp<int>(volume,0,100));
        players[id].setVolume(AudioRules::gain(Wav[id].Volume,volume,mix[bus(id)]));
        static constexpr float pitches[]={.985f,1.012f,.997f,1.025f};
        players[id].setPitch(ui(id)||id==INVENTION?1.f:pitches[(variation++)%4]);
        players[id].play();lastPlayed[id]=now;
    }
    void StopSound(short id)
    {
        if(!valid(id))return;
        players[id].stop();levels[id]=0;requested[id]=0;lastPlayed[id]=-1000;
    }
    int GetMix(Bus channel){return channel>=Music&&channel<=Effects?mix[channel]:0;}
    void SetMix(Bus channel,int level)
    {if(channel<Music||channel>Effects)return;mix[channel]=std::clamp(level,0,100);saveSettings();}
    void SetEnabled(bool enabled)
    {Soundzustand=enabled?1:0;if(!enabled)for(int id=1;id<COUNT;++id)StopSound(short(id));saveSettings();}

    void Update(std::int64_t micros,bool focused,bool paused)
    {
        hasFocus=focused;actionsAllowed=focused&&!paused;
        const float seconds=std::clamp(float(micros)/1000000,0.f,.25f);
        std::array<float,COUNT> target{};
        if(focused&&Soundzustand>0)
        {
            if(Spielzustand==State::LOGO)target[LOGO]=100;
            else if(Spielzustand==State::OUTRO)target[OUTRO]=100;
            else if(Spielzustand==State::GAME)environment(target);
            else if(Spielzustand==State::INTRO||Spielzustand==State::RESCUED)
            {target[SURF]=35;if(Guy.Zustand==GUYSCHIFF)target[STORM]=55;}
            if(Guy.Aktiv&&actionsAllowed)
            {
                if(Guy.Zustand==GUYSCHWIMMEN)target[SWIM]=65;
                if(Guy.Zustand==GUYSCHLAFEN||Guy.Zustand==GUYSCHLAFZELT||Guy.Zustand==GUYSCHLAFHAUS)target[SNORE]=55;
            }
            if(paused)for(int id=1;id<COUNT;++id)target[id]*=bus(id)==Music?1.f:.22f;
        }
        if(int(players.size())!=COUNT)return;
        for(int id=1;id<COUNT;++id)
        {
            if(!loaded[id])continue;
            if(id==LOG)
            {
                // The complete original clip owns the accented rhythm. Audio looping
                // keeps its 0.615 s beat independent of frame rate and animation wraps.
                const bool chopping=Guy.Aktiv&&Guy.Zustand==GUYFAELLEN;
                levels[id]=AudioRules::gain(Wav[id].Volume,100,mix[Effects]);
                players[id].setVolume(levels[id]);players[id].setPitch(1);
                if(!chopping||Soundzustand<=0)StopSound(LOG);
                else if(!focused||paused){if(players[id].getStatus()==sf::Sound::Playing)players[id].pause();}
                else if(players[id].getStatus()!=sf::Sound::Playing)players[id].play();
                continue;
            }
            if(Wav[id].Loop)
            {
                const float wanted=AudioRules::gain(Wav[id].Volume,int(target[id]),mix[bus(id)]);
                levels[id]=AudioRules::fade(levels[id],wanted,seconds);players[id].setVolume(levels[id]);
                if(levels[id]>.08f&&wanted>0&&players[id].getStatus()!=sf::Sound::Playing)players[id].play();
                else if(wanted==0&&levels[id]<.08f)players[id].stop();
            }
            else
            {
                players[id].setVolume(AudioRules::gain(Wav[id].Volume,int(requested[id]),mix[bus(id)]));
                if((!focused||paused)&&!ui(id)&&players[id].getStatus()==sf::Sound::Playing)players[id].pause();
                else if(actionsAllowed&&players[id].getStatus()==sf::Sound::Paused)players[id].play();
                if(Soundzustand<=0)players[id].stop();
            }
        }
        if(!actionsAllowed||Soundzustand<=0)return;
        const bool active=Guy.Aktiv&&Guy.Zustand>=0&&Guy.Zustand<BILDANZ;
        const bool edge=animation.observe(Guy.Zustand,active?int(Bmp[Guy.Zustand].Phase):0,active);
        if(character!=NOTHING&&(!active||Guy.Zustand!=characterPose))
        {StopSound(short(character));character=NOTHING;}
        if(!active){supply=-1;stepTime=0;return;}
        int cue=Bmp[Guy.Zustand].Sound;
        const bool usingSupply=Guy.Aktion==Action::CRAFT&&Crafting::UsingSupply();
        bool trigger=edge;
        if(usingSupply)
        {
            const int product=Crafting::Job().product;trigger=supply!=product;supply=product;
            cue=product==Crafting::FillWater?FILL:product==Crafting::ApplyBandage?BANDAGE:DRINK;
        }
        else supply=-1;
        if(trigger&&valid(cue)&&!Wav[cue].Loop)
        {
            PlaySound(short(cue),100);character=cue;characterPose=Guy.Zustand;
            if(usingSupply&&Crafting::Job().fraction>.1f)
            {const float duration=buffers[cue].getDuration().asSeconds();players[cue].setPlayingOffset(sf::seconds(std::min(duration*.9f,Crafting::Job().fraction*duration)));}
        }
        if(Guy.Zustand>=GUYLINKS&&Guy.Zustand<=GUYUNTEN)
        {stepTime+=seconds;if(stepTime>=.36f){PlaySound(FOOTSTEP,50);stepTime=0;}}
        else stepTime=0;
    }
    Snapshot Inspect()
    {
        Snapshot out;
        for(int id=1;id<COUNT;++id)if(loaded[id])
        {++out.loaded;if(players[id].getStatus()==sf::Sound::Playing)++out.playing;if(bus(id)==Music)out.music+=levels[id];else if(bus(id)==Ambience)out.ambience+=levels[id];}
        return out;
    }
    float CurrentVolume(short id){return valid(id)?players[id].getVolume():0;}
    bool IsPlaying(short id){return valid(id)&&players[id].getStatus()==sf::Sound::Playing;}
}
