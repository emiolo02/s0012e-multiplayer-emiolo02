#pragma once
#include "input/keyboard.h"

namespace Game {
    struct KeyMap {
        uint16 mask = 0;
        uint64 timeSet = 0;

        bool W() const { return mask & 1; }
        bool A() const { return mask & 2; }
        bool D() const { return mask & 4; }
        bool Up() const { return mask & 8; }
        bool Down() const { return mask & 16; }
        bool Left() const { return mask & 32; }
        bool Right() const { return mask & 64; }
        bool Space() const { return mask & 128; }
        bool Shift() const { return mask & 256; }

        // Returns true if input has changed since last update.
        bool Set(const Input::Keyboard *kbd) {
            timeSet = Time::Now();
            const uint16 newMask =
                    kbd->held[Input::Key::W] | kbd->held[Input::Key::A] << 1 |
                    kbd->held[Input::Key::D] << 2 | kbd->held[Input::Key::Up] << 3 |
                    kbd->held[Input::Key::Down] << 4 | kbd->held[Input::Key::Left] << 5 |
                    kbd->held[Input::Key::Right] << 6 | kbd->pressed[Input::Key::Space] << 7 |
                    kbd->held[Input::Key::Shift] << 8;

            const bool changed = newMask != mask;
            mask = newMask;
            return changed;
        }
    };
}
