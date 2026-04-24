/*
 * Copyright (c) 2010-2026 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "sdl2window.h"
#include <framework/core/clock.h>
#include <framework/graphics/image.h>
#include <framework/core/application.h>

SDL2Window::SDL2Window()
{
    m_window = nullptr;
    m_context = nullptr;
    m_cursor = nullptr;
}

void SDL2Window::init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        g_logger.fatal(stdext::format("SDL could not initialize! SDL_Error: %s", SDL_GetError()));
        return;
    }

    SDL_GL_set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_set_attribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_set_attribute(SDL_GL_DEPTH_SIZE, 24);

    uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    
    m_window = SDL_CreateWindow("OTClient", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, windowFlags);
    if (!m_window) {
        g_logger.fatal(stdext::format("Window could not be created! SDL_Error: %s", SDL_GetError()));
        return;
    }

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context) {
        g_logger.fatal(stdext::format("OpenGL context could not be created! SDL_Error: %s", SDL_GetError()));
        return;
    }

    SDL_GL_MakeCurrent(m_window, m_context);
    
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    m_size = Size(w, h);
    
    int pxW, pxH;
    SDL_GL_GetDrawableSize(m_window, &pxW, &pxH);
    if (w > 0) m_displayDensity = (float)pxW / w;

    m_visible = true;
    m_focused = true;
}

void SDL2Window::terminate()
{
    if (m_cursor) {
        SDL_FreeCursor(m_cursor);
        m_cursor = nullptr;
    }
    if (m_context) {
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

void SDL2Window::move(const Point& pos)
{
    SDL_SetWindowPosition(m_window, pos.x, pos.y);
    m_position = pos;
}

void SDL2Window::resize(const Size& size)
{
    SDL_SetWindowSize(m_window, size.width(), size.height());
    m_size = size;
}

void SDL2Window::show()
{
    SDL_ShowWindow(m_window);
    m_visible = true;
}

void SDL2Window::hide()
{
    SDL_HideWindow(m_window);
    m_visible = false;
}

void SDL2Window::maximize()
{
    SDL_MaximizeWindow(m_window);
    m_maximized = true;
}

void SDL2Window::poll()
{
    fireKeysPress();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_WINDOWEVENT:
                handleWindowEvent(event.window);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                handleKeyEvent(event.key);
                break;
            case SDL_TEXTINPUT:
                handleTextInputEvent(event.text);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                handleMouseButtonEvent(event.button);
                break;
            case SDL_MOUSEMOTION:
                handleMouseMotionEvent(event.motion);
                break;
            case SDL_MOUSEWHEEL:
                handleMouseWheelEvent(event.wheel);
                break;
            case SDL_QUIT:
                if (m_onClose) m_onClose();
                break;
        }
    }
}

void SDL2Window::swapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}

void SDL2Window::showMouse()
{
    SDL_ShowCursor(SDL_ENABLE);
}

void SDL2Window::hideMouse()
{
    SDL_ShowCursor(SDL_DISABLE);
}

void SDL2Window::setMouseCursor(int cursorId)
{
    // Simplified: mapping to system cursors
    SDL_SystemCursor sdlCursorId = SDL_SYSTEM_CURSOR_ARROW;
    switch(cursorId) {
        case 0: sdlCursorId = SDL_SYSTEM_CURSOR_ARROW; break; // Default
        case 1: sdlCursorId = SDL_SYSTEM_CURSOR_IBEAM; break; // Text
        case 2: sdlCursorId = SDL_SYSTEM_CURSOR_HAND; break;  // Hand
    }
    
    if (m_cursor) SDL_FreeCursor(m_cursor);
    m_cursor = SDL_CreateSystemCursor(sdlCursorId);
    SDL_SetCursor(m_cursor);
}

void SDL2Window::restoreMouseCursor()
{
    setMouseCursor(0);
}

void SDL2Window::setTitle(std::string_view title)
{
    SDL_SetWindowTitle(m_window, std::string(title).c_str());
}

void SDL2Window::setMinimumSize(const Size& minimumSize)
{
    SDL_SetWindowMinimumSize(m_window, minimumSize.width(), minimumSize.height());
    m_minimumSize = minimumSize;
}

void SDL2Window::setFullscreen(bool fullscreen)
{
    SDL_SetWindowFullscreen(m_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    m_fullscreen = fullscreen;
}

void SDL2Window::setVerticalSync(bool enable)
{
    SDL_GL_SetSwapInterval(enable ? 1 : 0);
    m_vsync = enable;
}

void SDL2Window::setIcon(const std::string& iconFile)
{
    const auto& image = Image::load(iconFile);
    if (!image) return;

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        image->getPixelData(), 
        image->getWidth(), image->getHeight(), 
        32, image->getWidth() * 4,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
    );

    if (surface) {
        SDL_SetWindowIcon(m_window, surface);
        SDL_FreeSurface(surface);
    }
}

void SDL2Window::setClipboardText(std::string_view text)
{
    SDL_SetClipboardText(std::string(text).c_str());
}

Size SDL2Window::getDisplaySize()
{
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);
    return Size(mode.w, mode.h);
}

std::string SDL2Window::getClipboardText()
{
    char* text = SDL_GetClipboardText();
    std::string res = text ? text : "";
    SDL_free(text);
    return res;
}

std::string SDL2Window::getPlatformType()
{
    return "SDL2";
}

int SDL2Window::internalLoadMouseCursor(const ImagePtr& image, const Point& hotSpot)
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        image->getPixelData(), 
        image->getWidth(), image->getHeight(), 
        32, image->getWidth() * 4,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
    );

    if (surface) {
        if (m_cursor) SDL_FreeCursor(m_cursor);
        m_cursor = SDL_CreateColorCursor(surface, hotSpot.x, hotSpot.y);
        SDL_SetCursor(m_cursor);
        SDL_FreeSurface(surface);
        return 1;
    }
    return -1;
}

void SDL2Window::handleWindowEvent(SDL_WindowEvent& event)
{
    switch (event.event) {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            m_size = Size(event.data1, event.data2);
            if (m_onResize) m_onResize(m_size);
            break;
        case SDL_WINDOWEVENT_MOVED:
            m_position = Point(event.data1, event.data2);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            m_focused = true;
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            m_focused = false;
            releaseAllKeys();
            break;
        case SDL_WINDOWEVENT_CLOSE:
            if (m_onClose) m_onClose();
            break;
    }
}

void SDL2Window::handleKeyEvent(SDL_KeyboardEvent& event)
{
    Fw::Key keyCode = rewrapKey(event.keysym.scancode, event.keysym.sym);
    if (keyCode == Fw::KeyUnknown) return;

    if (event.type == SDL_KEYDOWN) {
        processKeyDown(keyCode);
    } else {
        processKeyUp(keyCode);
    }
}

void SDL2Window::handleTextInputEvent(SDL_TextInputEvent& event)
{
    m_inputEvent.reset(Fw::KeyTextInputEvent);
    m_inputEvent.keyText = event.text;
    if (m_onInputEvent) m_onInputEvent(m_inputEvent);
}

void SDL2Window::handleMouseButtonEvent(SDL_MouseButtonEvent& event)
{
    m_inputEvent.reset(event.type == SDL_MOUSEBUTTONDOWN ? Fw::MousePressInputEvent : Fw::MouseReleaseInputEvent);
    m_inputEvent.mousePos = Point(event.x, event.y);
    m_inputEvent.mouseButton = rewrapMouseButton(event.button);

    if (event.type == SDL_MOUSEBUTTONDOWN)
        m_mouseButtonStates |= (1 << m_inputEvent.mouseButton);
    else
        m_mouseButtonStates &= ~(1 << m_inputEvent.mouseButton);

    if (m_onInputEvent) m_onInputEvent(m_inputEvent);
}

void SDL2Window::handleMouseMotionEvent(SDL_MouseMotionEvent& event)
{
    m_inputEvent.reset(Fw::MouseMoveInputEvent);
    m_inputEvent.mousePos = Point(event.x, event.y);
    m_inputEvent.mouseMoved = Point(event.xrel, event.yrel);
    if (m_onInputEvent) m_onInputEvent(m_inputEvent);
}

void SDL2Window::handleMouseWheelEvent(SDL_MouseWheelEvent& event)
{
    m_inputEvent.reset(Fw::MouseWheelInputEvent);
    m_inputEvent.mousePos = Point(event.x, event.y); // Note: might need current mouse pos
    m_inputEvent.wheelDirection = (event.y > 0) ? Fw::MouseWheelUp : Fw::MouseWheelDown;
    if (m_onInputEvent) m_onInputEvent(m_inputEvent);
}

Fw::Key SDL2Window::rewrapKey(SDL_Scancode scanCode, SDL_Keycode keyCode)
{
    // Mapping SDL2 keys to OTClient keys
    if (keyCode >= 'a' && keyCode <= 'z') return (Fw::Key)(Fw::KeyA + (keyCode - 'a'));
    if (keyCode >= '0' && keyCode <= '9') return (Fw::Key)(Fw::Key0 + (keyCode - '0'));
    
    switch(keyCode) {
        case SDLK_ESCAPE: return Fw::KeyEscape;
        case SDLK_TAB: return Fw::KeyTab;
        case SDLK_BACKSPACE: return Fw::KeyBackspace;
        case SDLK_RETURN: return Fw::KeyEnter;
        case SDLK_INSERT: return Fw::KeyInsert;
        case SDLK_DELETE: return Fw::KeyDelete;
        case SDLK_PAUSE: return Fw::KeyPause;
        case SDLK_PRINTSCREEN: return Fw::KeyPrintScreen;
        case SDLK_HOME: return Fw::KeyHome;
        case SDLK_END: return Fw::KeyEnd;
        case SDLK_PAGEUP: return Fw::KeyPageUp;
        case SDLK_PAGEDOWN: return Fw::KeyPageDown;
        case SDLK_UP: return Fw::KeyUp;
        case SDLK_DOWN: return Fw::KeyDown;
        case SDLK_LEFT: return Fw::KeyLeft;
        case SDLK_RIGHT: return Fw::KeyRight;
        case SDLK_NUMLOCKCLEAR: return Fw::KeyNumLock;
        case SDLK_SCROLLLOCK: return Fw::KeyScrollLock;
        case SDLK_CAPSLOCK: return Fw::KeyCapsLock;
        case SDLK_LCTRL: case SDLK_RCTRL: return Fw::KeyCtrl;
        case SDLK_LSHIFT: case SDLK_RSHIFT: return Fw::KeyShift;
        case SDLK_LALT: case SDLK_RALT: return Fw::KeyAlt;
        case SDLK_LGUI: case SDLK_RGUI: return Fw::KeyMeta;
        case SDLK_SPACE: return Fw::KeySpace;
    }
    
    if (keyCode >= SDLK_F1 && keyCode <= SDLK_F12) return (Fw::Key)(Fw::KeyF1 + (keyCode - SDLK_F1));
    if (keyCode >= SDLK_KP_1 && keyCode <= SDLK_KP_9) return (Fw::Key)(Fw::KeyNumpad1 + (keyCode - SDLK_KP_1));
    if (keyCode == SDLK_KP_0) return Fw::KeyNumpad0;

    return Fw::KeyUnknown;
}

Fw::MouseButton SDL2Window::rewrapMouseButton(uint8_t button)
{
    switch(button) {
        case SDL_BUTTON_LEFT: return Fw::MouseLeftButton;
        case SDL_BUTTON_RIGHT: return Fw::MouseRightButton;
        case SDL_BUTTON_MIDDLE: return Fw::MouseMidButton;
    }
    return Fw::MouseNoButton;
}
