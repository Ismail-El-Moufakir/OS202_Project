#include <string>
#include <vector>
#include <chrono>
#include <omp.h>
#include "model.hpp"
#include <iostream>

struct ParamsType {
    double length{10.};
    unsigned discretization{300u};
    std::array<double,2> wind{0.,0.};
    Model::LexicoIndices start{10u,10u};
};

void run_simulation(int num_threads) {
    ParamsType params;
    auto simu = Model(params.length, params.discretization, 
                     params.wind, params.start);
    const int MAX_ITERATIONS = 300;
    int iteration = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    omp_set_num_threads(num_threads);
    
    while(simu.update() && iteration < MAX_ITERATIONS) {
        std::vector<Model::LexicoIndices> front_indices;
        
        #pragma omp parallel
        {
            std::vector<Model::LexicoIndices> private_indices;
            #pragma omp for
            for (size_t i = 0; i < simu.m_fire_front.size(); i++) {
                auto it = simu.m_fire_front.begin();
                std::advance(it, i);
                private_indices.push_back(
                    simu.get_lexicographic_from_index(it->first));
            }
            #pragma omp critical
            front_indices.insert(front_indices.end(), 
                               private_indices.begin(), 
                               private_indices.end());
        }
        iteration++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                   (end_time - start_time);
    std::cout << "Execution time with " << num_threads 
              << " threads: " << duration.count() << " ms" << std::endl;
}

int main() {
    std::vector<int> thread_counts = {1, 2, 4, 8};
    for (int threads : thread_counts) {
        run_simulation(threads);
    }
    return 0;
}