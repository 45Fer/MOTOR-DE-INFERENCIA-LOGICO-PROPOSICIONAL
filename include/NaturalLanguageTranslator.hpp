#pragma once

#include <string>
#include <map>

class NaturalLanguageTranslator {
public:
    std::string translate(const std::string& sentence);
    void loadPhrases(const std::string& path);
    void savePhrases(const std::string& path) const;
    const std::map<std::string, std::string>& atomLabels() const noexcept;

private:
    static std::string normalize(const std::string& text);
    std::map<std::string, std::string> atoms_;
    std::map<std::string, std::string> labels_;
};
