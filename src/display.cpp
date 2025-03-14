#include <cassert>
#include <stdexcept>
#include <string> 
#include "display.hpp"

using namespace std::string_literals;

std::shared_ptr<Displayer> Displayer::unique_instance{nullptr};

Displayer::Displayer( std::uint32_t t_width, std::uint32_t t_height )
{
    // Initialisation du contexte pour SDL :
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        std::string err_msg = "Erreur lors de l'initialisation de SDL : "s + std::string(SDL_GetError());
        throw std::runtime_error(err_msg);
    }

    // On multiplie la taille par un facteur pour avoir une fenêtre plus grande
    const int scale = 20;
    const int window_width = t_width * scale;
    const int window_height = t_height * scale;

    // Création de la fenêtre avec support Metal sur macOS
    m_pt_window = SDL_CreateWindow("Simulation Feu de Forêt",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                window_width, window_height,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    
    if (m_pt_window == nullptr)
    {
        std::string err_msg = "Erreur lors de la création de la fenêtre : "s + std::string(SDL_GetError());
        throw std::runtime_error(err_msg);
    }

    // Création du renderer avec accélération matérielle
    m_pt_renderer = SDL_CreateRenderer(m_pt_window, -1, 
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (m_pt_renderer == nullptr)
    {
        std::string err_msg = "Erreur lors de la création du moteur de rendu : "s + std::string(SDL_GetError());
        throw std::runtime_error(err_msg);
    }

    // Configure le renderer pour utiliser la taille logique de la fenêtre
    SDL_RenderSetLogicalSize(m_pt_renderer, window_width, window_height);
}

Displayer::~Displayer()
{
    if (m_pt_renderer) SDL_DestroyRenderer(m_pt_renderer);
    if (m_pt_window) SDL_DestroyWindow(m_pt_window);
    SDL_Quit();
}

void
Displayer::update( std::vector<std::uint8_t> const & vegetation_global_map,
                   std::vector<std::uint8_t> const & fire_global_map )
{
    const int scale = 20;
    const int grid_w = vegetation_global_map.size() / scale;
    const int grid_h = grid_w;  // Puisque c'est un carré
    
    // Efface l'écran avec du noir
    SDL_SetRenderDrawColor(m_pt_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_pt_renderer);

    // Dessine chaque cellule comme un rectangle
    SDL_Rect rect;
    rect.w = scale;
    rect.h = scale;

    for (int i = 0; i < grid_h; ++i)
    {
        for (int j = 0; j < grid_w; ++j)
        {
            SDL_SetRenderDrawColor(m_pt_renderer, 
                                fire_global_map[j + grid_w*i],
                                vegetation_global_map[j + grid_w*i], 
                                0, 255);
            rect.x = j * scale;
            rect.y = (grid_h-i-1) * scale;
            SDL_RenderFillRect(m_pt_renderer, &rect);
        }
    }

    // Affiche le résultat
    SDL_RenderPresent(m_pt_renderer);
}

std::shared_ptr<Displayer> 
Displayer::init_instance( std::uint32_t t_width, std::uint32_t t_height )
{
    assert( ( "L'initialisation de l'instance ne doit etre appele qu'une seule fois !" && (unique_instance == nullptr) ) );
    unique_instance = std::make_shared<Displayer>(t_width, t_height);
    return unique_instance;
}

std::shared_ptr<Displayer> 
Displayer::instance()
{
    assert( ( "Il faut initialiser l'instance avant tout !" && (unique_instance != nullptr) ) );
    return unique_instance;
}
