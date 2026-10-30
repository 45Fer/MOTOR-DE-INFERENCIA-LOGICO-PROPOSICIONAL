#include "Tableau.hpp"
#include <algorithm>
#include <sstream>

namespace {
std::string propositionLetter(std::size_t index) {
    if (index < 11) return std::string(1, static_cast<char>('p' + index));
    return "p" + std::to_string(index - 10);
}

std::string operatorText(FormulaKind kind) {
    switch (kind) {
    case FormulaKind::And: return "∧";
    case FormulaKind::Or: return "∨";
    case FormulaKind::Implies: return "→";
    case FormulaKind::Iff: return "↔";
    default: return {};
    }
}
}

Tableau::Tableau(const std::vector<Formula::Ptr>& formulas,
                 const std::vector<std::string>& atoms,
                 const std::map<std::string, std::string>& labels)
    : labels_(labels) {
    atoms_ = atoms;
    formulas_ = formulas;
    for (std::size_t index = 0; index < atoms.size(); ++index) {
        atomLetters_[atoms[index]] = propositionLetter(index);
    }

    Branch initial;
    for (const auto& formula : formulas) initial.push_back({formula, false});
    expand(initial, "");
}

const std::map<std::string, std::string>& Tableau::atomLetters() const noexcept {
    return atomLetters_;
}

std::string Tableau::formulaText(const Formula::Ptr& formula) const {
    return display({formula, false});
}

bool Tableau::satisfiable() const noexcept { return !openBranches_.empty(); }

std::string Tableau::display(const SignedFormula& signedFormula) const {
    const Formula::Ptr& formula = signedFormula.formula;
    if (formula->kind() == FormulaKind::Atom) {
        const auto letter = atomLetters_.find(formula->atomName());
        const std::string name = letter == atomLetters_.end()
                                     ? formula->atomName()
                                     : letter->second;
        return signedFormula.negated ? "¬" + name : name;
    }
    if (formula->kind() == FormulaKind::Not) {
        SignedFormula child{formula->left(), !signedFormula.negated};
        return display(child);
    }
    const SignedFormula left{formula->left(), false};
    const SignedFormula right{formula->right(), false};
    std::string value = "(" + display(left) + " " +
                        operatorText(formula->kind()) + " " + display(right) + ")";
    return signedFormula.negated ? "¬" + value : value;
}

std::string Tableau::assignment(const Branch& branch) const {
    std::map<std::string, bool> values;
    for (const auto& item : branch) {
        if (item.formula->kind() == FormulaKind::Atom)
            values[item.formula->atomName()] = !item.negated;
    }

    std::size_t missing = 0;
    for (const auto& atom : atoms_)
        if (values.find(atom) == values.end()) ++missing;
    const std::size_t combinations = std::size_t{1} << missing;
    std::map<std::string, bool> completed = values;
    for (std::size_t mask = 0; mask < combinations; ++mask) {
        std::size_t bit = 0;
        for (const auto& atom : atoms_) {
            if (values.find(atom) == values.end())
                completed[atom] = (mask & (std::size_t{1} << bit++)) != 0;
        }
        bool valid = true;
        for (const auto& formula : formulas_) valid = valid && formula->evaluate(completed);
        if (valid) break;
    }

    std::string result;
    for (std::size_t index = 0; index < atoms_.size(); ++index) {
        if (index) result += ", ";
        const auto letter = atomLetters_.find(atoms_[index]);
        result += letter == atomLetters_.end() ? atoms_[index] : letter->second;
        const auto value = completed.find(atoms_[index]);
        result += " = ";
        result += value != completed.end() && value->second ? "V" : "F";
    }
    return result;
}

bool Tableau::isLiteral(const SignedFormula& signedFormula) const {
    return signedFormula.formula->kind() == FormulaKind::Atom;
}

bool Tableau::closed(const Branch& branch) const {
    std::map<std::string, bool> values;
    for (const auto& item : branch) {
        if (!isLiteral(item)) continue;
        const bool value = !item.negated;
        const auto found = values.find(item.formula->atomName());
        if (found != values.end() && found->second != value) return true;
        values[item.formula->atomName()] = value;
    }
    return false;
}

void Tableau::renderBranch(const Branch& branch, const std::string& prefix,
                           const std::string& marker) {
    output_ += prefix + marker;
    for (std::size_t index = 0; index < branch.size(); ++index) {
        if (index) output_ += ", ";
        output_ += display(branch[index]);
    }
    output_ += "\r\n";
}

void Tableau::expand(const Branch& branch, const std::string& prefix) {
    if (closed(branch)) {
        renderBranch(branch, prefix, "✕ CERRADA: ");
        return;
    }

    auto item = std::find_if(branch.begin(), branch.end(),
                             [this](const SignedFormula& value) {
                                 return !isLiteral(value);
                             });
    if (item == branch.end()) {
        ++branchNumber_;
        renderBranch(branch, prefix, "○ ABIERTA R" + std::to_string(branchNumber_) + ": ");
        openBranches_.push_back(branch);
        return;
    }

    const SignedFormula current = *item;
    Branch base = branch;
    base.erase(base.begin() + (item - branch.begin()));
    const FormulaKind kind = current.formula->kind();
    const bool negated = current.negated;
    output_ += prefix + "✓ Aplicada: " +
               (kind == FormulaKind::Not ? "¬" : display(current)) + "\r\n";
    if (kind == FormulaKind::Not) {
        base.push_back({current.formula->left(), !negated});
        expand(base, prefix + "  ");
        return;
    }

    const bool alpha = (!negated && kind == FormulaKind::And) ||
                       (negated && kind == FormulaKind::Or) ||
                       (negated && kind == FormulaKind::Implies);
    if (alpha) {
        if (negated && kind == FormulaKind::Implies) {
            base.push_back({current.formula->left(), false});
            base.push_back({current.formula->right(), true});
        } else {
            base.push_back({current.formula->left(), negated});
            base.push_back({current.formula->right(), negated});
        }
        expand(base, prefix + "  ");
        return;
    }

    Branch first = base;
    Branch second = base;
    if (kind == FormulaKind::And) {
        first.push_back({current.formula->left(), true});
        second.push_back({current.formula->right(), true});
    } else if (kind == FormulaKind::Or) {
        first.push_back({current.formula->left(), false});
        second.push_back({current.formula->right(), false});
    } else if (kind == FormulaKind::Implies) {
        first.push_back({current.formula->left(), true});
        second.push_back({current.formula->right(), false});
    } else {
        if (negated) {
            first.push_back({current.formula->left(), false});
            first.push_back({current.formula->right(), true});
            second.push_back({current.formula->left(), true});
            second.push_back({current.formula->right(), false});
        } else {
            first.push_back({current.formula->left(), false});
            first.push_back({current.formula->right(), false});
            second.push_back({current.formula->left(), true});
            second.push_back({current.formula->right(), true});
        }
    }
    output_ += prefix + "├── Rama A\r\n";
    expand(first, prefix + "│   ");
    output_ += prefix + "└── Rama B\r\n";
    expand(second, prefix + "    ");
}

std::string Tableau::render() const {
    std::string result =
        "============================================================\r\n"
        "                    TABLEAU SEMANTICO\r\n"
        "============================================================\r\n\r\n";
    result += output_;
    result += "\r\n";
    if (satisfiable()) {
        result += "RAMAS ABIERTAS: " + std::to_string(openBranches_.size()) + "\r\n";
        for (std::size_t index = 0; index < openBranches_.size(); ++index) {
            result += "R" + std::to_string(index + 1) + ": ";
            for (std::size_t item = 0; item < openBranches_[index].size(); ++item) {
                if (item) result += ", ";
                result += assignment(openBranches_[index]);
                break;
            }
            result += "\r\n";
        }
        result += "La base de conocimiento es SATISFACTIBLE.\r\n";
    } else {
        result += "Todas las ramas se cerraron.\r\n";
        result += "La base de conocimiento es INSATISFACTIBLE.\r\n";
    }
    return result;
}
