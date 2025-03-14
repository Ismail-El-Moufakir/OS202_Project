#ifndef _SIMULATION_HPP_
#define _SIMULATION_HPP_

#include <array>

struct ParamsType
{
    double length = 1.0;
    int discretization = 100;
    std::array<double,2> wind = {0.0, 0.0};
    std::array<double,2> start = {0.5, 0.5};
};

bool analyze_args(int nargs, char* argv[], ParamsType& params);
bool check_params(const ParamsType& params);

#endif 