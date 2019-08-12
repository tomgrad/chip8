#include <array>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <SFML/Graphics.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <random>

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
    array<u16, 16> stack;
    u8 DT;             // delay timer
    u8 ST;             // sound timer
    array<u8, 16> key; //keyboard
    std::default_random_engine rng;

public:
    array<u8, 64 * 32> gfx; // display
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

    bool put_pixel(u8 x, u8 y)
    {
        auto pos = (x + y * 64);
        auto prev = gfx[pos];
        gfx[pos] ^= 0xff;
        return prev == 0xff && gfx[pos] == 0;
    }

    u8 random_byte()
    {
        std::uniform_int_distribution<u8> distribution(0, 0xff);
        return distribution(rng);
    }

    void cycle()
    {
        // Fetch Opcode
        auto opcode = memory[PC] << 8 | memory[PC + 1];
        print("{:x} at {:x}\n", opcode, PC);
        auto panic = [&] {
            print("Panic! Unknown opcode {:x} at PC {:x}\n", opcode, PC);
            exit(1);
        };

        // Decode and execute opcode

        switch (opcode & 0xf000)
        {

        case 0x0000:
            switch (opcode & 0x00ff)
            {
            case 0xee: // RET
                PC = stack[--SP];
                break;
            }

            break;

        case 0x1000: // JP addr
            PC = opcode & 0x0fff;
            break;

        case 0x2000: // CALL addr
            stack[SP++] = PC;
            PC = opcode & 0x0fff;
            break;

        case 0x6000: // LD Vx, byte
            V[(opcode & 0x0f00 >> 8)] = opcode & 0x00ff;
            PC += 2;
            break;

        case 0x7000: // ADD Vx, byte
            V[opcode & 0x0f00 >> 8] += opcode & 0x00ff;
            PC += 2;
            break;

        case 0x8000: // 0x8xyn
        {
            u8 x = opcode & 0x0f00 >> 8;
            u8 y = opcode & 0x00f0 >> 4;

            switch (opcode & 0x000f)
            {
            case 0x0: // LD Vx, Vy
                V[opcode & 0x0f00 >> 8] = V[opcode & 0x00f0 >> 4];
                break;

            case 0x2: // AND Vx, Vy
                V[x] &= V[y];
                break;

            case 0x4: //ADD Vx, Vy
            {
                u16 sum = x + y;
                V[x] = sum & 0x00ff;
                V[0xf] = sum & 0xff00 ? 1 : 0;
            }
            break;

            case 0xe: //SHL Vx {, Vy}
                V[0xf] = V[x] >> 7;
                V[x] <<= 1;
                break;
            default:
                panic();
            }
        }
            PC += 2;

            break;

        case 0xa000: // LD I, addr
            I = opcode & 0x0fff;
            PC += 2;
            break;

        case 0xc000: // RND Vx, byte
            V[opcode & 0x0f00 >> 8] = random_byte() & (opcode & 0x00ff);
            PC += 2;
            break;

        case 0xd000: // DRW Vx, Vy, nibble
        {
            u8 x0 = opcode & 0x0f00 >> 8;
            u8 y0 = opcode & 0x00f0 >> 4;
            u8 n = opcode & 0x000f;
            u8 line = 0;
            for (auto it = begin(memory) + I; it != begin(memory) + I + n; ++it, ++line)
            {
                u8 y = y0 + line;
                for (u8 x = x0; x < x0 + 8; ++x)
                    if (put_pixel(x % 64, y % 32))
                        V[0xf] = 1;
            }
        }

            PC += 2;
            break;

        case 0xf000:
            switch (opcode & 0x00ff)
            {

            case 0x1e: // ADD I, Vx
                I += V[opcode & 0x0f00 >> 8];
                break;

            case 0x65: //Fx65 - LD Vx, [I]
                std::copy(begin(memory) + I, begin(memory) + I + 1 + (opcode & 0x0f00 >> 8), begin(V));
                break;
            default:
                panic();
            }
            PC += 2;
            break;

        default:
            panic();
        }

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

#include <numeric>
#include <valarray>

int main()
{
    print("Chip8 emulator by tomgrad (doctor8bit)\n");
    const auto width = 64u;
    const auto height = 32u;

    Chip8 emu;
    emu.load_program("roms/ParticleDemo.ch8");
    sf::RenderWindow window(sf::VideoMode(640, 320), "Chip8");
    sf::Texture fb;
    fb.create(width, height);

    // while (true)
    //     emu.cycle();

    sf::Sprite display;
    display.scale(10, 10);
    display.setTexture(fb);
    auto pixels = new sf::Uint8[width * height * 4];

    auto update = [&] {
        for (size_t i = 0; i < width * height; ++i)
        {
            pixels[i << 2] = 0;
            pixels[(i << 2) + 1] = emu.gfx[i] ? 255 : 0;
            pixels[(i << 2) + 2] = 0;
            pixels[(i << 2) + 3] = 255;
        }
        fb.update(pixels);
    };

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        emu.cycle();
        window.clear();
        update();
        window.draw(display);
        window.display();
    }

    return 0;
}