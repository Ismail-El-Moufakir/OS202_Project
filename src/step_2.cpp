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

// --- Fonctions de parsing d'arguments (exemple minimal) ---
struct ParamsType {
    double length{10.};
    unsigned discretization{300u};
    std::array<double,2> wind{0.,0.};
    Model::LexicoIndices start{10u,10u};
};

void analyze_arg(int nargs, char* args[], ParamsType& params) {
    // Implémentez ici votre parsing si besoin.
    // Pour cet exemple, on garde les valeurs par défaut.
}

ParamsType parse_arguments(int nargs, char* args[]) {
    ParamsType params;
    analyze_arg(nargs, args, params);
    return params;
}

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

    if (size != 2) {
        if (rank == 0)
            std::cerr << "Ce programme doit être lancé avec exactement 2 processus MPI." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    auto params = parse_arguments(argc - 1, &argv[1]);
    if (!check_params(params)) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    int grid_size = params.discretization * params.discretization;
    double total_start_time = MPI_Wtime(); // Chrono global depuis le début

    if (rank == 0) {
        // --- Processus 0 : Affichage ---
        std::shared_ptr<Displayer> displayer = Displayer::init_instance(params.discretization, params.discretization);
        std::vector<std::uint8_t> global_vegetal(grid_size);
        std::vector<std::uint8_t> global_fire(grid_size);
        bool running = true;
        unsigned display_count = 0;
        auto total_display_time = std::chrono::high_resolution_clock::duration::zero();

        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }
            if (!running) break;

            int flag = 0;
            MPI_Iprobe(1, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            if (flag) {
                MPI_Recv(global_vegetal.data(), grid_size, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(global_fire.data(), grid_size, MPI_UNSIGNED_CHAR, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                auto display_start = std::chrono::high_resolution_clock::now();
                displayer->update(global_vegetal, global_fire);
                auto display_end = std::chrono::high_resolution_clock::now();
                total_display_time += (display_end - display_start);
                display_count++;

                // Tous les 32 affichages, on affiche les temps
                if (display_count % 32 == 0) {
                    double total_sim_ms = 0.0;
                    MPI_Recv(&total_sim_ms, 1, MPI_DOUBLE, 1, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    double total_display_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_display_time).count();
                    double total_time_s = MPI_Wtime() - total_start_time;  // Temps global depuis le début

                    std::cout << "\n=== Iteration " << display_count << " ===" << std::endl;
                    std::cout << "[Affichage] Temps total partie affichage : " << total_display_ms / 1000.0 << " secondes" << std::endl;
                    std::cout << "[Simulation] Temps total partie calcul : " << total_sim_ms / 1000.0 << " secondes" << std::endl;
                    std::cout << "[Simulation] Temps total simulation : " << total_time_s << " secondes" << std::endl;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Envoi du signal de terminaison
        int termination_signal = 0;
        MPI_Send(&termination_signal, 1, MPI_INT, 1, 2, MPI_COMM_WORLD);

        // Affichage final
        double total_time_s = MPI_Wtime() - total_start_time;
        std::cout << "\n=== Résultats globaux ===" << std::endl;
        std::cout << "[Simulation Globale] Temps total : " << total_time_s << " secondes" << std::endl;
    }
    else if (rank == 1) {
        // --- Processus 1 : Simulation ---
        Model simu(params.length, params.discretization, params.wind, params.start);
        bool simulation_continue = true;
        unsigned step_count = 0;
        auto total_sim_time = std::chrono::high_resolution_clock::duration::zero();

        while (simulation_continue) {
            auto step_start = std::chrono::high_resolution_clock::now();
            simulation_continue = simu.update();
            auto step_end = std::chrono::high_resolution_clock::now();
            total_sim_time += (step_end - step_start);
            step_count++;

            std::vector<std::uint8_t> local_veg = simu.vegetal_map();
            std::vector<std::uint8_t> local_fire = simu.fire_map();
            MPI_Send(local_veg.data(), grid_size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
            MPI_Send(local_fire.data(), grid_size, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);

            // ENVOYER le temps total de simulation toutes les 32 itérations
            if (step_count % 32 == 0) {
                double total_sim_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_sim_time).count();
                MPI_Send(&total_sim_ms, 1, MPI_DOUBLE, 0, 4, MPI_COMM_WORLD);
            }

            int flag = 0;
            MPI_Iprobe(0, 2, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            if (flag) {
                int term;
                MPI_Recv(&term, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                simulation_continue = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
