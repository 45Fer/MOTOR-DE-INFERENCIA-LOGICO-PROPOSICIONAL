#include "LogicEngine.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {
const char* const learnedPhrasesPath = "data/frases_aprendidas.txt";
}

int main() {
    LogicEngine engine;
    try {
        engine.loadPhrases(learnedPhrasesPath);
    } catch (const std::exception& error) {
        std::cerr << "[ERROR] " << error.what() << "\n";
        return 1;
    }
    std::cout << "============================================================\n"
              << "       MOTOR DE INFERENCIA LOGICO-PROPOSICIONAL\n"
              << "============================================================\n"
              << "Escribe frases completas, una por linea.\n"
              << "Cuando termines, escribe solamente: fin\n\n"
              << "Ejemplo:\n"
              << "  Si una persona estudia, entonces aprende.\n"
              << "  Una persona estudia.\n"
              << "  fin\n\n";
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "fin" || line == "FIN" || line == "Fin") break;
        if (line.empty()) continue;
        try {
            const ParsedRule rule = engine.addRule(line);
            engine.savePhrases(learnedPhrasesPath);
            std::cout << "  [OK] Frase recibida: " << rule.original << "\n"
                      << "      Interpretacion: "
                      << rule.formula->toLabeledString(
                             engine.evaluate().atomLabels) << "\n\n";
        } catch (const std::exception& error) {
            std::cerr << "  [ERROR] " << error.what() << "\n"
                      << "  Corrige la frase e intenta nuevamente.\n\n";
        }
    }
    if (engine.rules().empty()) {
        std::cerr << "No se ingresaron frases. El programa terminara.\n";
        return 1;
    }
    const ModelReport report = engine.evaluate();
    std::cout << "\n============================================================\n"
              << "                    RESULTADO\n"
              << "============================================================\n"
              << "Las frases ingresadas son "
              << (report.satisfiable() ? "COMPATIBLES" : "INCOMPATIBLES")
              << ".\n\n";
    std::cout << report.toString();
    std::cout << "\nArbol semantico:\n";
    for (const auto& rule : engine.rules()) {
        std::cout << "Frase: " << rule.original << "\n"
                  << SemanticTree(rule.formula).render(report.atomLabels);
    }
}
