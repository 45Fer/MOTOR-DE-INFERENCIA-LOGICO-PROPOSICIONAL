#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

enum class FormulaKind { Atom, Not, And, Or, Implies, Iff };

class Formula {
public:
    using Ptr = std::shared_ptr<const Formula>;

    static Ptr atom(const std::string& name);
    static Ptr unary(FormulaKind kind, Ptr operand);
    static Ptr binary(FormulaKind kind, Ptr left, Ptr right);

    FormulaKind kind() const noexcept;
    const std::string& atomName() const noexcept;
    const Ptr& left() const noexcept;
    const Ptr& right() const noexcept;
    bool evaluate(const std::map<std::string, bool>& model) const;
    void collectAtoms(std::set<std::string>& atoms) const;
    std::string toString() const;
    std::string toLabeledString(
        const std::map<std::string, std::string>& labels) const;

private:
    Formula(FormulaKind kind, std::string atomName, Ptr left, Ptr right);
    FormulaKind kind_;
    std::string atomName_;
    Ptr left_;
    Ptr right_;
};
