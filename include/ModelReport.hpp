#pragma once

#include "Formula.hpp"
#include <map>
#include <string>
#include <vector>

struct ModelReport {
    std::vector<std::string> atoms;
    std::map<std::string, std::string> atomLabels;
    std::vector<std::map<std::string, bool>> models;
    std::vector<std::vector<bool>> oneHotModels;
    bool satisfiable() const noexcept { return !models.empty(); }
    std::string toString() const;
};

class ModelEvaluator {
public:
    ModelReport evaluate(const std::vector<Formula::Ptr>& formulas) const;
};
