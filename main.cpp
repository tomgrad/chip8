#include <array>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <SFML/Graphics.hpp>
#include <fmt/format.h>

using std::array;
using std::fill;
using u8 = uint8_t;
using u16 = uint16_t;
using fmt::print;

class Chip8
{
    u16 PC;          // program counter
    u16 SP;          // stack pointer
    array<u8, 16> V; // registers
    u16 I;           // addr. register
    array<u8, 4096> memory;
    array<u8, 16> stack;
    array<u8, 64 * 32> gfx; // display
    u8 DT;                  // delay timer
    u8 ST;                  // sound timer
    array<u8, 16> key;      //keyboard

public:
    Chip8() : PC(0x200), SP(0), I(0), DT(0), ST(0)
    {
        fill(begin(V), end(V), 0);
        fill(begin(memory), end(memory), 0);
        fill(begin(stack), end(stack), 0);
        fill(begin(gfx), end(gfx), 0);
        fill(begin(key), end(key), 0);

        array<u8, 80> charset{
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80, // F
        };

        std::copy(begin(charset), end(charset), begin(memory));
    }

    void load_program(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        std::vector<u8> buffer((std::istreambuf_iterator<char>(file)), {});
        std::copy(begin(buffer), end(buffer), begin(memory) + 0x200);
    }

    void cycle()
    {
        // Fetch Opcode
        auto opcode = memory[PC] << 8 | memory[PC + 1];
        print("{:x} at {:x}\n", opcode, PC);

        // Decode Opcode
        // Execute Opcode
        PC += 2;

        // Update timers
        if (DT > 0)
            --DT;

        if (ST > 0)
        {
            if (ST == 1)
                printf("BEEP!\n");
            --ST;
        }
    }
};

int main()
{
    print("Chip8 emulator by tomgrad (doctor8bit)\n");
    Chip8 emu;
    emu.load_program("roms/ParticleDemo.ch8");

    for (int i = 0; i < 16; ++i)
    {
        emu.cycle();
    }

    // const auto width = 64u;
    // const auto height = 32u;

    // sf::RenderWindow window(sf::VideoMode(640, 320), "Chip8");

    // sf::Texture fb;
    // fb.create(width, height);

    // auto pixels = new sf::Uint8[width * height * 4];

    // auto color = 0;

    // for (auto i = 0; i < width * height * 4; ++i)
    //     pixels[i] = (++i) % 0xff;

    // fb.update(pixels);

    // sf::Sprite display;
    // display.scale(10, 10);

    // display.setTexture(fb);
    // while (window.isOpen())
    // {
    //     sf::Event event;
    //     while (window.pollEvent(event))
    //     {
    //         if (event.type == sf::Event::Closed)
    //             window.close();
    //     }

    //     window.clear();

    //     for (auto i = 3; i < width * height * 4; i += 4)
    //         ++pixels[i];
    //     fb.update(pixels);

    //     window.draw(display);
    //     window.display();
    // }

    return 0;
}