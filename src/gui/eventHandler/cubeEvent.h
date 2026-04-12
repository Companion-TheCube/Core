/*
 ██████╗██╗   ██╗██████╗ ███████╗███████╗██╗   ██╗███████╗███╗   ██╗████████╗██╗  ██╗
██╔════╝██║   ██║██╔══██╗██╔════╝██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██║  ██║
██║     ██║   ██║██████╔╝█████╗  █████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ███████║
██║     ██║   ██║██╔══██╗██╔══╝  ██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██╔══██║
╚██████╗╚██████╔╝██████╔╝███████╗███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║██╗██║  ██║
 ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝╚═╝╚═╝  ╚═╝
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

#pragma once

#include <cstdint>

enum class SpecificEventTypes : unsigned int {
    KEYPRESS_A,
    KEYPRESS_B,
    KEYPRESS_C,
    KEYPRESS_D,
    KEYPRESS_E,
    KEYPRESS_F,
    KEYPRESS_G,
    KEYPRESS_H,
    KEYPRESS_I,
    KEYPRESS_J,
    KEYPRESS_K,
    KEYPRESS_L,
    KEYPRESS_M,
    KEYPRESS_N,
    KEYPRESS_O,
    KEYPRESS_P,
    KEYPRESS_Q,
    KEYPRESS_R,
    KEYPRESS_S,
    KEYPRESS_T,
    KEYPRESS_U,
    KEYPRESS_V,
    KEYPRESS_W,
    KEYPRESS_X,
    KEYPRESS_Y,
    KEYPRESS_Z,
    KEYPRESS_0,
    KEYPRESS_1,
    KEYPRESS_2,
    KEYPRESS_3,
    KEYPRESS_4,
    KEYPRESS_5,
    KEYPRESS_6,
    KEYPRESS_7,
    KEYPRESS_8,
    KEYPRESS_9,
    KEYPRESS_ESC,
    KEYPRESS_LCTRL,
    KEYPRESS_LSHIFT,
    KEYPRESS_LALT,
    KEYPRESS_LSYSTEM,
    KEYPRESS_RCTRL,
    KEYPRESS_RSHIFT,
    KEYPRESS_RALT,
    KEYPRESS_RSYSTEM,
    KEYPRESS_MENU,
    KEYPRESS_LBRACKET,
    KEYPRESS_RBRACKET,
    KEYPRESS_SEMICOLON,
    KEYPRESS_COMMA,
    KEYPRESS_PERIOD,
    KEYPRESS_QUOTE,
    KEYPRESS_SLASH,
    KEYPRESS_BACKSLASH,
    KEYPRESS_TILDE,
    KEYPRESS_EQUAL,
    KEYPRESS_DASH,
    KEYPRESS_SPACE,
    KEYPRESS_RETURN,
    KEYPRESS_BACKSPACE,
    KEYPRESS_TAB,
    KEYPRESS_PAGEUP,
    KEYPRESS_PAGEDOWN,
    KEYPRESS_END,
    KEYPRESS_HOME,
    KEYPRESS_INSERT,
    KEYPRESS_DELETE,
    KEYPRESS_ADD,
    KEYPRESS_SUBTRACT,
    KEYPRESS_MULTIPLY,
    KEYPRESS_DIVIDE,
    KEYPRESS_LEFT,
    KEYPRESS_RIGHT,
    KEYPRESS_UP,
    KEYPRESS_DOWN,
    KEYPRESS_NUMPAD0,
    KEYPRESS_NUMPAD1,
    KEYPRESS_NUMPAD2,
    KEYPRESS_NUMPAD3,
    KEYPRESS_NUMPAD4,
    KEYPRESS_NUMPAD5,
    KEYPRESS_NUMPAD6,
    KEYPRESS_NUMPAD7,
    KEYPRESS_NUMPAD8,
    KEYPRESS_NUMPAD9,
    KEYPRESS_F1,
    KEYPRESS_F2,
    KEYPRESS_F3,
    KEYPRESS_F4,
    KEYPRESS_F5,
    KEYPRESS_F6,
    KEYPRESS_F7,
    KEYPRESS_F8,
    KEYPRESS_F9,
    KEYPRESS_F10,
    KEYPRESS_F11,
    KEYPRESS_F12,
    KEYPRESS_F13,
    KEYPRESS_F14,
    KEYPRESS_F15,
    KEYPRESS_PAUSE,
    MOUSEPRESSED_LEFT,
    MOUSEPRESSED_RIGHT,
    MOUSEPRESSED_MIDDLE,
    MOUSEPRESSED_XBUTTON1,
    MOUSEPRESSED_XBUTTON2,
    MOUSERELEASED_LEFT,
    MOUSERELEASED_RIGHT,
    MOUSERELEASED_MIDDLE,
    MOUSERELEASED_XBUTTON1,
    MOUSERELEASED_XBUTTON2,
    MOUSEMOVED,
    MOUSEWHEEL,
    MOUSEENTERED,
    MOUSELEFT,
    JOYSTICKCONNECTED,
    JOYSTICKDISCONNECTED,
    JOYSTICKBUTTONPRESSED,
    JOYSTICKBUTTONRELEASED,
    JOYSTICKMOVED,
    DRAG_Y,
    DRAG_X,
    NULL_EVENT
};

enum class CubeEventType : uint8_t {
    None,
    WindowClosed,
    KeyPressed,
    KeyReleased,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseWheelScrolled
};

enum class CubeKey : uint16_t {
    Unknown,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Escape,
    LeftControl,
    LeftShift,
    LeftAlt,
    LeftSystem,
    RightControl,
    RightShift,
    RightAlt,
    RightSystem,
    Menu,
    LeftBracket,
    RightBracket,
    Semicolon,
    Comma,
    Period,
    Quote,
    Slash,
    Backslash,
    Tilde,
    Equal,
    Dash,
    Space,
    Return,
    Backspace,
    Tab,
    PageUp,
    PageDown,
    End,
    Home,
    Insert,
    Delete,
    Add,
    Subtract,
    Multiply,
    Divide,
    Left,
    Right,
    Up,
    Down,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    Pause
};

enum class CubeMouseButton : uint8_t {
    Unknown,
    Left,
    Right,
    Middle,
    XButton1,
    XButton2
};

struct CubeEvent {
    CubeEventType type = CubeEventType::None;
    CubeKey key = CubeKey::Unknown;
    CubeMouseButton mouseButton = CubeMouseButton::Unknown;
    int x = 0;
    int y = 0;
    int deltaX = 0;
    int deltaY = 0;
    double scrollX = 0.0;
    double scrollY = 0.0;
    bool isRepeat = false;
};

CubeKey cubeKeyFromGlfwKey(int glfwKey);
CubeMouseButton cubeMouseButtonFromGlfwButton(int glfwButton);
SpecificEventTypes specificEventTypeFromCubeKey(CubeKey key);
SpecificEventTypes specificEventTypeFromCubeMouseButton(CubeMouseButton button, CubeEventType eventType);
