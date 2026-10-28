#include "LogicEngine.hpp"
#include <utility>

void LogicEngine::loadPhrases(const std::string& path) {
    translator_.loadPhrases(path);
}

void LogicEngine::savePhrases(const std::string& path) const {
    translator_.savePhrases(path);
}

ParsedRule LogicEngine::addRule(const std::string& naturalLanguageRule) {
    NaturalLanguageTranslator candidate = translator_;
    ParsedRule rule{naturalLanguageRule, candidate.translate(naturalLanguageRule), nullptr};
    rule.formula = parser_.parse(rule.translated);
    translator_ = std::move(candidate);
    rules_.push_back(rule);
    return rule;
}

ModelReport LogicEngine::evaluate() const {
    std::vector<Formula::Ptr> formulas;
    for (const auto& rule : rules_) formulas.push_back(rule.formula);
    ModelReport report = ModelEvaluator().evaluate(formulas);
    report.atomLabels = translator_.atomLabels();
    return report;
}

std::string LogicEngine::semanticTree() const {
    std::string result;
    for (const auto& rule : rules_) {
        result += "Regla: " + rule.original + "\n";
        result += SemanticTree(rule.formula).render();
    }
    return result;
}

const std::vector<ParsedRule>& LogicEngine::rules() const noexcept { return rules_; }
