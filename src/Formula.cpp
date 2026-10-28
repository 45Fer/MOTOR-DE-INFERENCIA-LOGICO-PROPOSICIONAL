#include "Formula.hpp"
#include <stdexcept>
#include <utility>

Formula::Formula(FormulaKind kind, std::string atomName, Ptr left, Ptr right)
    : kind_(kind), atomName_(std::move(atomName)), left_(std::move(left)),
      right_(std::move(right)) {}

Formula::Ptr Formula::atom(const std::string& name) {
    if (name.empty()) throw std::invalid_argument("El átomo no puede estar vacío.");
    return Ptr(new Formula(FormulaKind::Atom, name, nullptr, nullptr));
}

Formula::Ptr Formula::unary(FormulaKind kind, Ptr operand) {
    if (kind != FormulaKind::Not || !operand)
        throw std::invalid_argument("Operador unario inválido.");
    return Ptr(new Formula(kind, "", std::move(operand), nullptr));
}

Formula::Ptr Formula::binary(FormulaKind kind, Ptr left, Ptr right) {
    if (!left || !right || kind == FormulaKind::Atom || kind == FormulaKind::Not)
        throw std::invalid_argument("Operador binario inválido.");
    return Ptr(new Formula(kind, "", std::move(left), std::move(right)));
}

FormulaKind Formula::kind() const noexcept { return kind_; }
const std::string& Formula::atomName() const noexcept { return atomName_; }
const Formula::Ptr& Formula::left() const noexcept { return left_; }
const Formula::Ptr& Formula::right() const noexcept { return right_; }

bool Formula::evaluate(const std::map<std::string, bool>& model) const {
    switch (kind_) {
    case FormulaKind::Atom: {
        const auto it = model.find(atomName_);
        if (it == model.end())
            throw std::invalid_argument("El modelo no contiene el átomo: " + atomName_);
        return it->second;
    }
    case FormulaKind::Not: return !left_->evaluate(model);
    case FormulaKind::And: return left_->evaluate(model) && right_->evaluate(model);
    case FormulaKind::Or: return left_->evaluate(model) || right_->evaluate(model);
    case FormulaKind::Implies: return !left_->evaluate(model) || right_->evaluate(model);
    case FormulaKind::Iff: return left_->evaluate(model) == right_->evaluate(model);
    }
    throw std::logic_error("Tipo de fórmula desconocido.");
}

void Formula::collectAtoms(std::set<std::string>& atoms) const {
    if (kind_ == FormulaKind::Atom) {
        atoms.insert(atomName_);
        return;
    }
    left_->collectAtoms(atoms);
    if (right_) right_->collectAtoms(atoms);
}

std::string Formula::toString() const {
    switch (kind_) {
    case FormulaKind::Atom: return atomName_;
    case FormulaKind::Not: return "¬" + left_->toString();
    case FormulaKind::And: return "(" + left_->toString() + " ∧ " + right_->toString() + ")";
    case FormulaKind::Or: return "(" + left_->toString() + " ∨ " + right_->toString() + ")";
    case FormulaKind::Implies: return "(" + left_->toString() + " → " + right_->toString() + ")";
    case FormulaKind::Iff: return "(" + left_->toString() + " ↔ " + right_->toString() + ")";
    }
    return {};
}

std::string Formula::toLabeledString(
    const std::map<std::string, std::string>& labels) const {
    if (kind_ == FormulaKind::Atom) {
        const auto label = labels.find(atomName_);
        return label == labels.end() ? atomName_ : label->second;
    }
    if (kind_ == FormulaKind::Not) return "no (" + left_->toLabeledString(labels) + ")";
    const std::string operation =
        kind_ == FormulaKind::And ? " y " :
        kind_ == FormulaKind::Or ? " o " :
        kind_ == FormulaKind::Implies ? " implica " : " si y solo si ";
    return "(" + left_->toLabeledString(labels) + operation +
           right_->toLabeledString(labels) + ")";
}
