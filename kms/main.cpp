#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>
#include <cmath>

using json = nlohmann::json;

void settingsWindow(json& settings, sf::Music& music, sf::RenderWindow& mainWindow, sf::Sprite& sprite, const sf::Vector2u& baseSize);

std::vector<sf::Texture> loadFrames(const std::string& path, int count) {
    std::vector<sf::Texture> v;
    for (int i = 1; i <= count; ++i) {
        sf::Texture t;
        if (!t.loadFromFile(path + "/frame(" + std::to_string(i) + ").png")) {
            std::cerr << "Failed to load " << path << "/frame(" << i << ").png\n";
            continue;
        }
        v.push_back(t);
    }
    return v;
}

void saveSettingsToFile(const json& settings, const std::string& filename = "settings.json") {
    std::ofstream f(filename);
    if (f.is_open()) f << settings.dump(4);
}

json loadSettingsFromFile(const std::string& filename = "settings.json") {
    std::ifstream f(filename);
    json s;
    if (f.is_open()) {
        try { f >> s; }
        catch (...) { std::cerr << "Bad settings.json, using defaults\n"; }
    }
    return s;
}

void centerWindow(sf::RenderWindow& w) {
    sf::VideoMode dm = sf::VideoMode::getDesktopMode();

    int deskW = dm.width;
    int deskH = dm.height;

    sf::Vector2u sz = w.getSize();

    int x = (deskW - static_cast<int>(sz.x)) / 2;
    int y = (deskH - static_cast<int>(sz.y)) / 2;

    w.setPosition(sf::Vector2i(x, y));
}

int main() {
    // load settings
    json settings = loadSettingsFromFile();
    if (settings.is_null()) settings = json::object();
    int savedVolume = settings.value("volume", 20);
    float savedScale = settings.value("scale", 1.0f);

    const std::string framePath = "animation";
    const int frameCount = 297;
    std::vector<sf::Texture> frames = loadFrames(framePath, frameCount);
    if (frames.empty()) return -1;

    sf::Vector2u baseSize = frames[0].getSize();
    unsigned int winW = static_cast<unsigned int>(baseSize.x * savedScale);
    unsigned int winH = static_cast<unsigned int>(baseSize.y * savedScale);

    sf::RenderWindow window(sf::VideoMode(winW, winH), "dobry den", sf::Style::None);

    // overlay/color-key
    HWND hwnd = window.getSystemHandle();
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    centerWindow(window);

    // music
    sf::Music music;
    if (!music.openFromFile("Konata.ogg")) std::cerr << "Couldn't open Konata.ogg\n";
    else {
        music.setLoop(true);
        music.setVolume(static_cast<float>(savedVolume));
        music.play();
    }

    // sprite
    sf::Sprite sprite;
    sprite.setTexture(frames[0]);
    auto updateSpriteScale = [&](unsigned int w, unsigned int h, const sf::Texture& tex) {
        sf::Vector2u ts = tex.getSize();
        float sx = (float)w / ts.x;
        float sy = (float)h / ts.y;
        sprite.setScale(sx, sy);
        };
    updateSpriteScale(winW, winH, frames[0]);

    size_t current = 0;
    sf::Clock clock;
    const int frameDelay = 40;

    bool draggingWindow = false;
    sf::Vector2i dragOffset;

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
            else if (ev.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(ev.size.width), static_cast<float>(ev.size.height))));

            }
            else if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                draggingWindow = true;
                dragOffset = sf::Mouse::getPosition(window);
            }
            else if (ev.type == sf::Event::MouseButtonReleased && ev.mouseButton.button == sf::Mouse::Left) {
                draggingWindow = false;
            }
            else if (ev.type == sf::Event::MouseMoved && draggingWindow) {
                sf::Vector2i m = sf::Mouse::getPosition();
                window.setPosition(m - dragOffset);
            }
            else if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Escape) window.close();
            else if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::F11) {
                settingsWindow(settings, music, window, sprite, baseSize);

                float scale = settings.value("scale", 1.0f);
                int vol = settings.value("volume", static_cast<int>(music.getVolume()));

                //setting width and height
                unsigned int newW = static_cast<unsigned int>(std::round(baseSize.x * scale));
                unsigned int newH = static_cast<unsigned int>(std::round(baseSize.y * scale));

                if (newW != window.getSize().x || newH != window.getSize().y) {
                    window.setSize({ newW, newH });

                    window.setView(sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(newW), static_cast<float>(newH))));

                    HWND hwnd2 = window.getSystemHandle();
                    LONG st2 = GetWindowLong(hwnd2, GWL_EXSTYLE);
                    SetWindowLong(hwnd2, GWL_EXSTYLE, st2 | WS_EX_LAYERED);
                    SetLayeredWindowAttributes(hwnd2, RGB(0, 0, 0), 0, LWA_COLORKEY);
                    SetWindowPos(hwnd2, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                }
                if (music.getStatus() != sf::SoundSource::Stopped) music.setVolume(static_cast<float>(vol));
                updateSpriteScale(window.getSize().x, window.getSize().y, frames[current]);
            }
        }

        if (clock.getElapsedTime().asMilliseconds() > frameDelay) {
            current = (current + 1) % frames.size();
            sprite.setTexture(frames[current], true);
            updateSpriteScale(window.getSize().x, window.getSize().y, frames[current]);
            clock.restart();
        }

        window.clear(sf::Color(0, 0, 0, 0));
        window.draw(sprite);
        window.display();
    }

    saveSettingsToFile(settings);
    return 0;
}

void settingsWindow(json& settings, sf::Music& music, sf::RenderWindow& mainWindow, sf::Sprite& sprite, const sf::Vector2u& baseSize) {
    json orig = settings;
    bool musicOk = (music.getStatus() != sf::SoundSource::Stopped);
    bool wasPlaying = musicOk && (music.getStatus() == sf::SoundSource::Playing);

    if (musicOk) music.pause();

    int volume = settings.value("volume", static_cast<int>(music.getVolume()));
    float scale = settings.value("scale", 1.0f);

    // dropdown choices
    const std::vector<std::pair<std::string, float>> choices = {
        {"1.5x", 1.5f}, {"1x", 1.0f}, {"0.5x", 0.5f}, {"0.2x", 0.2f}
    };

    int selected = 1; // default 1x
    for (size_t i = 0; i < choices.size(); ++i) if (std::abs(choices[i].second - scale) < 1e-6f) selected = (int)i;

    const int SW = 420, SH = 220;
    sf::RenderWindow win(sf::VideoMode(SW, SH), "Settings", sf::Style::Titlebar | sf::Style::Close);
    win.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("Consolas.ttf")) {
        return;
    }

    // UI simple
    sf::Text header("Settings", font, 24);
    header.setPosition(12, 8);
    header.setFillColor(sf::Color::White);

    // volume slider
    sf::FloatRect track(20, 50, 380, 8);
    sf::RectangleShape trackRect(sf::Vector2f(track.width, track.height));

    trackRect.setPosition(track.left, track.top);
    trackRect.setFillColor(sf::Color(70, 70, 80));

    float handleR = 10.f;
    sf::CircleShape handle(handleR); handle.setOrigin(handleR, handleR);

    auto volToX = [&](int v) { return track.left + (v / 100.f) * track.width; };
    auto xToVol = [&](float x) {
        float t = (x - track.left) / track.width;
        t = std::max(0.f, std::min(1.f, t));
        return static_cast<int>(std::round(t * 100.f));
        };

    handle.setPosition(volToX(volume), track.top + track.height / 2.f);
    handle.setFillColor(sf::Color(120, 200, 255));

    sf::Text volumeText("Music Volume", font, 16);
    volumeText.setPosition(20, 30);
    volumeText.setFillColor(sf::Color::White);

    sf::Text volValue(std::to_string(volume), font, 14);
    volValue.setPosition(340, 32);
    volValue.setFillColor(sf::Color::White);

    // dropdown simple (rects)
    sf::Text sizeText("Size multiplier", font, 16);
    sizeText.setPosition(20, 90);
    sizeText.setFillColor(sf::Color::White);

    std::vector<sf::RectangleShape> ddRects;
    std::vector<sf::Text> ddTexts;

    for (size_t i = 0; i < choices.size(); ++i) {
        sf::RectangleShape r(sf::Vector2f(80, 28));
        r.setPosition(20 + (float)i * (90.f), 120);
        r.setFillColor(i == selected ? sf::Color(120, 200, 255) : sf::Color(60, 60, 70));

        ddRects.push_back(r);

        sf::Text t(choices[i].first, font, 16);
        t.setFillColor(i == selected ? sf::Color(10, 10, 20) : sf::Color::White);

        //center
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition(r.getPosition().x + (r.getSize().x - b.width) / 2.f - b.left, r.getPosition().y + (r.getSize().y - b.height) / 2.f - b.top);
        ddTexts.push_back(t);
    }

    //Apply / Cancel buttons
    sf::RectangleShape apply(sf::Vector2f(100, 32)); apply.setPosition(SW - 120, SH - 50);
    apply.setFillColor(sf::Color(120, 200, 255));

    sf::Text applyT("Apply", font, 18); applyT.setFillColor(sf::Color(10, 10, 20));
    applyT.setPosition(apply.getPosition().x + 24, apply.getPosition().y + 4);

    sf::RectangleShape cancel(sf::Vector2f(100, 32)); cancel.setPosition(SW - 240, SH - 50);
    cancel.setFillColor(sf::Color(80, 80, 90));

    sf::Text cancelT("Cancel", font, 18); cancelT.setFillColor(sf::Color::White);
    cancelT.setPosition(cancel.getPosition().x + 18, cancel.getPosition().y + 4);

    bool dragging = false;
    bool running = true;
    while (win.isOpen() && running) {
        sf::Event e;
        while (win.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
                win.close(); running = false;
                if (musicOk && wasPlaying) music.play();
                return;
            }
            else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f m((float)sf::Mouse::getPosition(win).x, (float)sf::Mouse::getPosition(win).y);

                // handle slider
                sf::FloatRect hb(handle.getPosition().x - handleR, handle.getPosition().y - handleR, handleR * 2, handleR * 2);
                if (hb.contains(m)) dragging = true;
                else if (trackRect.getGlobalBounds().contains(m)) {
                    volume = xToVol(m.x);
                    handle.setPosition(volToX(volume), track.top + track.height / 2.f);
                }

                // dropdown
                for (size_t i = 0; i < ddRects.size(); ++i) {
                    if (ddRects[i].getGlobalBounds().contains(m)) {
                        selected = (int)i;
                        // update colors/text colors
                        for (size_t j = 0; j < ddRects.size(); ++j) {
                            ddRects[j].setFillColor(j == selected ? sf::Color(120, 200, 255) : sf::Color(60, 60, 70));
                            ddTexts[j].setFillColor(j == selected ? sf::Color(10, 10, 20) : sf::Color::White);
                        }
                    }
                }

                // buttons
                if (apply.getGlobalBounds().contains(m)) {
                    // apply
                    settings["volume"] = volume;
                    settings["scale"] = choices[selected].second;

                    saveSettingsToFile(settings);

                    if (musicOk) music.setVolume(static_cast<float>(volume));

                    float sc = choices[selected].second;
                    unsigned int newW = static_cast<unsigned int>(std::round(baseSize.x * sc));
                    unsigned int newH = static_cast<unsigned int>(std::round(baseSize.y * sc));

                    mainWindow.setSize({ newW, newH });

                    mainWindow.setView(sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(newW), static_cast<float>(newH))));

                    HWND hwndMain = mainWindow.getSystemHandle();
                    LONG st = GetWindowLong(hwndMain, GWL_EXSTYLE);
                    SetWindowLong(hwndMain, GWL_EXSTYLE, st | WS_EX_LAYERED);
                    SetLayeredWindowAttributes(hwndMain, RGB(0, 0, 0), 0, LWA_COLORKEY);
                    SetWindowPos(hwndMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                    centerWindow(mainWindow);

                    if (sprite.getTexture()) {
                        sf::Vector2u ts = sprite.getTexture()->getSize();
                        float sx = (float)newW / ts.x; float sy = (float)newH / ts.y;
                        sprite.setScale(sx, sy);
                    }

                    if (musicOk) music.play();
                    win.close(); running = false;
                    break;
                }
                if (cancel.getGlobalBounds().contains(m)) {
                    settings = orig;
                    if (musicOk && wasPlaying) music.play();
                    win.close(); running = false;
                    break;
                }
            }
            else if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
                dragging = false;
            }
            else if (e.type == sf::Event::MouseMoved && dragging) {
                float mx = (float)e.mouseMove.x;
                volume = xToVol(mx);
                handle.setPosition(volToX(volume), track.top + track.height / 2.f);
            }
            else if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Escape) {
                    settings = orig;
                    if (musicOk && wasPlaying) {
                        music.play();
                        win.close();
                        running = false;
                        break;
                    }
                }
                if (e.key.code == sf::Keyboard::Enter) {
                    // same as apply
                    settings["volume"] = volume;
                    settings["scale"] = choices[selected].second;

                    saveSettingsToFile(settings);

                    if (musicOk) music.setVolume(static_cast<float>(volume));

                    float sc = choices[selected].second;
                    unsigned int newW = static_cast<unsigned int>(std::round(baseSize.x * sc));
                    unsigned int newH = static_cast<unsigned int>(std::round(baseSize.y * sc));

                    mainWindow.setSize({ newW, newH });

                    if (sprite.getTexture()) {
                        sf::Vector2u ts = sprite.getTexture()->getSize();
                        float sx = (float)newW / ts.x; float sy = (float)newH / ts.y;
                        sprite.setScale(sx, sy);
                    }

                    if (musicOk) music.play();
                    win.close();
                    running = false;
                    break;
                }
            }
        }

        volValue.setString(std::to_string(volume));
        handle.setPosition(volToX(volume), track.top + track.height / 2.f);

        win.clear(sf::Color(30, 30, 36));
        win.draw(header);
        win.draw(volumeText);
        win.draw(trackRect);
        // progress
        sf::RectangleShape prog(sf::Vector2f((volume / 100.f) * track.width, track.height));
        prog.setPosition(track.left, track.top);
        prog.setFillColor(sf::Color(100, 180, 255));
        win.draw(prog);
        win.draw(handle);
        win.draw(volValue);
        win.draw(sizeText);
        for (size_t i = 0; i < ddRects.size(); ++i) { win.draw(ddRects[i]); win.draw(ddTexts[i]); }
        win.draw(apply); win.draw(applyT); win.draw(cancel); win.draw(cancelT);
        win.display();
    }

    if (!musicOk) return;
    if (wasPlaying && music.getStatus() != sf::SoundSource::Playing) music.play();
}