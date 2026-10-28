#include "ModelReport.hpp"
#include <sstream>
#include <stdexcept>

ModelReport ModelEvaluator::evaluate(const std::vector<Formula::Ptr>& formulas) const {
    ModelReport report;
    std::set<std::string> atomSet;
    for (const auto& formula : formulas) formula->collectAtoms(atomSet);
    report.atoms.assign(atomSet.begin(), atomSet.end());
    if (report.atoms.size() >= sizeof(std::size_t) * 8)
        throw std::invalid_argument("Demasiados átomos para enumerar modelos.");
    const std::size_t combinations = std::size_t{1} << report.atoms.size();
    for (std::size_t mask = 0; mask < combinations; ++mask) {
        std::map<std::string, bool> model;
        for (std::size_t i = 0; i < report.atoms.size(); ++i)
            model[report.atoms[i]] = (mask & (std::size_t{1} << i)) != 0;
        bool valid = true;
        for (const auto& formula : formulas) valid = valid && formula->evaluate(model);
        if (valid) {
            std::vector<bool> oneHot;
            for (const auto& atom : report.atoms) oneHot.push_back(model.at(atom));
            report.models.push_back(std::move(model));
            report.oneHotModels.push_back(std::move(oneHot));
        }
    }
    return report;
}

std::string ModelReport::toString() const {
    std::ostringstream output;
    output << (satisfiable() ? "SATISFACTIBLE" : "INSATISFACTIBLE") << "\n";
    output << "Átomos: ";
    for (std::size_t i = 0; i < atoms.size(); ++i) {
        const auto label = atomLabels.find(atoms[i]);
        output << (i ? ", " : "")
               << (label == atomLabels.end() ? atoms[i] : label->second);
    }
    output << "\nModelos: " << models.size() << "\n";
    for (std::size_t modelIndex = 0; modelIndex < models.size(); ++modelIndex) {
        const auto& model = models[modelIndex];
        output << "{ ";
        for (std::size_t i = 0; i < atoms.size(); ++i) {
            const auto label = atomLabels.find(atoms[i]);
            output << (i ? ", " : "")
                   << (label == atomLabels.end() ? atoms[i] : label->second)
                   << "=" << (model.at(atoms[i]) ? "V" : "F");
        }
        output << " }  One Hot: [";
        for (std::size_t i = 0; i < oneHotModels[modelIndex].size(); ++i)
            output << (i ? ", " : "") << (oneHotModels[modelIndex][i] ? "1" : "0");
        output << "]\n";
    }
    return output.str();
}
