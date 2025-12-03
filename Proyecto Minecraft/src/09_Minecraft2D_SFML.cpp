#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <array>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

// Ejemplo 2D tipo "Minecraft" usando SFML con físicas básicas solo para el jugador
// Características añadidas:
// - Física vertical: gravedad, salto, velocidad y colisión con tiles sólidos
// - Mapa más grande y una cueva/túnel subterráneo

const int W = 40;
const int H = 20;
const int TILE = 32;

enum Block : char { AIR = ' ', GRASS = 'G', DIRT = 'D', STONE = 'S', WOOD = 'W', BEDR = 'B' };

using World = std::vector<std::string>;

struct Player {
    float px, py; // posición en píxeles
    float vx, vy; // velocidad en píxeles/s
    int fx, fy;   // dirección de mirada (-1/0/1 en x, y)
    char selected;
    std::map<char,int> inv;
    float w, h; // tamaño del rectángulo del jugador
};

bool in_bounds(int x,int y){ return x>=0 && x<W && y>=0 && y<H; }
bool isSolid(char b){ return b!=(char)AIR; }

char get_block(const World &w, int x,int y){ if(!in_bounds(x,y)) return (char)BEDR; return w[y][x]; }
void set_block(World &w,int x,int y,char b){ if(in_bounds(x,y)) w[y][x]=b; }

void init_world(World &world) {
    world.assign(H, std::string(W, (char)AIR));
    int ground = H - 5;
    for (int y = ground; y < H-1; ++y) {
        for (int x = 0; x < W; ++x) {
            if (y == ground) world[y][x] = (char)GRASS;
            else if (y < H-2) world[y][x] = (char)DIRT;
            else world[y][x] = (char)STONE;
        }
    }
    for (int x = 0; x < W; ++x) world[H-1][x] = (char)BEDR;

    // Árboles simples dispersos
    for (int t = 2; t < W-2; t+=8) {
        int tx = t; int ty = ground-1;
        if (ty-2 >= 0 && tx>0 && tx<W) {
            world[ty][tx] = (char)WOOD;
            world[ty-1][tx] = (char)WOOD;
            world[ty-2][tx] = (char)WOOD;
        }
    }

    // Cueva simple: abrir túnel horizontal bajo la superficie
    int caveY = ground + 2;
    for (int x = 10; x < 30; ++x) {
        if (caveY < H-1) world[caveY][x] = (char)AIR;
        if (caveY+1 < H-1 && (x%5==0)) world[caveY+1][x] = (char)AIR; // ampliaciones
    }
    // entrada a la cueva
    world[ground+1][10] = (char)AIR;
    world[ground+2][10] = (char)AIR;
}

// Helpers para detección de colisiones AABB -> tiles
void resolveHorizontal(World &world, Player &p, float newPx) {
    float left = newPx;
    float right = newPx + p.w - 1;
    int topTile = std::floor(p.py / TILE);
    int bottomTile = std::floor((p.py + p.h - 1) / TILE);
    int leftTile = std::floor(left / TILE);
    int rightTile = std::floor(right / TILE);
    if (p.vx > 0) {
        for (int tx = rightTile; tx <= rightTile; ++tx) {
            for (int ty = topTile; ty <= bottomTile; ++ty) {
                if (in_bounds(tx,ty) && isSolid(get_block(world,tx,ty))) {
                    p.px = tx * TILE - p.w; p.vx = 0; return;
                }
            }
        }
    } else if (p.vx < 0) {
        for (int tx = leftTile; tx >= leftTile; --tx) {
            for (int ty = topTile; ty <= bottomTile; ++ty) {
                if (in_bounds(tx,ty) && isSolid(get_block(world,tx,ty))) {
                    p.px = (tx+1) * TILE; p.vx = 0; return;
                }
            }
        }
    }
    p.px = newPx;
}

void resolveVertical(World &world, Player &p, float newPy) {
    float top = newPy;
    float bottom = newPy + p.h - 1;
    int leftTile = std::floor(p.px / TILE);
    int rightTile = std::floor((p.px + p.w - 1) / TILE);
    int topTile = std::floor(top / TILE);
    int bottomTile = std::floor(bottom / TILE);
    if (p.vy > 0) { // falling
        for (int ty = bottomTile; ty <= bottomTile; ++ty) {
            for (int tx = leftTile; tx <= rightTile; ++tx) {
                if (in_bounds(tx,ty) && isSolid(get_block(world,tx,ty))) {
                    p.py = ty * TILE - p.h; p.vy = 0; return;
                }
            }
        }
    } else if (p.vy < 0) { // rising
        for (int ty = topTile; ty >= topTile; --ty) {
            for (int tx = leftTile; tx <= rightTile; ++tx) {
                if (in_bounds(tx,ty) && isSolid(get_block(world,tx,ty))) {
                    p.py = (ty+1) * TILE; p.vy = 0; return;
                }
            }
        }
    }
    p.py = newPy;
}

// (No enemy-specific resolvers needed for now)

int main(){
    World world;
    init_world(world);

    Player p{};
    p.w = TILE-6; p.h = TILE-6;
    p.px = (W/2) * TILE; p.py = (H-6) * TILE; p.vx = 0; p.vy = 0; p.fx = 1; p.fy = 0; p.selected = (char)GRASS;
    p.inv[(char)GRASS]=10; p.inv[(char)DIRT]=8; p.inv[(char)STONE]=6; p.inv[(char)WOOD]=3; p.inv[(char)BEDR]=0;

    sf::RenderWindow window(sf::VideoMode(W*TILE, H*TILE + 80), "Minecraft2D - SFML (Fisicas)");
    window.setFramerateLimit(60);

    std::map<char, sf::Color> color {
        {(char)AIR, sf::Color(135,206,235)},
        {(char)GRASS, sf::Color(88, 166, 72)},
        {(char)DIRT, sf::Color(134, 96, 67)},
        {(char)STONE, sf::Color(120,120,120)},
        {(char)WOOD, sf::Color(150, 111, 51)},
        {(char)BEDR, sf::Color(40,40,40)}
    };

    sf::Font font;
    font.loadFromFile("assets/fonts/Minecraft.ttf");

    // Música de fondo (C418) — si existe en assets/music
    sf::Music bgm;
    if (bgm.openFromFile("assets/music/C418-Subwoofer-Lullaby-Minecraft-Volume-Alpha.ogg")) {
        bgm.setLoop(true);
        bgm.setVolume(40);
        bgm.play();
    }

    sf::RectangleShape tileShape(sf::Vector2f(TILE, TILE));
    sf::RectangleShape playerShape(sf::Vector2f(p.w, p.h));
    playerShape.setFillColor(sf::Color::Yellow);

    // No enemy: eliminados para simplificar

    const float GRAVITY = 1500.0f; // px/s^2
    const float MOVE_SPEED = 150.0f; // px/s
    const float JUMP_SPEED = 520.0f; // px/s

    sf::Clock clock;
    // Picar bloques por tiempo
    bool breaking = false;
    int breakX = -1, breakY = -1;
    float breakProgress = 0.0f;
    const float BASE_BREAK_TIME = 0.8f; // segundos para bloques normales
    while (window.isOpen()){
        sf::Event ev;
        while (window.pollEvent(ev)){
            if (ev.type == sf::Event::Closed) window.close();
            if (ev.type == sf::Event::KeyPressed){
                if (ev.key.code == sf::Keyboard::Escape) window.close();
                if (ev.key.code == sf::Keyboard::Num1) p.selected=(char)GRASS;
                if (ev.key.code == sf::Keyboard::Num2) p.selected=(char)DIRT;
                if (ev.key.code == sf::Keyboard::Num3) p.selected=(char)STONE;
                if (ev.key.code == sf::Keyboard::Num4) p.selected=(char)WOOD;
                if (ev.key.code == sf::Keyboard::Num5) p.selected=(char)BEDR;
                // tecla X ahora inicia picar (mecánica por tiempo) — manejado en el bucle principal
                if (ev.key.code == sf::Keyboard::C) {
                    int centerX = static_cast<int>(p.px + p.w/2);
                    int centerY = static_cast<int>(p.py + p.h/2);
                    int tx = (centerX + p.fx * TILE) / TILE;
                    int ty = (centerY + p.fy * TILE) / TILE;
                    char b = p.selected;
                    if (in_bounds(tx,ty) && get_block(world,tx,ty)==(char)AIR && p.inv[b]>0){ p.inv[b]--; set_block(world,tx,ty,b); }
                }
                if (ev.key.code == sf::Keyboard::W || ev.key.code == sf::Keyboard::Space || ev.key.code == sf::Keyboard::Up) {
                    // Salto: solo si estamos sobre suelo (pequeña comprobación)
                    int belowTileY = static_cast<int>(std::floor((p.py + p.h + 1) / TILE));
                    int leftTile = static_cast<int>(std::floor(p.px / TILE));
                    int rightTile = static_cast<int>(std::floor((p.px + p.w -1) / TILE));
                    bool onGround = false;
                    for (int tx = leftTile; tx <= rightTile; ++tx) if (in_bounds(tx,belowTileY) && isSolid(get_block(world,tx,belowTileY))) onGround = true;
                    if (onGround) { p.vy = -JUMP_SPEED; }
                }
            }
            if (ev.type == sf::Event::MouseButtonPressed){
                // click handling: colocar con botón derecho (inmediato). Picar con botón izquierdo ahora se maneja manteniendo pulsado (ver loop principal).
                sf::Vector2i m = sf::Mouse::getPosition(window);
                int mx = m.x / TILE; int my = m.y / TILE;
                if (ev.mouseButton.button == sf::Mouse::Right){
                    if (in_bounds(mx,my)){
                        char b = p.selected;
                        if (get_block(world,mx,my)==(char)AIR && p.inv[b]>0){ p.inv[b]--; set_block(world,mx,my,b); }
                    }
                }
            }
        }

        float dt = clock.restart().asSeconds();

        // Input horizontal
        float targetVx = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) { targetVx = -MOVE_SPEED; p.fx = -1; }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { targetVx = MOVE_SPEED; p.fx = 1; }
        else { targetVx = 0; }
        p.vx = targetVx;

        // Apply gravity
        p.vy += GRAVITY * dt;
        if (p.vy > 2000.0f) p.vy = 2000.0f;

        // Move horizontally and resolve collisions
        float newPx = p.px + p.vx * dt;
        resolveHorizontal(world, p, newPx);

        // Move vertically and resolve collisions
        float newPy = p.py + p.vy * dt;
        resolveVertical(world, p, newPy);

        // update facing y
        p.fy = (p.vy > 0) ? 1 : (p.vy < 0 ? -1 : 0);

        // --- Mecánica de picar por tiempo ---
        bool keyBreak = sf::Keyboard::isKeyPressed(sf::Keyboard::X);
        bool mouseBreak = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        int targetX = -1, targetY = -1;
        if (keyBreak) {
            int centerX = static_cast<int>(p.px + p.w/2);
            int centerY = static_cast<int>(p.py + p.h/2);
            targetX = (centerX + p.fx * TILE) / TILE;
            targetY = (centerY + p.fy * TILE) / TILE;
        } else if (mouseBreak) {
            sf::Vector2i mpos = sf::Mouse::getPosition(window);
            targetX = mpos.x / TILE; targetY = mpos.y / TILE;
        }

        if (targetX != -1 && in_bounds(targetX, targetY)) {
            char tb = get_block(world, targetX, targetY);
            if (tb != (char)AIR && tb != (char)BEDR) {
                // determine break time modifier by block type
                float mult = 1.0f;
                if (tb == (char)STONE) mult = 2.0f;
                else if (tb == (char)WOOD) mult = 0.8f;

                if (breaking && breakX == targetX && breakY == targetY) {
                    breakProgress += dt;
                } else {
                    breaking = true;
                    breakX = targetX; breakY = targetY; breakProgress = dt;
                }

                float need = BASE_BREAK_TIME * mult;
                if (breakProgress >= need) {
                    // completar ruptura
                    p.inv[tb]++;
                    set_block(world, breakX, breakY, (char)AIR);
                    breaking = false; breakX = breakY = -1; breakProgress = 0.0f;
                }
            } else {
                // objetivo no picable
                breaking = false; breakX = breakY = -1; breakProgress = 0.0f;
            }
        } else {
            // no está picando
            breaking = false; breakX = breakY = -1; breakProgress = 0.0f;
        }

        // (No enemies active)

        window.clear(sf::Color(135,206,235));

        // draw world
        for (int y=0;y<H;++y){
            for (int x=0;x<W;++x){
                char b = get_block(world,x,y);
                sf::Color col = color.count(b) ? color[b] : sf::Color::Magenta;
                tileShape.setPosition(x*TILE, y*TILE);
                tileShape.setFillColor(col);
                window.draw(tileShape);
            }
        }

        // mostrar progreso de picar si aplica
        if (breaking && breakX>=0 && breakY>=0) {
            sf::RectangleShape overlay(sf::Vector2f(TILE, TILE));
            overlay.setPosition(breakX * TILE, breakY * TILE);
            overlay.setFillColor(sf::Color(0,0,0,80));
            window.draw(overlay);
            // barra de progreso
            char tb = get_block(world, breakX, breakY);
            float mult = 1.0f;
            if (tb == (char)STONE) mult = 2.0f; else if (tb == (char)WOOD) mult = 0.8f;
            float need = BASE_BREAK_TIME * mult;
            float ratio = std::min(1.0f, breakProgress / (need + 1e-6f));
            sf::RectangleShape barBg(sf::Vector2f(TILE-6, 8));
            barBg.setPosition(breakX * TILE + 3, breakY * TILE + TILE - 12);
            barBg.setFillColor(sf::Color(0,0,0,160));
            window.draw(barBg);
            sf::RectangleShape bar(sf::Vector2f((TILE-6) * ratio, 8));
            bar.setPosition(breakX * TILE + 3, breakY * TILE + TILE - 12);
            bar.setFillColor(sf::Color::Green);
            window.draw(bar);
        }

        // draw player
        playerShape.setPosition(p.px, p.py);
        window.draw(playerShape);

        // (No enemies to draw)

        // HUD
        sf::RectangleShape hudBg(sf::Vector2f(W*TILE, 80));
        hudBg.setPosition(0, H*TILE);
        hudBg.setFillColor(sf::Color(30,30,30,200));
        window.draw(hudBg);

        // inventory
        for (int i=0;i<5;++i){
            char mapSel[5] = {(char)GRASS,(char)DIRT,(char)STONE,(char)WOOD,(char)BEDR};
            char b = mapSel[i];
            sf::RectangleShape slot(sf::Vector2f(48,48));
            slot.setPosition(10 + i*60, H*TILE + 16);
            slot.setFillColor(color[b]);
            if (b==p.selected) { slot.setOutlineThickness(3); slot.setOutlineColor(sf::Color::Yellow); }
            else { slot.setOutlineThickness(1); slot.setOutlineColor(sf::Color::Black); }
            window.draw(slot);
            sf::Text t(std::to_string(p.inv[b]), font, 16);
            t.setFillColor(sf::Color::White);
            t.setPosition(10 + i*60 + 28, H*TILE + 48);
            window.draw(t);
        }

        sf::Text instr("W/Space: saltar  A/D: mover  X: picar  C: colocar  1-5: seleccionar", font, 14);
        instr.setFillColor(sf::Color::White);
        instr.setPosition(10, H*TILE + 4);
        window.draw(instr);

        // (No HUD de vida ni manejo de Game Over en esta versión)

        window.display();
    }
    return 0;
}
