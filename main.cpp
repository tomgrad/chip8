#include <array>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <SFML/Graphics.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <random>
#include <numeric>
#include <valarray>

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
    u8 DT; // delay timer
    u8 ST; // sound timer
    std::default_random_engine rng;

public:
    array<u8, 64 * 32> gfx; // display
    array<u8, 16> key;      //keyboard
    Chip8() : PC(0x200), SP(0), I(0), DT(0), ST(0)
    {
        std::fill(begin(V), end(V), 0);
        std::fill(begin(memory), end(memory), 0);
        std::fill(begin(stack), end(stack), 0);
        std::fill(begin(gfx), end(gfx), 0);
        std::fill(begin(key), end(key), 0);

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
        auto pos = (y * 64 + x);
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
        u8 x = (opcode & 0x0f00) >> 8;
        u8 y = (opcode & 0x00f0) >> 4;

        // print("{:x} at {:x}\n", opcode, PC);
        auto panic = [&] {
            print("{:x} unknown. Panic at PC {:x}\n", opcode, PC);
            exit(1);
        };

        // Decode and execute opcode

        switch (opcode & 0xf000)
        {

        case 0x0000:
            switch (opcode & 0x00ff)
            {
            case 0xee: // RET
                PC = stack[--SP] + 2;
                break;
            case 0xe0: // CLS
                std::fill(begin(gfx), end(gfx), 0);
                PC += 2;
                break;
            default:
                panic();
            }

            break;

        case 0x1000: // JP addr
            PC = opcode & 0x0fff;
            break;

        case 0x2000: // CALL addr
            stack[SP++] = PC;
            PC = opcode & 0x0fff;
            break;

        case 0x3000: // SE Vx, byte
            PC += V[x] == (opcode & 0x00ff) ? 4 : 2;
            break;

        case 0x4000: // SnE Vx, byte
            PC += V[x] == (opcode & 0x00ff) ? 2 : 4;
            break;

        case 0x5000: // SE Vx, Vy
            PC += V[x] == V[y] ? 4 : 2;
            break;

        case 0x6000: // LD Vx, byte
            V[x] = opcode & 0x00ff;
            PC += 2;
            break;

        case 0x7000: // ADD Vx, byte
            V[x] += opcode & 0x00ff;
            PC += 2;
            break;

        case 0x8000: // 0x8xyn

            switch (opcode & 0x000f)
            {
            case 0x0: // LD Vx, Vy
                V[x] = V[y];
                break;

            case 0x1: // OR Vx, Vy
                V[x] |= V[y];
                break;

            case 0x2: // AND Vx, Vy
                V[x] &= V[y];
                break;

            case 0x3: // XOR Vx, Vy
                V[x] ^= V[y];
                break;

            case 0x4: //ADD Vx, Vy
            {
                u16 sum = V[x] + V[y];
                V[x] = sum & 0x00ff;
                V[0xf] = sum & 0xff00 ? 1 : 0;
            }
            break;

            case 0x6: //SHR Vx {, Vy}
                V[0xf] = V[x] & 1;
                V[x] >>= 1;
                break;

            case 0xe: //SHL Vx {, Vy}
                V[0xf] = V[x] >> 7;
                V[x] <<= 1;
                break;
            default:
                panic();
            }

            PC += 2;

            break;

        case 0x9000: //SNE Vx, Vy
        {
            PC += V[x] == V[y] ? 2 : 4;
        }
        break;

        case 0xa000: // LD I, addr
            I = opcode & 0x0fff;
            PC += 2;
            break;

        case 0xc000: // RND Vx, byte
            V[x] = random_byte() & (opcode & 0x00ff);
            PC += 2;
            break;

        case 0xd000: // DRW Vx, Vy, nibble
        {
            V[0xf] = 0;
            u8 n = opcode & 0x000f;
            for (u8 yi = 0; yi < n; ++yi)
            {
                auto line = memory[I + yi];
                for (u8 xi = 0; xi < 8; ++xi)
                {
                    if (line >> 7)
                        if (put_pixel((xi + V[x]) % 64, (yi + V[y]) % 32))
                            V[0xf] = 1;
                    line <<= 1;
                }
            }
        }

            PC += 2;
            break;

        case 0xE000:
            switch (opcode & 0x00ff)
            {
            case 0x9e: // SKP Vx
                PC += key[V[x]] ? 4 : 2;
                break;
            case 0xa1: // SKNP Vx
                PC += key[V[x]] ? 2 : 4;
                break;
            default:
                panic();
            }
            break;
        case 0xf000:
            switch (opcode & 0x00ff)
            {

            case 0x07: // LD Vx, DT
                V[x] = DT;
                break;

            case 0x15: // LD DT, Vx
                DT = V[x];
                break;
            case 0x1e: // ADD I, Vx
                I += V[x];
                break;

            case 0x29: // LD F, Vx
                I = V[x] * 5;
                break;

            case 0x33: // LD B, Vx
            {
                u8 d2 = V[x] / 100;
                u8 d1 = (V[x] - d2 * 100) / 10;
                u8 d0 = V[x] - d2 * 100 - d1 * 10;

                memory[I] = d2;
                memory[I + 1] = d1;
                memory[I + 2] = d0;
            }
            break;

            case 0x55: //Fx55 - LD [I], Vx
                std::copy(begin(V), begin(V) + x + 1, begin(memory) + I);
                break;

            case 0x65: //Fx65 - LD Vx, [I]
                std::copy(begin(memory) + I, begin(memory) + I + x + 1, begin(V));
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

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        print("Usage: chip8 rom.ch8\n");
        exit(0);
    }
    print("Chip8 emulator by tomgrad (doctor8bit)\n");
    const auto width = 64u;
    const auto height = 32u;
    const auto scale = 5u;

    Chip8 emu;
    emu.load_program(argv[1]);
    sf::RenderWindow window(sf::VideoMode(width * scale, height * scale), "Chip8", sf::Style::Close);
    sf::Texture fb;
    fb.create(width, height);

    sf::Sprite display;

    display.setTexture(fb);
    auto pixels = new sf::Uint8[width * height * 4];

    const std::map<sf::Keyboard::Key, u8> bindings = {
        {sf::Keyboard::Num0, 0},
        {sf::Keyboard::Num1, 1},
        {sf::Keyboard::Num2, 2},
        {sf::Keyboard::Num3, 3},
        {sf::Keyboard::Num4, 4},
        {sf::Keyboard::Num5, 5},
        {sf::Keyboard::Num6, 6},
        {sf::Keyboard::Num7, 7},
        {sf::Keyboard::Num8, 8},
        {sf::Keyboard::Num9, 9},
        {sf::Keyboard::A, 10},
        {sf::Keyboard::B, 11},
        {sf::Keyboard::C, 12},
        {sf::Keyboard::D, 13},
        {sf::Keyboard::E, 14},
        {sf::Keyboard::F, 15}};

    auto update = [&] {
        for (size_t i = 0; i < width * height; ++i)
        {
            pixels[4 * i] = 0;
            pixels[4 * i + 1] = emu.gfx[i];
            pixels[4 * i + 2] = 0;
            pixels[4 * i + 3] = 255;
        }
        fb.update(pixels);
    };
    display.setScale(scale, scale);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();
            }
        }

        for (auto const &[key, val] : bindings)
            emu.key[val] =
                sf::Keyboard::isKeyPressed(key) ? 1 : 0;

        emu.cycle();
        window.clear();
        update();
        window.draw(display);
        window.display();
    }

    return 0;
}