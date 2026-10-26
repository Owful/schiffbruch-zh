#pragma once
#include <SFML/Graphics.hpp>
namespace NativeDisplay
{
    void Bind(sf::RenderWindow& window);
    void Present();
    void Resize(unsigned width,unsigned height);
    sf::Vector2i MousePoint();
    void SetFade(float r,float g,float b);
    float FrameMilliseconds();
}
