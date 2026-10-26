#include "Direct.hpp"

#include "types.hpp"

#include "Action.hpp"
#include "Game.hpp"
#include "Math.hpp"
#include "Renderer.hpp"
#include "Sound.hpp"
#include "State.hpp"
#include "World.hpp"
#include "Expedition.hpp"

#include <SFML/Window.hpp>
#include <array>
#include <stdexcept>
#include "NativeDisplay.hpp"
#include "ModernUI.hpp"
#include "SceneView.hpp"

namespace Direct
{
    bool Button0down; // linke Maustaste gedrückt gehalten
    bool Button1down; // rechte Maustaste gedrückt gehalten
    std::array<bool, sf::Keyboard::KeyCount> queuedKeys{};

    void QueueKeyPress(int key)
    {
        if (key >= 0 && key < sf::Keyboard::KeyCount) queuedKeys[key] = true;
    }

    void finiObjects()
    {
        NativeSurface::present={};
        NativeSurface** surfaces[]={&lpDDSPrimary,&lpDDSBack,&lpDDSMisc,&lpDDSPanel,&lpDDSGuyAni,&lpDDSAnimation,&lpDDSKarte,&lpDDSSchrift,&lpDDSSchrift1,&lpDDSSchrift2,&lpDDSTextFeld,&lpDDSPapier,&lpDDSBaum,&lpDDSBau,&lpDDSCredits,&lpDDSLogo,&lpDDSCursor,&lpDDSButtons,&lpDDSInventar,&lpDDSScape,&lpDDSSchatzkarte};
        for(auto** surface:surfaces){delete *surface;*surface=nullptr;}
    }

    void InitDDraw(HWND)
    {
        lpDDSPrimary=new NativeSurface(MAXX,MAXY);
        lpDDSBack=new NativeSurface(MAXX,MAXY);
        lpDDSMisc=DDLoadBitmap(nullptr,Misc,0,0);
        if(!lpDDSMisc)throw std::runtime_error("Missing bitmap resource: Misc");
        DDSetColorKey(lpDDSMisc,RGB(255,0,255));
        lpDDSPanel=DDLoadBitmap(nullptr,Panel,0,0);
        if(!lpDDSPanel)throw std::runtime_error("Missing bitmap resource: Panel");
        DDSetColorKey(lpDDSPanel,RGB(255,0,255));
        lpDDSGuyAni=DDLoadBitmap(nullptr,GuyAni,0,0);
        if(!lpDDSGuyAni)throw std::runtime_error("Missing bitmap resource: GuyAni");
        DDSetColorKey(lpDDSGuyAni,RGB(255,0,255));
        lpDDSAnimation=DDLoadBitmap(nullptr,Animation,0,0);
        if(!lpDDSAnimation)throw std::runtime_error("Missing bitmap resource: Animation");
        DDSetColorKey(lpDDSAnimation,RGB(255,0,255));
        lpDDSSchrift1=DDLoadBitmap(nullptr,Schrift1,0,0);
        if(!lpDDSSchrift1)throw std::runtime_error("Missing bitmap resource: Schrift1");
        DDSetColorKey(lpDDSSchrift1,RGB(255,0,255));
        lpDDSSchrift2=DDLoadBitmap(nullptr,Schrift2,0,0);
        if(!lpDDSSchrift2)throw std::runtime_error("Missing bitmap resource: Schrift2");
        DDSetColorKey(lpDDSSchrift2,RGB(255,0,255));
        lpDDSPapier=DDLoadBitmap(nullptr,Papier,0,0);
        if(!lpDDSPapier)throw std::runtime_error("Missing bitmap resource: Papier");
        DDSetColorKey(lpDDSPapier,RGB(255,0,255));
        lpDDSBaum=DDLoadBitmap(nullptr,Baum,0,0);
        if(!lpDDSBaum)throw std::runtime_error("Missing bitmap resource: Baum");
        DDSetColorKey(lpDDSBaum,RGB(255,0,255));
        lpDDSCursor=DDLoadBitmap(nullptr,Cursorbmp,0,0);
        if(!lpDDSCursor)throw std::runtime_error("Missing bitmap resource: Cursorbmp");
        DDSetColorKey(lpDDSCursor,RGB(255,0,255));
        lpDDSButtons=DDLoadBitmap(nullptr,Buttons,0,0);
        if(!lpDDSButtons)throw std::runtime_error("Missing bitmap resource: Buttons");
        DDSetColorKey(lpDDSButtons,RGB(255,0,255));
        lpDDSInventar=DDLoadBitmap(nullptr,Inventarbmp,0,0);
        if(!lpDDSInventar)throw std::runtime_error("Missing bitmap resource: Inventarbmp");
        DDSetColorKey(lpDDSInventar,RGB(255,0,255));
        lpDDSBau=DDLoadBitmap(nullptr,Bau,0,0);
        if(!lpDDSBau)throw std::runtime_error("Missing bitmap resource: Bau");
        DDSetColorKey(lpDDSBau,RGB(255,0,255));
        lpDDSCredits=DDLoadBitmap(nullptr,Credits,0,0);
        if(!lpDDSCredits)throw std::runtime_error("Missing bitmap resource: Credits");
        DDSetColorKey(lpDDSCredits,RGB(255,0,255));
        lpDDSLogo=DDLoadBitmap(nullptr,Logo,0,0);
        if(!lpDDSLogo)throw std::runtime_error("Missing bitmap resource: Logo");
        DDSetColorKey(lpDDSLogo,RGB(255,0,255));
        lpDDSTextFeld=DDLoadBitmap(nullptr,TextFeld,0,0);
        if(!lpDDSTextFeld)throw std::runtime_error("Missing bitmap resource: TextFeld");
        DDSetColorKey(lpDDSTextFeld,RGB(255,0,255));
        DDSetColorKey(lpDDSCredits,RGB(0,0,0));
        lpDDSKarte=new NativeSurface(2*MAXXKACH,2*MAXYKACH);
        lpDDSScape=new NativeSurface(MAXSCAPEX,MAXSCAPEY);
        lpDDSSchrift=new NativeSurface(MAXX,MAXY);
        lpDDSSchatzkarte=new NativeSurface(SKARTEX,SKARTEY);
        DDSetColorKey(lpDDSSchrift,RGB(255,0,255));
        lpDDSSchrift->GetPixelFormat(&ddpf);
        ddbltfx.dwSize=sizeof(ddbltfx);
        Renderer::GradeTerrain(lpDDSMisc);Renderer::GradeTerrain(lpDDSAnimation);Renderer::GradeTerrain(lpDDSBaum);
    }

    void Reset()
    {
        Button0down = false;
        Button1down = false;
    }
    void ClearInput(){queuedKeys.fill(false);Reset();}

    void CheckMouse()
    {
        const auto screenPoint=NativeDisplay::MousePoint();
        if(ModernUI::OverlayActive()||screenPoint.x<0||screenPoint.x>=960||screenPoint.y<64||screenPoint.y>=656)return;
        const auto point=SceneView::ToScene(screenPoint.x,screenPoint.y);
        const short xDiff=MousePosition.x-point.x,yDiff=MousePosition.y-point.y;
        MousePosition.x=static_cast<short>(point.x);MousePosition.y=static_cast<short>(point.y);
        if(ModernUI::OverlayActive()||point.x<0||point.x>=960||point.y<64||point.y>=656)return;
        if (TwoClicks == -1)
        {
            if (Guy.Aktiv)
            {
                if (Math::InRect(MousePosition.x, MousePosition.y, Bmp[BUTTSTOP].rcDes) && (Bmp[BUTTSTOP].Phase != -1))
                    CursorTyp = CUPFEIL;
                else
                    CursorTyp = CUUHR;
            }
            else
                CursorTyp = CUPFEIL;
        }

        short Button = -1; // Welcher Knopf ist gedrückt worden
        short Push = 0; // Knopf gedrückt(1) oder losgelassen(-1) oder gedrückt(0) gehalten

        if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
        {
            Button = 1;
            if (Button1down)
                Push = 0;
            else
            {
                Push = 1;
                Button1down = true;
            }
        }
        else
        {
            if (Button1down)
            {
                Button = 1;
                Push = -1;
                Button1down = false;
            }
        }

        // Wenn ein Text steht, dann bei Mausdruck Text weg 
        if (PapierText != -1)
        {
            if (Frage == 0)
            {
                if (Math::InRect(MousePosition.x, MousePosition.y, Bmp[JA].rcDes))
                {
                    CursorTyp = CUPFEIL;
                    if ((Button == 0) && (Push == 1))
                    {
                        Frage = 1;
                        Renderer::Textloeschen(TXTPAPIER);
                        PapierText = -1;
                        Guy.Aktiv = false;
                        PlaySound(Sound::CLICK2, 100);
                    }
                }
                else if (Math::InRect(MousePosition.x, MousePosition.y, Bmp[NEIN].rcDes))
                {
                    CursorTyp = CUPFEIL;
                    if ((Button == 0) && (Push == 1))
                    {
                        Frage = 2;
                        Renderer::Textloeschen(TXTPAPIER);
                        PapierText = -1;
                        Guy.Aktiv = false;
                        PlaySound(Sound::CLICK2, 100);
                    }
                }
                else if ((Button == 0) && (Push == 1))
                    PlaySound(Sound::CLICK, 100);
            }
            else if ((Button != -1) && (Push == 1))
            {
                Renderer::Textloeschen(TXTPAPIER);
                PapierText = -1;
                Guy.Aktiv = false;
            }
            return;
        }

        // Animationen und Text löschen (werden dann von den MouseIn.. Sachen neu angestellt
        Renderer::Textloeschen(TXTTEXTFELD);
        Math::ButtAniAus();

        // Wenn der Guy aktiv dann linke Mouse-Buttons ignorieren
        if ((Guy.Aktiv) && (Button == 0))
        {
            if (!(Math::InRect(MousePosition.x, MousePosition.y, Bmp[BUTTSTOP].rcDes)) ||
                (Bmp[BUTTSTOP].Phase == -1))
                Button = -1;
        }

        // die Maus ist in der Spielflaeche ->
        if (Math::InRect(MousePosition.x, MousePosition.y, rcSpielflaeche))
            Math::MouseInSpielflaeche(Button, Push, xDiff, yDiff);
        // die Maus ist im Panel ->
        if (Math::InRect(MousePosition.x, MousePosition.y, rcPanel))
            Math::MouseInPanel(Button, Push);
    }

    short CheckKey()
    {
        const auto keyEvents = queuedKeys;
        queuedKeys.fill(false);
        const auto pressed = [&](sf::Keyboard::Key key)
        {
            return keyEvents[key];
        };
        // Sample every frame, including non-game states, so held keys never repeat.
        const bool journalKey=pressed(sf::Keyboard::F1), effectsKey=pressed(sf::Keyboard::F2), hudKey=pressed(sf::Keyboard::F3);
        const bool waterKey=pressed(sf::Keyboard::K), bandageKey=pressed(sf::Keyboard::J);
        const bool gridKey=pressed(sf::Keyboard::G), soundKey=pressed(sf::Keyboard::S);
        const bool escapeKey=pressed(sf::Keyboard::Escape), enterKey=pressed(sf::Keyboard::Return), spaceKey=pressed(sf::Keyboard::Space), restartKey=pressed(sf::Keyboard::F11);
        if (Spielzustand == State::LOGO)
        {
            if (escapeKey || enterKey ||
                spaceKey) // Logo Abbrechen
            {
                StopSound(Sound::LOGO);
                Game::NeuesSpiel(false);
                return 2;
            }
        }
        else if (Spielzustand == State::INTRO)
        {
            if (escapeKey || enterKey ||
                spaceKey) // Intro Abbrechen
            {
                StopSound(Sound::STORM); // Sound hier sofort stoppen
                StopSound(Sound::SWIM); // Sound hier sofort stoppen
                Guy.Aktiv = false;
                for (short x = Guy.Pos.x; x < MAXXKACH; x++)
                {
                    Guy.Pos.x = x;
                    World::Entdecken();
                    if (Scape[Guy.Pos.x][Guy.Pos.y].Art != 1)
                        break;
                }
                Scape[Guy.Pos.x - 2][Guy.Pos.y].Objekt = WRACK;
                Scape[Guy.Pos.x - 2][Guy.Pos.y].ObPos.x = static_cast<short>(Bmp[WRACK].rcDes.left);
                Scape[Guy.Pos.x - 2][Guy.Pos.y].ObPos.y = static_cast<short>(Bmp[WRACK].rcDes.top);

                Guy.PosScreen.x =
                (Scape[Guy.Pos.x][Guy.Pos.y].xScreen + EckKoor[Scape[Guy.Pos.x][Guy.Pos.y].Typ][0].x +
                    Scape[Guy.Pos.x][Guy.Pos.y].xScreen + EckKoor[Scape[Guy.Pos.x][Guy.Pos.y].Typ][2].x) / 2;
                Guy.PosScreen.y =
                (Scape[Guy.Pos.x][Guy.Pos.y].yScreen + EckKoor[Scape[Guy.Pos.x][Guy.Pos.y].Typ][1].y +
                    Scape[Guy.Pos.x][Guy.Pos.y].yScreen + EckKoor[Scape[Guy.Pos.x][Guy.Pos.y].Typ][3].y) / 2;
                RouteStart.x = -1;
                RouteStart.y = -1;
                RouteZiel.x = -1;
                RouteZiel.y = -1;
                Camera.x = Guy.PosScreen.x - static_cast<short>(rcSpielflaeche.right / 2);
                Camera.y = Guy.PosScreen.y - static_cast<short>(rcSpielflaeche.bottom / 2);
                if (BootsFahrt) World::ChangeBootsFahrt();
                Guy.Zustand = GUYLINKS;
                Guy.Aktion = Action::NOTHING;
                Spielzustand = State::GAME;
                Guy.PosAlt = Guy.PosScreen;
                Game::SaveGame();
                return 1;
            }
        }
        else if (Spielzustand == State::RESCUED)
        {
            if (escapeKey || enterKey ||
                spaceKey)
            {
                Spielzustand = State::OUTRO;
                return 1;
            }
        }
        else if (Spielzustand == State::GAME)
        {
            if (journalKey) Expedition::ToggleJournal();
            if (effectsKey) Expedition::ToggleEffects();
            if (hudKey) Expedition::ToggleHud();
            if (waterKey) Expedition::UseWaterskin();
            if (bandageKey) Expedition::UseBandage();
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                Camera.x += 10;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                Camera.x -= 10;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                Camera.y += 10;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                Camera.y -= 10;
            if (restartKey)
            {
                Guy.AkNummer = 0;
                Guy.Aktiv = false;
                Guy.Aktion = Action::RESTART;
            }
            if (gridKey)
            {
                Gitter = !Gitter;
                World::Generate();
            }

            // Development
            /*
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
            {
                short x,y;
                for (y=0;y<MAXYKACH;y++)
                for (x=0;x<MAXXKACH;x++)
                Scape[x][y].Entdeckt = true;
                World::Generate();
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::I))
            {
                Guy.Inventar[ROHAST] = 10;
                Guy.Inventar[ROHSTEIN] = 10;
                Guy.Inventar[ROHBLATT] = 10;
                Guy.Inventar[ROHLIANE] = 10;
                Guy.Inventar[ROHSTAMM] = 9;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            {
                Guy.Inventar[ROHAXT]   = 1;
                Guy.Inventar[ROHEGGE]  = 1;
                Guy.Inventar[ROHANGEL]  = 1;
                Guy.Inventar[ROHHAMMER]   = 1;
                Guy.Inventar[ROHFERNROHR] = 1;
                Guy.Inventar[ROHSTREICHHOLZ] = 1;
                Guy.Inventar[ROHSCHAUFEL] = 1;
                Guy.Inventar[ROHKARTE] = 1;
                Guy.Inventar[ROHSCHLEUDER] = 1;

                Bmp[BUTTFAELLEN].Phase  = 0;
                Bmp[BUTTANGELN].Phase  = 0;
                Bmp[BUTTANZUENDEN].Phase  = 0;
                Bmp[BUTTAUSSCHAU].Phase = 0;
                Bmp[BUTTSCHATZKARTE].Phase = 0;
                Bmp[BUTTSCHATZ].Phase = 0;
                Bmp[BUTTSCHLEUDER].Phase = 0;
                Bmp[BUTTFELD].Phase  = 0;
                Bmp[BUTTBOOT].Phase  = 0;
                Bmp[BUTTROHR].Phase  = 0;
                Bmp[BUTTHAUS1].Phase = 0;
                Bmp[BUTTHAUS2].Phase = 0;
                Bmp[BUTTHAUS3].Phase = 0;
            }*/

            if (soundKey)
            {
                if (Soundzustand == 0)
                    Soundzustand = 1;
                else if (Soundzustand == 1)
                    Soundzustand = 0;
            }
        }
        else if (Spielzustand == State::OUTRO)
        {
            if (escapeKey || enterKey ||
                spaceKey)
            {
                StopSound(Sound::OUTRO);
                return 0;
            }
        }
        return 1;
    }
} // namespace Direct
