/*
 ██████╗██╗   ██╗██████╗ ███████╗███████╗██╗   ██╗███████╗███╗   ██╗████████╗ ██████╗██████╗ ██████╗
██╔════╝██║   ██║██╔══██╗██╔════╝██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗
██║     ██║   ██║██████╔╝█████╗  █████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██║     ██████╔╝██████╔╝
██║     ██║   ██║██╔══██╗██╔══╝  ██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██║     ██╔═══╝ ██╔═══╝
╚██████╗╚██████╔╝██████╔╝███████╗███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║██╗╚██████╗██║     ██║
 ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "cubeEvent.h"

#include <GLFW/glfw3.h>

CubeKey cubeKeyFromGlfwKey(int glfwKey)
{
    switch (glfwKey) {
    case GLFW_KEY_A: return CubeKey::A;
    case GLFW_KEY_B: return CubeKey::B;
    case GLFW_KEY_C: return CubeKey::C;
    case GLFW_KEY_D: return CubeKey::D;
    case GLFW_KEY_E: return CubeKey::E;
    case GLFW_KEY_F: return CubeKey::F;
    case GLFW_KEY_G: return CubeKey::G;
    case GLFW_KEY_H: return CubeKey::H;
    case GLFW_KEY_I: return CubeKey::I;
    case GLFW_KEY_J: return CubeKey::J;
    case GLFW_KEY_K: return CubeKey::K;
    case GLFW_KEY_L: return CubeKey::L;
    case GLFW_KEY_M: return CubeKey::M;
    case GLFW_KEY_N: return CubeKey::N;
    case GLFW_KEY_O: return CubeKey::O;
    case GLFW_KEY_P: return CubeKey::P;
    case GLFW_KEY_Q: return CubeKey::Q;
    case GLFW_KEY_R: return CubeKey::R;
    case GLFW_KEY_S: return CubeKey::S;
    case GLFW_KEY_T: return CubeKey::T;
    case GLFW_KEY_U: return CubeKey::U;
    case GLFW_KEY_V: return CubeKey::V;
    case GLFW_KEY_W: return CubeKey::W;
    case GLFW_KEY_X: return CubeKey::X;
    case GLFW_KEY_Y: return CubeKey::Y;
    case GLFW_KEY_Z: return CubeKey::Z;
    case GLFW_KEY_0: return CubeKey::Num0;
    case GLFW_KEY_1: return CubeKey::Num1;
    case GLFW_KEY_2: return CubeKey::Num2;
    case GLFW_KEY_3: return CubeKey::Num3;
    case GLFW_KEY_4: return CubeKey::Num4;
    case GLFW_KEY_5: return CubeKey::Num5;
    case GLFW_KEY_6: return CubeKey::Num6;
    case GLFW_KEY_7: return CubeKey::Num7;
    case GLFW_KEY_8: return CubeKey::Num8;
    case GLFW_KEY_9: return CubeKey::Num9;
    case GLFW_KEY_ESCAPE: return CubeKey::Escape;
    case GLFW_KEY_LEFT_CONTROL: return CubeKey::LeftControl;
    case GLFW_KEY_LEFT_SHIFT: return CubeKey::LeftShift;
    case GLFW_KEY_LEFT_ALT: return CubeKey::LeftAlt;
    case GLFW_KEY_LEFT_SUPER: return CubeKey::LeftSystem;
    case GLFW_KEY_RIGHT_CONTROL: return CubeKey::RightControl;
    case GLFW_KEY_RIGHT_SHIFT: return CubeKey::RightShift;
    case GLFW_KEY_RIGHT_ALT: return CubeKey::RightAlt;
    case GLFW_KEY_RIGHT_SUPER: return CubeKey::RightSystem;
    case GLFW_KEY_MENU: return CubeKey::Menu;
    case GLFW_KEY_LEFT_BRACKET: return CubeKey::LeftBracket;
    case GLFW_KEY_RIGHT_BRACKET: return CubeKey::RightBracket;
    case GLFW_KEY_SEMICOLON: return CubeKey::Semicolon;
    case GLFW_KEY_COMMA: return CubeKey::Comma;
    case GLFW_KEY_PERIOD: return CubeKey::Period;
    case GLFW_KEY_APOSTROPHE: return CubeKey::Quote;
    case GLFW_KEY_SLASH: return CubeKey::Slash;
    case GLFW_KEY_BACKSLASH: return CubeKey::Backslash;
    case GLFW_KEY_GRAVE_ACCENT: return CubeKey::Tilde;
    case GLFW_KEY_EQUAL: return CubeKey::Equal;
    case GLFW_KEY_MINUS: return CubeKey::Dash;
    case GLFW_KEY_SPACE: return CubeKey::Space;
    case GLFW_KEY_ENTER: return CubeKey::Return;
    case GLFW_KEY_BACKSPACE: return CubeKey::Backspace;
    case GLFW_KEY_TAB: return CubeKey::Tab;
    case GLFW_KEY_PAGE_UP: return CubeKey::PageUp;
    case GLFW_KEY_PAGE_DOWN: return CubeKey::PageDown;
    case GLFW_KEY_END: return CubeKey::End;
    case GLFW_KEY_HOME: return CubeKey::Home;
    case GLFW_KEY_INSERT: return CubeKey::Insert;
    case GLFW_KEY_DELETE: return CubeKey::Delete;
    case GLFW_KEY_KP_ADD: return CubeKey::Add;
    case GLFW_KEY_KP_SUBTRACT: return CubeKey::Subtract;
    case GLFW_KEY_KP_MULTIPLY: return CubeKey::Multiply;
    case GLFW_KEY_KP_DIVIDE: return CubeKey::Divide;
    case GLFW_KEY_LEFT: return CubeKey::Left;
    case GLFW_KEY_RIGHT: return CubeKey::Right;
    case GLFW_KEY_UP: return CubeKey::Up;
    case GLFW_KEY_DOWN: return CubeKey::Down;
    case GLFW_KEY_KP_0: return CubeKey::Numpad0;
    case GLFW_KEY_KP_1: return CubeKey::Numpad1;
    case GLFW_KEY_KP_2: return CubeKey::Numpad2;
    case GLFW_KEY_KP_3: return CubeKey::Numpad3;
    case GLFW_KEY_KP_4: return CubeKey::Numpad4;
    case GLFW_KEY_KP_5: return CubeKey::Numpad5;
    case GLFW_KEY_KP_6: return CubeKey::Numpad6;
    case GLFW_KEY_KP_7: return CubeKey::Numpad7;
    case GLFW_KEY_KP_8: return CubeKey::Numpad8;
    case GLFW_KEY_KP_9: return CubeKey::Numpad9;
    case GLFW_KEY_F1: return CubeKey::F1;
    case GLFW_KEY_F2: return CubeKey::F2;
    case GLFW_KEY_F3: return CubeKey::F3;
    case GLFW_KEY_F4: return CubeKey::F4;
    case GLFW_KEY_F5: return CubeKey::F5;
    case GLFW_KEY_F6: return CubeKey::F6;
    case GLFW_KEY_F7: return CubeKey::F7;
    case GLFW_KEY_F8: return CubeKey::F8;
    case GLFW_KEY_F9: return CubeKey::F9;
    case GLFW_KEY_F10: return CubeKey::F10;
    case GLFW_KEY_F11: return CubeKey::F11;
    case GLFW_KEY_F12: return CubeKey::F12;
    case GLFW_KEY_F13: return CubeKey::F13;
    case GLFW_KEY_F14: return CubeKey::F14;
    case GLFW_KEY_F15: return CubeKey::F15;
    case GLFW_KEY_PAUSE: return CubeKey::Pause;
    default: return CubeKey::Unknown;
    }
}

CubeMouseButton cubeMouseButtonFromGlfwButton(int glfwButton)
{
    switch (glfwButton) {
    case GLFW_MOUSE_BUTTON_LEFT: return CubeMouseButton::Left;
    case GLFW_MOUSE_BUTTON_RIGHT: return CubeMouseButton::Right;
    case GLFW_MOUSE_BUTTON_MIDDLE: return CubeMouseButton::Middle;
    case GLFW_MOUSE_BUTTON_4: return CubeMouseButton::XButton1;
    case GLFW_MOUSE_BUTTON_5: return CubeMouseButton::XButton2;
    default: return CubeMouseButton::Unknown;
    }
}

SpecificEventTypes specificEventTypeFromCubeKey(CubeKey key)
{
    switch (key) {
    case CubeKey::A: return SpecificEventTypes::KEYPRESS_A;
    case CubeKey::B: return SpecificEventTypes::KEYPRESS_B;
    case CubeKey::C: return SpecificEventTypes::KEYPRESS_C;
    case CubeKey::D: return SpecificEventTypes::KEYPRESS_D;
    case CubeKey::E: return SpecificEventTypes::KEYPRESS_E;
    case CubeKey::F: return SpecificEventTypes::KEYPRESS_F;
    case CubeKey::G: return SpecificEventTypes::KEYPRESS_G;
    case CubeKey::H: return SpecificEventTypes::KEYPRESS_H;
    case CubeKey::I: return SpecificEventTypes::KEYPRESS_I;
    case CubeKey::J: return SpecificEventTypes::KEYPRESS_J;
    case CubeKey::K: return SpecificEventTypes::KEYPRESS_K;
    case CubeKey::L: return SpecificEventTypes::KEYPRESS_L;
    case CubeKey::M: return SpecificEventTypes::KEYPRESS_M;
    case CubeKey::N: return SpecificEventTypes::KEYPRESS_N;
    case CubeKey::O: return SpecificEventTypes::KEYPRESS_O;
    case CubeKey::P: return SpecificEventTypes::KEYPRESS_P;
    case CubeKey::Q: return SpecificEventTypes::KEYPRESS_Q;
    case CubeKey::R: return SpecificEventTypes::KEYPRESS_R;
    case CubeKey::S: return SpecificEventTypes::KEYPRESS_S;
    case CubeKey::T: return SpecificEventTypes::KEYPRESS_T;
    case CubeKey::U: return SpecificEventTypes::KEYPRESS_U;
    case CubeKey::V: return SpecificEventTypes::KEYPRESS_V;
    case CubeKey::W: return SpecificEventTypes::KEYPRESS_W;
    case CubeKey::X: return SpecificEventTypes::KEYPRESS_X;
    case CubeKey::Y: return SpecificEventTypes::KEYPRESS_Y;
    case CubeKey::Z: return SpecificEventTypes::KEYPRESS_Z;
    case CubeKey::Num0: return SpecificEventTypes::KEYPRESS_0;
    case CubeKey::Num1: return SpecificEventTypes::KEYPRESS_1;
    case CubeKey::Num2: return SpecificEventTypes::KEYPRESS_2;
    case CubeKey::Num3: return SpecificEventTypes::KEYPRESS_3;
    case CubeKey::Num4: return SpecificEventTypes::KEYPRESS_4;
    case CubeKey::Num5: return SpecificEventTypes::KEYPRESS_5;
    case CubeKey::Num6: return SpecificEventTypes::KEYPRESS_6;
    case CubeKey::Num7: return SpecificEventTypes::KEYPRESS_7;
    case CubeKey::Num8: return SpecificEventTypes::KEYPRESS_8;
    case CubeKey::Num9: return SpecificEventTypes::KEYPRESS_9;
    case CubeKey::Escape: return SpecificEventTypes::KEYPRESS_ESC;
    case CubeKey::LeftControl: return SpecificEventTypes::KEYPRESS_LCTRL;
    case CubeKey::LeftShift: return SpecificEventTypes::KEYPRESS_LSHIFT;
    case CubeKey::LeftAlt: return SpecificEventTypes::KEYPRESS_LALT;
    case CubeKey::LeftSystem: return SpecificEventTypes::KEYPRESS_LSYSTEM;
    case CubeKey::RightControl: return SpecificEventTypes::KEYPRESS_RCTRL;
    case CubeKey::RightShift: return SpecificEventTypes::KEYPRESS_RSHIFT;
    case CubeKey::RightAlt: return SpecificEventTypes::KEYPRESS_RALT;
    case CubeKey::RightSystem: return SpecificEventTypes::KEYPRESS_RSYSTEM;
    case CubeKey::Menu: return SpecificEventTypes::KEYPRESS_MENU;
    case CubeKey::LeftBracket: return SpecificEventTypes::KEYPRESS_LBRACKET;
    case CubeKey::RightBracket: return SpecificEventTypes::KEYPRESS_RBRACKET;
    case CubeKey::Semicolon: return SpecificEventTypes::KEYPRESS_SEMICOLON;
    case CubeKey::Comma: return SpecificEventTypes::KEYPRESS_COMMA;
    case CubeKey::Period: return SpecificEventTypes::KEYPRESS_PERIOD;
    case CubeKey::Quote: return SpecificEventTypes::KEYPRESS_QUOTE;
    case CubeKey::Slash: return SpecificEventTypes::KEYPRESS_SLASH;
    case CubeKey::Backslash: return SpecificEventTypes::KEYPRESS_BACKSLASH;
    case CubeKey::Tilde: return SpecificEventTypes::KEYPRESS_TILDE;
    case CubeKey::Equal: return SpecificEventTypes::KEYPRESS_EQUAL;
    case CubeKey::Dash: return SpecificEventTypes::KEYPRESS_DASH;
    case CubeKey::Space: return SpecificEventTypes::KEYPRESS_SPACE;
    case CubeKey::Return: return SpecificEventTypes::KEYPRESS_RETURN;
    case CubeKey::Backspace: return SpecificEventTypes::KEYPRESS_BACKSPACE;
    case CubeKey::Tab: return SpecificEventTypes::KEYPRESS_TAB;
    case CubeKey::PageUp: return SpecificEventTypes::KEYPRESS_PAGEUP;
    case CubeKey::PageDown: return SpecificEventTypes::KEYPRESS_PAGEDOWN;
    case CubeKey::End: return SpecificEventTypes::KEYPRESS_END;
    case CubeKey::Home: return SpecificEventTypes::KEYPRESS_HOME;
    case CubeKey::Insert: return SpecificEventTypes::KEYPRESS_INSERT;
    case CubeKey::Delete: return SpecificEventTypes::KEYPRESS_DELETE;
    case CubeKey::Add: return SpecificEventTypes::KEYPRESS_ADD;
    case CubeKey::Subtract: return SpecificEventTypes::KEYPRESS_SUBTRACT;
    case CubeKey::Multiply: return SpecificEventTypes::KEYPRESS_MULTIPLY;
    case CubeKey::Divide: return SpecificEventTypes::KEYPRESS_DIVIDE;
    case CubeKey::Left: return SpecificEventTypes::KEYPRESS_LEFT;
    case CubeKey::Right: return SpecificEventTypes::KEYPRESS_RIGHT;
    case CubeKey::Up: return SpecificEventTypes::KEYPRESS_UP;
    case CubeKey::Down: return SpecificEventTypes::KEYPRESS_DOWN;
    case CubeKey::Numpad0: return SpecificEventTypes::KEYPRESS_NUMPAD0;
    case CubeKey::Numpad1: return SpecificEventTypes::KEYPRESS_NUMPAD1;
    case CubeKey::Numpad2: return SpecificEventTypes::KEYPRESS_NUMPAD2;
    case CubeKey::Numpad3: return SpecificEventTypes::KEYPRESS_NUMPAD3;
    case CubeKey::Numpad4: return SpecificEventTypes::KEYPRESS_NUMPAD4;
    case CubeKey::Numpad5: return SpecificEventTypes::KEYPRESS_NUMPAD5;
    case CubeKey::Numpad6: return SpecificEventTypes::KEYPRESS_NUMPAD6;
    case CubeKey::Numpad7: return SpecificEventTypes::KEYPRESS_NUMPAD7;
    case CubeKey::Numpad8: return SpecificEventTypes::KEYPRESS_NUMPAD8;
    case CubeKey::Numpad9: return SpecificEventTypes::KEYPRESS_NUMPAD9;
    case CubeKey::F1: return SpecificEventTypes::KEYPRESS_F1;
    case CubeKey::F2: return SpecificEventTypes::KEYPRESS_F2;
    case CubeKey::F3: return SpecificEventTypes::KEYPRESS_F3;
    case CubeKey::F4: return SpecificEventTypes::KEYPRESS_F4;
    case CubeKey::F5: return SpecificEventTypes::KEYPRESS_F5;
    case CubeKey::F6: return SpecificEventTypes::KEYPRESS_F6;
    case CubeKey::F7: return SpecificEventTypes::KEYPRESS_F7;
    case CubeKey::F8: return SpecificEventTypes::KEYPRESS_F8;
    case CubeKey::F9: return SpecificEventTypes::KEYPRESS_F9;
    case CubeKey::F10: return SpecificEventTypes::KEYPRESS_F10;
    case CubeKey::F11: return SpecificEventTypes::KEYPRESS_F11;
    case CubeKey::F12: return SpecificEventTypes::KEYPRESS_F12;
    case CubeKey::F13: return SpecificEventTypes::KEYPRESS_F13;
    case CubeKey::F14: return SpecificEventTypes::KEYPRESS_F14;
    case CubeKey::F15: return SpecificEventTypes::KEYPRESS_F15;
    case CubeKey::Pause: return SpecificEventTypes::KEYPRESS_PAUSE;
    default: return SpecificEventTypes::NULL_EVENT;
    }
}

SpecificEventTypes specificEventTypeFromCubeMouseButton(CubeMouseButton button, CubeEventType eventType)
{
    if (eventType == CubeEventType::MouseButtonPressed) {
        switch (button) {
        case CubeMouseButton::Left: return SpecificEventTypes::MOUSEPRESSED_LEFT;
        case CubeMouseButton::Right: return SpecificEventTypes::MOUSEPRESSED_RIGHT;
        case CubeMouseButton::Middle: return SpecificEventTypes::MOUSEPRESSED_MIDDLE;
        case CubeMouseButton::XButton1: return SpecificEventTypes::MOUSEPRESSED_XBUTTON1;
        case CubeMouseButton::XButton2: return SpecificEventTypes::MOUSEPRESSED_XBUTTON2;
        default: return SpecificEventTypes::NULL_EVENT;
        }
    }

    if (eventType == CubeEventType::MouseButtonReleased) {
        switch (button) {
        case CubeMouseButton::Left: return SpecificEventTypes::MOUSERELEASED_LEFT;
        case CubeMouseButton::Right: return SpecificEventTypes::MOUSERELEASED_RIGHT;
        case CubeMouseButton::Middle: return SpecificEventTypes::MOUSERELEASED_MIDDLE;
        case CubeMouseButton::XButton1: return SpecificEventTypes::MOUSERELEASED_XBUTTON1;
        case CubeMouseButton::XButton2: return SpecificEventTypes::MOUSERELEASED_XBUTTON2;
        default: return SpecificEventTypes::NULL_EVENT;
        }
    }

    return SpecificEventTypes::NULL_EVENT;
}
