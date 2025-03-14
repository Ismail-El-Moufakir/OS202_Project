#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <cassert>
#include <mpi.h>
#include "simulation.hpp"
#include "display.hpp"
#include "model.hpp"

bool analyze_args(int nargs, char* argv[], ParamsType& params)
{
    for (int i = 1; i < nargs; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-l" || arg == "--length")
        {
            if (i + 1 < nargs)
            {
                params.length = std::stod(argv[++i]);
            }
            else
            {
                std::cerr << "Missing value for " << arg << std::endl;
                return false;
            }
        }
        else if (arg == "-d" || arg == "--discretization")
        {
            if (i + 1 < nargs)
            {
                params.discretization = std::stoi(argv[++i]);
            }
            else
            {
                std::cerr << "Missing value for " << arg << std::endl;
                return false;
            }
        }
        else if (arg == "-w" || arg == "--wind")
        {
            if (i + 2 < nargs)
            {
                params.wind[0] = std::stod(argv[++i]);
                params.wind[1] = std::stod(argv[++i]);
            }
            else
            {
                std::cerr << "Missing values for " << arg << std::endl;
                return false;
            }
        }
        else if (arg == "-s" || arg == "--start")
        {
            if (i + 2 < nargs)
            {
                params.start[0] = std::stod(argv[++i]);
                params.start[1] = std::stod(argv[++i]);
            }
            else
            {
                std::cerr << "Missing values for " << arg << std::endl;
                return false;
            }
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return false;
        }
    }
    return true;
}

bool check_params(const ParamsType& params)
{
    if (params.length <= 0)
    {
        std::cerr << "Length must be positive" << std::endl;
        return false;
    }
    if (params.discretization <= 0)
    {
        std::cerr << "Discretization must be positive" << std::endl;
        return false;
    }
    if (params.start[0] < 0 || params.start[0] > 1 || params.start[1] < 0 || params.start[1] > 1)
    {
        std::cerr << "Start position must be between 0 and 1" << std::endl;
        return false;
    }
    return true;
}

int main(int nargs, char* argv[])
{
    MPI_Init(&nargs, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0)
            std::cerr << "Ce programme doit être exécuté avec exactement 2 processus MPI" << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    ParamsType params;
    if (!analyze_args(nargs, argv, params)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    if (!check_params(params)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        // Processus maître : affichage
        std::cout << "Paramètres de la simulation :" << std::endl;
        std::cout << "  Longueur du terrain : " << params.length << std::endl;
        std::cout << "  Discrétisation : " << params.discretization << std::endl;
        std::cout << "  Vent : (" << params.wind[0] << ", " << params.wind[1] << ")" << std::endl;
        std::cout << "  Position initiale du foyer : (" << params.start[0] << ", " << params.start[1] << ")" << std::endl;
        std::cout << std::endl;

        auto displayer = Displayer::createOrGetInstance(params.discretization * 5, params.discretization * 5);
        std::vector<std::uint8_t> veg_buffer(params.discretization * params.discretization);
        std::vector<std::uint8_t> fire_buffer(params.discretization * params.discretization);
        bool running = true;

        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                    MPI_Send(&running, 1, MPI_CXX_BOOL, 1, 3, MPI_COMM_WORLD);
                    break;
                }
            }
            if (!running) break;

            MPI_Status status;
            int flag;
            MPI_Iprobe(1, 0, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                MPI_Recv(&running, 1, MPI_CXX_BOOL, 1, 0, MPI_COMM_WORLD, &status);
                if (!running) break;

                MPI_Recv(veg_buffer.data(), veg_buffer.size(), MPI_UINT8_T, 1, 1, MPI_COMM_WORLD, &status);
                MPI_Recv(fire_buffer.data(), fire_buffer.size(), MPI_UINT8_T, 1, 2, MPI_COMM_WORLD, &status);
                
                displayer->update(veg_buffer, fire_buffer);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    else {
        // Processus de calcul
        auto simu = Model(params.length, params.discretization, 
                         params.wind,
                         {static_cast<unsigned int>(params.start[0] * params.discretization), 
                          static_cast<unsigned int>(params.start[1] * params.discretization)},
                         10.0);  // Augmentation de la vitesse maximale du vent pour une meilleure propagation

        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();

        bool running = true;
        while (running) {
            int flag;
            MPI_Status status;
            MPI_Iprobe(0, 3, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                MPI_Recv(&running, 1, MPI_CXX_BOOL, 0, 3, MPI_COMM_WORLD, &status);
                break;
            }

            running = simu.update();
            MPI_Send(&running, 1, MPI_CXX_BOOL, 0, 0, MPI_COMM_WORLD);
            
            if (running) {
                auto veg_map = simu.vegetal_map();
                auto fire_map = simu.fire_map();
                MPI_Send(veg_map.data(), veg_map.size(), MPI_UINT8_T, 0, 1, MPI_COMM_WORLD);
                MPI_Send(fire_map.data(), fire_map.size(), MPI_UINT8_T, 0, 2, MPI_COMM_WORLD);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::cout << "Temps pour la simulation : " << elapsed_seconds.count() << " secondes" << std::endl;
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
} 