#include <cassert>
#include <stdexcept>
#include <string> 
#include "display.hpp"
#include <iostream>
#include <cmath>

using namespace std::string_literals;

std::shared_ptr<Displayer> Displayer::unique_instance{nullptr};

std::shared_ptr<Displayer> Displayer::createOrGetInstance(int width, int height)
{
    if (unique_instance == nullptr)
        unique_instance = std::shared_ptr<Displayer>(new Displayer(width, height));
    return unique_instance;
}

Displayer::Displayer(int width, int height)
    : m_width(width), m_height(height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");  // Pas d'interpolation pour un rendu pixel perfect

    m_window = SDL_CreateWindow("Fire Simulation",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_SHOWN);
    if (!m_window) {
        std::cout << "Window creation failed: " << SDL_GetError() << std::endl;
        return;
    }

    m_pt_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_pt_renderer) {
        std::cout << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return;
    }
}

Displayer::~Displayer()
{
    if (m_pt_renderer) SDL_DestroyRenderer(m_pt_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Displayer::update(const std::vector<std::uint8_t>& vegetation_global_map, const std::vector<std::uint8_t>& fire_global_map)
{
    int grid_size = static_cast<int>(std::sqrt(vegetation_global_map.size()));
    double cell_w = static_cast<double>(m_width) / grid_size;
    double cell_h = static_cast<double>(m_height) / grid_size;

    SDL_SetRenderDrawColor(m_pt_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_pt_renderer);

    for (int i = 0; i < grid_size; ++i) {
        for (int j = 0; j < grid_size; ++j) {
            int index = i * grid_size + j;
            SDL_Rect rect = {
                static_cast<int>(j * cell_w),
                static_cast<int>(i * cell_h),
                static_cast<int>(std::ceil(cell_w)),  // Arrondi au pixel supérieur
                static_cast<int>(std::ceil(cell_h))   // pour éviter les trous
            };

            uint8_t fire = fire_global_map[index];
            uint8_t veg = vegetation_global_map[index];

            if (fire > 0) {
                // Gradient de couleur pour le feu : rouge -> orange -> jaune
                if (fire > 127) {
                    // Rouge vif pour le feu intense
                    SDL_SetRenderDrawColor(m_pt_renderer, 255, 0, 0, 255);
                } else {
                    // Orange/jaune pour le feu qui s'éteint
                    SDL_SetRenderDrawColor(m_pt_renderer, 255, 
                                         static_cast<uint8_t>(255 * (1 - fire/127.0)), 
                                         0, 255);
                }
            } else {
                // Végétation en vert, plus foncé quand la densité est plus élevée
                SDL_SetRenderDrawColor(m_pt_renderer, 0, 
                                     static_cast<uint8_t>(veg), 
                                     0, 255);
            }
            SDL_RenderFillRect(m_pt_renderer, &rect);
        }
    }

    SDL_RenderPresent(m_pt_renderer);
}
