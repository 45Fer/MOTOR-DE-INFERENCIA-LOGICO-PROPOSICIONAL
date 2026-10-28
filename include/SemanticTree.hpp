#pragma once

#include "Formula.hpp"
#include <map>
#include <string>

class SemanticTree {
public:
    explicit SemanticTree(Formula::Ptr formula);
    std::string render() const;
    std::string render(
        const std::map<std::string, std::string>& labels) const;

private:
    Formula::Ptr formula_;
    static void renderNode(const Formula::Ptr& node, std::string prefix,
                           bool last, std::string& output);
    static void renderNode(const Formula::Ptr& node, std::string prefix,
                           bool last, std::string& output,
                           const std::map<std::string, std::string>& labels);
};
