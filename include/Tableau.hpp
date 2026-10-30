#pragma once

#include "Formula.hpp"
#include <map>
#include <string>
#include <vector>

class Tableau {
public:
    Tableau(const std::vector<Formula::Ptr>& formulas,
            const std::vector<std::string>& atoms,
            const std::map<std::string, std::string>& labels);

    const std::map<std::string, std::string>& atomLetters() const noexcept;
    std::string formulaText(const Formula::Ptr& formula) const;
    std::string render() const;
    bool satisfiable() const noexcept;

private:
    struct SignedFormula {
        Formula::Ptr formula;
        bool negated;
    };

    using Branch = std::vector<SignedFormula>;

    std::map<std::string, std::string> atomLetters_;
    std::vector<std::string> atoms_;
    std::vector<Formula::Ptr> formulas_;
    std::map<std::string, std::string> labels_;
    std::vector<Branch> openBranches_;
    std::string output_;
    int branchNumber_ = 0;

    std::string display(const SignedFormula& formula) const;
    std::string assignment(const Branch& branch) const;
    bool isLiteral(const SignedFormula& formula) const;
    bool closed(const Branch& branch) const;
    void expand(const Branch& branch, const std::string& prefix);
    void renderBranch(const Branch& branch, const std::string& prefix,
                      const std::string& marker);
};
