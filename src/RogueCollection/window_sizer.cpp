#pragma once
#include <algorithm>
#include <sstream>
#include <SDL.h>
#include <coord.h>
#include "window_sizer.h"
#include "utility.h"
#include "environment.h"

WindowSizer::WindowSizer(SDL_Window * window, SDL_Renderer * renderer, Environment * current_env) :
    m_window(window),
    m_renderer(renderer),
    m_current_env(current_env)
{
}

bool WindowSizer::ConsumeEvent(const SDL_Event & e)
{
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.mod & KMOD_ALT) {
            if (e.key.keysym.sym == SDLK_RETURN) {
                ToggleFullscreen(m_window);
                return true;
            }
            else if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9)
            {
                scale_window(e.key.keysym.sym - SDLK_0);
                SetFullscreen(m_window, false);
                return true;
            }
            else if (e.key.keysym.sym == SDLK_0)
            {
                scale_window(INT_MAX);
                SetFullscreen(m_window, false);
                return true;
            }
        }
    }
    return false;
}

void WindowSizer::SetWindowSize(int w, int h)
{
    int scale = INT_MAX;
    std::string value;
    if (m_current_env->get("window_scaling", &value))
    {
        scale = atoi(value.c_str());
    }
    set_window_size(w, h, scale);
}

void WindowSizer::set_window_size(int w, int h, int scale)
{
    Coord window_size = ::get_scaled_coord({ w, h }, scale);
    window_size.x = std::max(window_size.x, 342); //mdk: 342 empirically determined to be min window width
    SDL_SetWindowSize(m_window, window_size.x, window_size.y);
    SDL_RenderSetLogicalSize(m_renderer, w, h);
}

void WindowSizer::scale_window(int scale)
{
    std::ostringstream ss;
    ss << scale;
    m_current_env->set("window_scaling", ss.str());

    int w, h;
    SDL_RenderGetLogicalSize(m_renderer, &w, &h);
    set_window_size(w, h, scale);
}