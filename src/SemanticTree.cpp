#include "SemanticTree.hpp"
#include <utility>

SemanticTree::SemanticTree(Formula::Ptr formula) : formula_(std::move(formula)) {}

void SemanticTree::renderNode(const Formula::Ptr& node, std::string prefix,
                              bool last, std::string& output) {
    output += prefix + (last ? "└── " : "├── ") + node->toString() + "\n";
    if (node->kind() == FormulaKind::Atom) return;
    const std::string childPrefix = prefix + (last ? "    " : "│   ");
    renderNode(node->left(), childPrefix, !node->right(), output);
    if (node->right()) renderNode(node->right(), childPrefix, true, output);
}

std::string SemanticTree::render() const {
    std::string result;
    renderNode(formula_, "", true, result);
    return result;
}

void SemanticTree::renderNode(
    const Formula::Ptr& node, std::string prefix, bool last,
    std::string& output, const std::map<std::string, std::string>& labels) {
    output += prefix + (last ? "└── " : "├── ") +
              node->toLabeledString(labels) + "\n";
    if (node->kind() == FormulaKind::Atom) return;
    const std::string childPrefix = prefix + (last ? "    " : "│   ");
    renderNode(node->left(), childPrefix, !node->right(), output, labels);
    if (node->right()) renderNode(node->right(), childPrefix, true, output, labels);
}

std::string SemanticTree::render(
    const std::map<std::string, std::string>& labels) const {
    std::string result;
    renderNode(formula_, "", true, result, labels);
    return result;
}
