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

#include "sdlwindow.h"
#include <framework/core/resourcemanager.h>
#include <framework/core/eventdispatcher.h>
#include <framework/graphics/image.h>
#include <framework/core/application.h>
#include <framework/core/clock.h>

#ifndef OPENGL_ES
#include <GL/glew.h>
#endif

SDLWindow::SDLWindow()
{
    m_minimumSize = Size(600, 480);
    m_size = Size(800, 600);

    // Initializing key map
    m_keyMap[SDLK_ESCAPE] = Fw::KeyEscape;
    m_keyMap[SDLK_TAB] = Fw::KeyTab;
    m_keyMap[SDLK_RETURN] = Fw::KeyEnter;
    m_keyMap[SDLK_BACKSPACE] = Fw::KeyBackspace;
    m_keyMap[SDLK_PAGEUP] = Fw::KeyPageUp;
    m_keyMap[SDLK_PAGEDOWN] = Fw::KeyPageDown;
    m_keyMap[SDLK_HOME] = Fw::KeyHome;
    m_keyMap[SDLK_END] = Fw::KeyEnd;
    m_keyMap[SDLK_INSERT] = Fw::KeyInsert;
    m_keyMap[SDLK_DELETE] = Fw::KeyDelete;
    m_keyMap[SDLK_UP] = Fw::KeyUp;
    m_keyMap[SDLK_DOWN] = Fw::KeyDown;
    m_keyMap[SDLK_LEFT] = Fw::KeyLeft;
    m_keyMap[SDLK_RIGHT] = Fw::KeyRight;
    m_keyMap[SDLK_NUMLOCKCLEAR] = Fw::KeyNumLock;
    m_keyMap[SDLK_SCROLLLOCK] = Fw::KeyScrollLock;
    m_keyMap[SDLK_CAPSLOCK] = Fw::KeyCapsLock;
    m_keyMap[SDLK_PRINTSCREEN] = Fw::KeyPrintScreen;
    m_keyMap[SDLK_PAUSE] = Fw::KeyPause;
    m_keyMap[SDLK_LCTRL] = Fw::KeyCtrl;
    m_keyMap[SDLK_RCTRL] = Fw::KeyCtrl;
    m_keyMap[SDLK_LSHIFT] = Fw::KeyShift;
    m_keyMap[SDLK_RSHIFT] = Fw::KeyShift;
    m_keyMap[SDLK_LALT] = Fw::KeyAlt;
    m_keyMap[SDLK_RALT] = Fw::KeyAlt;
    m_keyMap[SDLK_LGUI] = Fw::KeyMeta;
    m_keyMap[SDLK_RGUI] = Fw::KeyMeta;
    m_keyMap[SDLK_MENU] = Fw::KeyMenu;
    m_keyMap[SDLK_SPACE] = Fw::KeySpace;
    
    // Numbers
    for(int i = 0; i <= 9; ++i) m_keyMap[SDLK_0 + i] = (Fw::Key)(Fw::Key0 + i);
    // Letters
    for(int i = 0; i < 26; ++i) m_keyMap[SDLK_a + i] = (Fw::Key)(Fw::KeyA + i);
    // F keys
    for(int i = 0; i < 12; ++i) m_keyMap[SDLK_F1 + i] = (Fw::Key)(Fw::KeyF1 + i);
    
    m_keyMap[SDLK_KP_ENTER] = Fw::KeyEnter;
    m_keyMap[SDLK_KP_0] = Fw::KeyNumpad0;
    m_keyMap[SDLK_KP_1] = Fw::KeyNumpad1;
    m_keyMap[SDLK_KP_2] = Fw::KeyNumpad2;
    m_keyMap[SDLK_KP_3] = Fw::KeyNumpad3;
    m_keyMap[SDLK_KP_4] = Fw::KeyNumpad4;
    m_keyMap[SDLK_KP_5] = Fw::KeyNumpad5;
    m_keyMap[SDLK_KP_6] = Fw::KeyNumpad6;
    m_keyMap[SDLK_KP_7] = Fw::KeyNumpad7;
    m_keyMap[SDLK_KP_8] = Fw::KeyNumpad8;
    m_keyMap[SDLK_KP_9] = Fw::KeyNumpad9;
}

void SDLWindow::init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        g_logger.fatal(std::string("Failed to initialize SDL: ") + SDL_GetError());
    }

#ifdef OPENGL_ES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
#ifdef __APPLE__
    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    m_window = SDL_CreateWindow(
        "",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        m_size.width(), m_size.height(),
        flags
    );

    if (!m_window) {
        g_logger.fatal(std::string("Failed to create SDL window: ") + SDL_GetError());
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        g_logger.fatal(std::string("Failed to create SDL GL context: ") + SDL_GetError());
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);

#ifndef OPENGL_ES
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    m_created = true;
}

void SDLWindow::terminate()
{
    for (auto cursor : m_cursors) SDL_FreeCursor(cursor);
    m_cursors.clear();

    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

void SDLWindow::poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handleEvent(event);
    }
}

void SDLWindow::handleEvent(SDL_Event& event)
{
    switch (event.type) {
        case SDL_QUIT:
            if (m_onClose) m_onClose();
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                m_size = Size(event.window.data1, event.window.data2);
                if (m_onResize) m_onResize(m_size);
            } else if (event.window.event == SDL_WINDOWEVENT_MOVED) {
                m_position = Point(event.window.data1, event.window.data2);
            } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                m_focused = true;
            } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                m_focused = false;
                releaseAllKeys();
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            {
                auto it = m_keyMap.find(event.key.keysym.sym);
                Fw::Key keyCode = (it != m_keyMap.end()) ? it->second : Fw::KeyUnknown;
                if (event.type == SDL_KEYDOWN) processKeyDown(keyCode);
                else processKeyUp(keyCode);
            }
            break;
        case SDL_TEXTINPUT:
            if (m_onInputEvent) {
                m_inputEvent.reset(Fw::KeyTextInputEvent);
                m_inputEvent.keyText = event.text.text;
                m_onInputEvent(m_inputEvent);
            }
            break;
        case SDL_MOUSEMOTION:
            m_inputEvent.mousePos = Point(event.motion.x, event.motion.y);
            if (m_onInputEvent) {
                m_inputEvent.type = Fw::MouseMoveInputEvent;
                m_onInputEvent(m_inputEvent);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            {
                m_inputEvent.mousePos = Point(event.button.x, event.button.y);
                m_inputEvent.type = (event.type == SDL_MOUSEBUTTONDOWN) ? Fw::MousePressInputEvent : Fw::MouseReleaseInputEvent;
                if (event.button.button == SDL_BUTTON_LEFT) m_inputEvent.mouseButton = Fw::MouseLeftButton;
                else if (event.button.button == SDL_BUTTON_RIGHT) m_inputEvent.mouseButton = Fw::MouseRightButton;
                else if (event.button.button == SDL_BUTTON_MIDDLE) m_inputEvent.mouseButton = Fw::MouseMidButton;
                
                if (event.type == SDL_MOUSEBUTTONDOWN) m_mouseButtonStates |= (1 << m_inputEvent.mouseButton);
                else m_mouseButtonStates &= ~(1 << m_inputEvent.mouseButton);
                
                if (m_onInputEvent) m_onInputEvent(m_inputEvent);
            }
            break;
        case SDL_MOUSEWHEEL:
            if (m_onInputEvent) {
                m_inputEvent.type = Fw::MouseWheelInputEvent;
                m_inputEvent.wheelDirection = (event.wheel.y > 0) ? Fw::MouseWheelUp : Fw::MouseWheelDown;
                m_onInputEvent(m_inputEvent);
            }
            break;
    }
}

void SDLWindow::swapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}

void SDLWindow::show() { SDL_ShowWindow(m_window); m_visible = true; }
void SDLWindow::hide() { SDL_HideWindow(m_window); m_visible = false; }
void SDLWindow::maximize() { SDL_MaximizeWindow(m_window); }
void SDLWindow::move(const Point& pos) { SDL_SetWindowPosition(m_window, pos.x, pos.y); }
void SDLWindow::resize(const Size& size) { SDL_SetWindowSize(m_window, size.width(), size.height()); }
void SDLWindow::showMouse() { SDL_ShowCursor(SDL_ENABLE); }
void SDLWindow::hideMouse() { SDL_ShowCursor(SDL_DISABLE); }
void SDLWindow::setTitle(std::string_view title) { SDL_SetWindowTitle(m_window, title.data()); }
void SDLWindow::setMinimumSize(const Size& minimumSize) { m_minimumSize = minimumSize; SDL_SetWindowMinimumSize(m_window, minimumSize.width(), minimumSize.height()); }
void SDLWindow::setFullscreen(bool fullscreen) { SDL_SetWindowFullscreen(m_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); m_fullscreen = fullscreen; }
void SDLWindow::setVerticalSync(bool enable) { SDL_GL_SetSwapInterval(enable ? 1 : 0); m_vsync = enable; }
void SDLWindow::setClipboardText(std::string_view text) { SDL_SetClipboardText(text.data()); }
std::string SDLWindow::getClipboardText() { char* text = SDL_GetClipboardText(); std::string s = text ? text : ""; SDL_free(text); return s; }

Size SDLWindow::getDisplaySize()
{
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) return Size(0, 0);
    return Size(dm.w, dm.h);
}

int SDLWindow::internalLoadMouseCursor(const ImagePtr& image, const Point& hotSpot)
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        image->getPixelData(), image->getWidth(), image->getHeight(), 32, image->getWidth() * 4,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
    );
    if (!surface) return -1;
    SDL_Cursor* cursor = SDL_CreateColorCursor(surface, hotSpot.x, hotSpot.y);
    SDL_FreeSurface(surface);
    if (!cursor) return -1;
    m_cursors.push_back(cursor);
    return m_cursors.size() - 1;
}

void SDLWindow::setMouseCursor(int cursorId)
{
    if (cursorId >= 0 && cursorId < (int)m_cursors.size()) {
        SDL_SetCursor(m_cursors[cursorId]);
        m_currentCursor = cursorId;
    }
}

void SDLWindow::restoreMouseCursor()
{
    SDL_SetCursor(SDL_GetDefaultCursor());
    m_currentCursor = -1;
}

void SDLWindow::setIcon(const std::string& iconFile)
{
    ImagePtr image = Image::load(iconFile);
    if (!image) return;
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        image->getPixelData(), image->getWidth(), image->getHeight(), 32, image->getWidth() * 4,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
    );
    if (surface) {
        SDL_SetWindowIcon(m_window, surface);
        SDL_FreeSurface(surface);
    }
}
