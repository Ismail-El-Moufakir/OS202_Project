#ifndef _DISPLAY_HPP_
#define _DISPLAY_HPP_

#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include <cstdint>

class Displayer
{
public:
    static std::shared_ptr<Displayer> createOrGetInstance(int width, int height);
    ~Displayer();
    void update(const std::vector<std::uint8_t>& vegetation_global_map, const std::vector<std::uint8_t>& fire_global_map);

private:
    Displayer(int width, int height);
    static std::shared_ptr<Displayer> unique_instance;
    
    int m_width;
    int m_height;
    SDL_Window* m_window;
    SDL_Renderer* m_pt_renderer;
};

#endif
