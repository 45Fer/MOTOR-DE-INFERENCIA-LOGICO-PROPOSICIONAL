#include "FormulaParser.hpp"
#include <cctype>
#include <stdexcept>
#include <vector>

namespace {
struct Token { std::string text; };

std::vector<Token> tokenize(const std::string& input) {
    std::vector<Token> result;
    for (std::size_t i = 0; i < input.size();) {
        if (std::isspace(static_cast<unsigned char>(input[i]))) { ++i; continue; }
        if (input[i] == '(' || input[i] == ')') {
            result.push_back({std::string(1, input[i++])});
            continue;
        }
        if (input.compare(i, 2, "->") == 0 || input.compare(i, 2, "<>") == 0) {
            result.push_back({input.substr(i, 2)}); i += 2; continue;
        }
        if (input[i] == '!' || input[i] == '&' || input[i] == '|' ||
            input[i] == '~' || input[i] == '^') {
            result.push_back({std::string(1, input[i++])}); continue;
        }
        if (std::isalpha(static_cast<unsigned char>(input[i])) || input[i] == '_') {
            const auto start = i++;
            while (i < input.size() &&
                   (std::isalnum(static_cast<unsigned char>(input[i])) || input[i] == '_')) ++i;
            result.push_back({input.substr(start, i - start)});
            continue;
        }
        throw std::invalid_argument("Símbolo no reconocido en la posición " + std::to_string(i) + ".");
    }
    return result;
}

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}
    Formula::Ptr parse() {
        if (tokens_.empty()) throw std::invalid_argument("La fórmula está vacía.");
        auto value = iff();
        if (position_ != tokens_.size()) throw std::invalid_argument("Fórmula mal formada.");
        return value;
    }
private:
    Formula::Ptr iff() {
        auto value = implication();
        while (match("<>")) value = Formula::binary(FormulaKind::Iff, value, implication());
        return value;
    }
    Formula::Ptr implication() {
        auto value = disjunction();
        if (match("->")) value = Formula::binary(FormulaKind::Implies, value, implication());
        return value;
    }
    Formula::Ptr disjunction() {
        auto value = conjunction();
        while (match("|")) value = Formula::binary(FormulaKind::Or, value, conjunction());
        return value;
    }
    Formula::Ptr conjunction() {
        auto value = negation();
        while (match("&") || match("^")) value = Formula::binary(FormulaKind::And, value, negation());
        return value;
    }
    Formula::Ptr negation() {
        if (match("!") || match("~")) return Formula::unary(FormulaKind::Not, negation());
        if (match("(")) {
            auto value = iff();
            expect(")");
            return value;
        }
        if (position_ >= tokens_.size()) throw std::invalid_argument("Se esperaba un átomo.");
        const auto token = tokens_[position_++].text;
        if (token == ")" || token == "|" || token == "&" || token == "->")
            throw std::invalid_argument("Se esperaba un átomo.");
        return Formula::atom(token);
    }
    bool match(const std::string& value) {
        if (position_ < tokens_.size() && tokens_[position_].text == value) { ++position_; return true; }
        return false;
    }
    void expect(const std::string& value) {
        if (!match(value)) throw std::invalid_argument("Falta '" + value + "'.");
    }
    const std::vector<Token>& tokens_;
    std::size_t position_ = 0;
};
}

Formula::Ptr FormulaParser::parse(const std::string& expression) const {
    return Parser(tokenize(expression)).parse();
}
