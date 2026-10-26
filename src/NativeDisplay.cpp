#include "NativeDisplay.hpp"
#include "extern.hpp"
#include <algorithm>
#include <vector>

namespace NativeDisplay
{
    namespace
    {
        sf::RenderWindow* window=nullptr;
        sf::Texture texture;
        sf::Shader shader;
        bool useShader=false;
        sf::Vector3f fade{1,1,1};
        float frameMs=0;
        std::vector<sf::Uint8> fallback;
    }
    void Bind(sf::RenderWindow& target)
    {
        window=&target;texture.create(MAXX,MAXY);texture.setSmooth(true);
        useShader=sf::Shader::isAvailable()&&shader.loadFromMemory(
            "uniform sampler2D image;uniform vec3 fade;void main(){vec4 c=texture2D(image,gl_TexCoord[0].xy);gl_FragColor=vec4(c.bgr*fade,1.0);}",sf::Shader::Fragment);
        if(useShader)shader.setUniform("image",sf::Shader::CurrentTexture);
        else fallback.resize(MAXX*MAXY*4);
        NativeSurface::present=Present;Resize(target.getSize().x,target.getSize().y);
    }
    void Resize(unsigned width,unsigned height)
    {
        if(!window||!width||!height)return;
        sf::View view(sf::FloatRect(0,0,MAXX,MAXY));
        const float ratio=float(width)/height,desired=float(MAXX)/MAXY;
        view.setViewport(ratio>desired?sf::FloatRect((1-desired/ratio)*.5f,0,desired/ratio,1):sf::FloatRect(0,(1-ratio/desired)*.5f,1,ratio/desired));
        window->setView(view);
    }
    sf::Vector2i MousePoint()
    {
        if(!window)return {-1,-1};
        const auto p=window->mapPixelToCoords(sf::Mouse::getPosition(*window));return {int(p.x),int(p.y)};
    }
    void SetFade(float r,float g,float b){fade={std::clamp(r,0.f,1.f),std::clamp(g,0.f,1.f),std::clamp(b,0.f,1.f)};}
    float FrameMilliseconds(){return frameMs;}
    void Present()
    {
        if(!window||!window->isOpen()||!lpDDSBack)return;
        sf::Clock clock;const auto* bytes=lpDDSBack->pixels();
        if(useShader){texture.update(bytes);shader.setUniform("fade",fade);}
        else{for(std::size_t i=0;i<fallback.size();i+=4){fallback[i]=sf::Uint8(bytes[i+2]*fade.x);fallback[i+1]=sf::Uint8(bytes[i+1]*fade.y);fallback[i+2]=sf::Uint8(bytes[i]*fade.z);fallback[i+3]=255;}texture.update(fallback.data());}
        window->clear(sf::Color(12,24,27));
        window->draw(sf::Sprite(texture),useShader?sf::RenderStates(&shader):sf::RenderStates::Default);
        window->display();frameMs=frameMs*.9f+clock.getElapsedTime().asMilliseconds()*.1f;
    }
}
