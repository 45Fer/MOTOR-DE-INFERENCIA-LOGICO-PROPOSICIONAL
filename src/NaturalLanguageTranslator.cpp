#include "NaturalLanguageTranslator.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace {
void replaceAll(std::string& text, const std::string& from, const std::string& to) {
    std::size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, from.size(), to);
        position += to.size();
    }
}

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(' ');
    return text.substr(first, last - first + 1);
    }
}

std::string NaturalLanguageTranslator::normalize(const std::string& text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c >= 0x80) {
            result += static_cast<char>(c);
            if (i + 1 < text.size()) result += text[++i];
        } else {
            result += static_cast<char>(std::tolower(c));
        }
    }
    return result;
}

void NaturalLanguageTranslator::loadPhrases(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        if (std::filesystem::exists(path)) {
            throw std::runtime_error("No se pudo abrir el archivo de frases aprendidas: " + path);
        }
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::string phrase = trim(normalize(line));
        if (phrase.empty() || atoms_.count(phrase) != 0) continue;

        const std::string atom = "a" + std::to_string(atoms_.size());
        atoms_[phrase] = atom;
        labels_[atom] = phrase;
    }
    if (input.bad()) {
        throw std::runtime_error("Error al leer el archivo de frases aprendidas: " + path);
    }
}

void NaturalLanguageTranslator::savePhrases(const std::string& path) const {
    const std::filesystem::path filePath(path);
    if (!filePath.parent_path().empty()) {
        std::error_code error;
        std::filesystem::create_directories(filePath.parent_path(), error);
        if (error) {
            throw std::runtime_error(
                "No se pudo crear la carpeta de frases aprendidas: " + error.message());
        }
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("No se pudo guardar el archivo de frases aprendidas: " + path);
    }
    for (const auto& entry : labels_) output << entry.second << '\n';
    if (!output) {
        throw std::runtime_error("Error al escribir el archivo de frases aprendidas: " + path);
    }
}

std::string NaturalLanguageTranslator::translate(const std::string& sentence) {
    std::string text = normalize(sentence);
    if (text.empty()) throw std::invalid_argument("La regla no puede estar vacía.");
    if (text.back() == '.') text.pop_back();
    const bool startsWithSi = text.compare(0, 3, "si ") == 0;
    const auto commaPosition = text.find(',');
    replaceAll(text, ",", " ");
    replaceAll(text, "si y solo si", " <> ");
    replaceAll(text, "si solo si", " <> ");
    replaceAll(text, "equivale a", " <> ");
    replaceAll(text, "es equivalente a", " <> ");
    replaceAll(text, "implica que", " -> ");
    replaceAll(text, "implica", " -> ");
    replaceAll(text, "ademas de", " & ");
    replaceAll(text, "ademas", " & ");
    replaceAll(text, " y ", " & ");
    replaceAll(text, " o bien ", " | ");
    replaceAll(text, " o ", " | ");

    const auto ifPosition = text.find("si ");
    const auto thenPosition = text.find(" entonces ");
    if (ifPosition == 0 && thenPosition != std::string::npos) {
        const std::string antecedent = text.substr(3, thenPosition - 3);
        const std::string consequent = text.substr(thenPosition + 10);
        text = "( " + antecedent + " ) -> ( " + consequent + " )";
    } else if (startsWithSi && commaPosition != std::string::npos) {
        const std::string antecedent = text.substr(3, commaPosition - 3);
        const std::string consequent = text.substr(commaPosition);
        text = "( " + antecedent + " ) -> ( " + consequent + " )";
    } else {
        replaceAll(text, " entonces ", " -> ");
    }
    std::string symbolic;
    std::string fragment;
    bool negateNext = false;
    auto flushFragment = [&]() {
        const std::string phrase = trim(fragment);
        fragment.clear();
        if (phrase.empty()) throw std::invalid_argument("Falta una proposición en la regla.");
        auto it = atoms_.find(phrase);
        if (it == atoms_.end()) {
            const std::string atom = "a" + std::to_string(atoms_.size());
            it = atoms_.emplace(phrase, atom).first;
            labels_[atom] = phrase;
        }
        if (negateNext) {
            symbolic += "! ";
            negateNext = false;
        }
        symbolic += it->second + " ";
    };

    std::istringstream input(text);
    std::string word;
    while (input >> word) {
        if (word == "no" || word == "nunca" || word == "!") {
            if (!fragment.empty()) flushFragment();
            negateNext = true;
        } else if (word == "&" || word == "|" || word == "->" || word == "<>" ||
                   word == "(" || word == ")") {
            if (!fragment.empty()) flushFragment();
            symbolic += word + " ";
        } else {
            if (!fragment.empty()) fragment += " ";
            fragment += word;
        }
    }
    if (!fragment.empty()) flushFragment();
    if (negateNext) throw std::invalid_argument("Falta una proposición después de la negación.");
    return symbolic;
}

const std::map<std::string, std::string>&
NaturalLanguageTranslator::atomLabels() const noexcept {
    return labels_;
}
