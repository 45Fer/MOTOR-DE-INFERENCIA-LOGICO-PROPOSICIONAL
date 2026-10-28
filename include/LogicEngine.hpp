#pragma once

#include "FormulaParser.hpp"
#include "ModelReport.hpp"
#include "NaturalLanguageTranslator.hpp"
#include "SemanticTree.hpp"
#include <string>
#include <vector>

struct ParsedRule {
    std::string original;
    std::string translated;
    Formula::Ptr formula;
};

class LogicEngine {
public:
    void loadPhrases(const std::string& path);
    void savePhrases(const std::string& path) const;
    ParsedRule addRule(const std::string& naturalLanguageRule);
    ModelReport evaluate() const;
    std::string semanticTree() const;
    const std::vector<ParsedRule>& rules() const noexcept;

private:
    NaturalLanguageTranslator translator_;
    FormulaParser parser_;
    std::vector<ParsedRule> rules_;
};
