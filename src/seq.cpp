#include <string>
#include <vector>
#include "model.hpp"

struct ParamsType {
    double length{10.};
    unsigned discretization{300u};
    std::array<double,2> wind{0.,0.};
    Model::LexicoIndices start{10u,10u};
};

int main() {
    ParamsType params;
    auto simu = Model(params.length, params.discretization, 
                     params.wind, params.start);
    const int MAX_ITERATIONS = 200;
    int iteration = 0;
    
    while(simu.update() && iteration < MAX_ITERATIONS) {
        std::vector<Model::LexicoIndices> front_indices;
        for (const auto& pair : simu.m_fire_front) {
            front_indices.push_back(
                simu.get_lexicographic_from_index(pair.first));
        }
        iteration++;
    }
    return 0;
}