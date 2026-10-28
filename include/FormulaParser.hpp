#pragma once

#include "Formula.hpp"
#include <string>

class FormulaParser {
public:
    Formula::Ptr parse(const std::string& expression) const;
};
