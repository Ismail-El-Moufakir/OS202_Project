#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <SDL2/SDL.h>
#include "model.hpp"
#include "display.hpp"

// Structure pour les paramètres de simulation
struct ParamsType {
    double length{1.0};
    unsigned discretization{200};
    std::array<double,2> wind{1.0, 0.0};
    Model::LexicoIndices start{40, 100}; // Position initiale du feu (0.2, 0.5) * 200
};

// Analyse des arguments de la ligne de commande
void analyze_arg(int nargs, char* args[], ParamsType& params) {
    for (int i = 1; i < nargs; ++i) {
        std::string arg = args[i];
        if (arg == "-l" || arg == "--length") {
            if (i + 1 < nargs) params.length = std::stod(args[++i]);
        }
        else if (arg == "-d" || arg == "--discretization") {
            if (i + 1 < nargs) params.discretization = std::stoul(args[++i]);
        }
        else if (arg == "-w" || arg == "--wind") {
            if (i + 2 < nargs) {
                params.wind[0] = std::stod(args[++i]);
                params.wind[1] = std::stod(args[++i]);
            }
        }
        else if (arg == "-s" || arg == "--start") {
            if (i + 2 < nargs) {
                double x = std::stod(args[++i]);
                double y = std::stod(args[++i]);
                params.start = {
                    static_cast<unsigned int>(x * params.discretization),
                    static_cast<unsigned int>(y * params.discretization)
                };
            }
        }
    }
}

// Vérification des paramètres
bool check_params(ParamsType& params) {
    bool flag = true;
    if (params.length <= 0) {
        std::cerr << "[ERREUR] La longueur doit être positive." << std::endl;
        flag = false;
    }
    if (params.discretization == 0) {
        std::cerr << "[ERREUR] Le nombre de cellules doit être positif." << std::endl;
        flag = false;
    }
    if (params.start.row >= params.discretization || params.start.column >= params.discretization) {
        std::cerr << "[ERREUR] Indices de départ incorrects." << std::endl;
        flag = false;
    }
    return flag;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Vérifier qu'il y a au moins 2 processus
    if (size < 2) {
        if (rank == 0)
            std::cerr << "Ce programme nécessite au moins 2 processus MPI." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return EXIT_FAILURE;
    }
    
    // Paramètres de la simulation
    ParamsType params;
    analyze_arg(argc, argv, params);
    if (!check_params(params)) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    const int MAX_ITERATIONS = 500;  // Réduit le nombre maximum d'itérations
    auto start_time = std::chrono::high_resolution_clock::now();

    // Calculer les dimensions des tranches
    int slice_height = params.discretization / (size - 1);
    int slice_width = params.discretization;

    if (rank == 0) {
        // Processus d'affichage
        std::cout << "Paramètres de la simulation :" << std::endl;
        std::cout << "  Longueur du terrain : " << params.length << std::endl;
        std::cout << "  Discrétisation : " << params.discretization << std::endl;
        std::cout << "  Vent : [" << params.wind[0] << ", " << params.wind[1] << "]" << std::endl;
        std::cout << "  Position initiale : (" << params.start.column << ", " << params.start.row << ")" << std::endl;
        std::cout << "  Nombre de processus : " << size << std::endl;
        std::cout << "  Hauteur des tranches : " << slice_height << std::endl;

        // Initialisation de l'affichage
        const int SCALE = 5;
        auto displayer = Displayer::createOrGetInstance(params.discretization * SCALE, params.discretization * SCALE);
        std::vector<std::uint8_t> global_vegetal(params.discretization * params.discretization);
        std::vector<std::uint8_t> global_fire(params.discretization * params.discretization);
        bool running = true;
        int iteration = 0;

        // Boucle principale d'affichage
        while (running && iteration < MAX_ITERATIONS) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                    // Informer tous les processus de calcul de l'arrêt
                    for (int i = 1; i < size; ++i) {
                        MPI_Send(&running, 1, MPI_CXX_BOOL, i, 0, MPI_COMM_WORLD);
                    }
                    break;
                }
            }
            if (!running) break;

            // Collecter les données de tous les processus de calcul
            bool all_finished = true;
            for (int source = 1; source < size; ++source) {
                MPI_Status status;
                bool proc_running;
                MPI_Recv(&proc_running, 1, MPI_CXX_BOOL, source, 1, MPI_COMM_WORLD, &status);
                all_finished = all_finished && !proc_running;

                int start_row = (source - 1) * slice_height;
                int real_slice_size = slice_height * slice_width;

                // Recevoir les données de la tranche
                std::vector<std::uint8_t> slice_vegetal(real_slice_size);
                std::vector<std::uint8_t> slice_fire(real_slice_size);
                MPI_Recv(slice_vegetal.data(), real_slice_size, MPI_UINT8_T, source, 2, MPI_COMM_WORLD, &status);
                MPI_Recv(slice_fire.data(), real_slice_size, MPI_UINT8_T, source, 3, MPI_COMM_WORLD, &status);

                // Copier les données dans les tableaux globaux
                for (int i = 0; i < slice_height; ++i) {
                    std::copy(slice_vegetal.begin() + i * slice_width,
                            slice_vegetal.begin() + (i + 1) * slice_width,
                            global_vegetal.begin() + (start_row + i) * slice_width);
                    std::copy(slice_fire.begin() + i * slice_width,
                            slice_fire.begin() + (i + 1) * slice_width,
                            global_fire.begin() + (start_row + i) * slice_width);
                }
            }

            displayer->update(global_vegetal, global_fire);
            iteration++;

            if (all_finished) {
                running = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        std::cout << "\nRésultats de la simulation :" << std::endl;
        std::cout << "  Nombre d'itérations : " << iteration << std::endl;
        std::cout << "  Temps total : " << elapsed_seconds.count() << " secondes" << std::endl;
        std::cout << "  Temps moyen par itération : " << elapsed_seconds.count() / iteration * 1000 << " ms" << std::endl;
    }
    else {
        // Processus de calcul
        int start_row = (rank - 1) * slice_height;
        bool running = true;
        int iteration = 0;

        // Créer le modèle avec la taille totale
        Model simu(params.length, params.discretization, params.wind, params.start);
        
        // Boucle principale de calcul
        while (running && iteration < MAX_ITERATIONS) {
            // Vérifier les messages d'arrêt
            MPI_Status status;
            int flag;
            MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                MPI_Recv(&running, 1, MPI_CXX_BOOL, 0, 0, MPI_COMM_WORLD, &status);
                break;
            }

            // Échanger les cellules fantômes avec les voisins
            if (rank > 1) {
                MPI_Send(simu.fire_map().data() + start_row * slice_width, 
                        slice_width, MPI_UINT8_T, rank - 1, 3, MPI_COMM_WORLD);
                std::vector<std::uint8_t> ghost_line(slice_width);
                MPI_Recv(ghost_line.data(), slice_width, MPI_UINT8_T, 
                        rank - 1, 4, MPI_COMM_WORLD, &status);
                std::copy(ghost_line.begin(), ghost_line.end(), 
                        simu.fire_map().data() + (start_row - 1) * slice_width);
            }

            if (rank < size - 1) {
                MPI_Send(simu.fire_map().data() + (start_row + slice_height - 1) * slice_width,
                        slice_width, MPI_UINT8_T, rank + 1, 4, MPI_COMM_WORLD);
                std::vector<std::uint8_t> ghost_line(slice_width);
                MPI_Recv(ghost_line.data(), slice_width, MPI_UINT8_T,
                        rank + 1, 3, MPI_COMM_WORLD, &status);
                std::copy(ghost_line.begin(), ghost_line.end(),
                        simu.fire_map().data() + (start_row + slice_height) * slice_width);
            }

            // Mettre à jour la simulation
            running = simu.update();

            // Envoyer les résultats au processus d'affichage
            auto vegetal_map = simu.vegetal_map();
            auto fire_map = simu.fire_map();
            
            // Envoyer uniquement la tranche locale
            int real_slice_size = slice_height * slice_width;
            std::vector<std::uint8_t> slice_vegetal(real_slice_size);
            std::vector<std::uint8_t> slice_fire(real_slice_size);
            
            for (int i = 0; i < slice_height; ++i) {
                std::copy(vegetal_map.begin() + (start_row + i) * slice_width,
                        vegetal_map.begin() + (start_row + i + 1) * slice_width,
                        slice_vegetal.begin() + i * slice_width);
                std::copy(fire_map.begin() + (start_row + i) * slice_width,
                        fire_map.begin() + (start_row + i + 1) * slice_width,
                        slice_fire.begin() + i * slice_width);
            }
            
            MPI_Send(&running, 1, MPI_CXX_BOOL, 0, 1, MPI_COMM_WORLD);
            MPI_Send(slice_vegetal.data(), real_slice_size, MPI_UINT8_T, 0, 2, MPI_COMM_WORLD);
            MPI_Send(slice_fire.data(), real_slice_size, MPI_UINT8_T, 0, 3, MPI_COMM_WORLD);

            iteration++;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
} 