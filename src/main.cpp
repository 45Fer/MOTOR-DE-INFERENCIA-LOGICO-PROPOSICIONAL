#include "LogicEngine.hpp"
#include "Tableau.hpp"
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <exception>
#include <sstream>
#include <string>
#include <vector>
#include <functional>

namespace {
constexpr int InputId = 1001;
constexpr int AnalyzeId = 1002;
constexpr int ClearId = 1003;
constexpr int ResultId = 1004;
constexpr int StatusId = 1005;
constexpr int AtomsId = 1006;
constexpr int DiagramId = 1007;
constexpr int ShowDiagramId = 1008;
constexpr int DiagramTitleId = 1009;
constexpr int BackDiagramId = 1010;
constexpr int AtomsViewId = 1011;
constexpr int RulesViewId = 1012;
constexpr int TableauViewId = 1013;

const char* const learnedPhrasesPath = "data/frases_aprendidas.txt";
HFONT bodyFont;
HFONT headingFont;
HFONT smallFont;
HFONT treeFont;
HBRUSH backgroundBrush;
std::vector<Formula::Ptr> diagramFormulas;
std::vector<std::string> diagramOriginals;
std::map<std::string, std::string> diagramLabels;
HWND diagramWindow = nullptr;
bool diagramMode = false;
bool tableauCanvasMode = false;
bool treeResultMode = false;
std::string lastResult;
std::string semanticTreeResult;
std::string atomsResult;
std::string rulesResult;
std::string tableauResult;

std::string propositionLetter(std::size_t index) {
    if (index < 11) return std::string(1, static_cast<char>('p' + index));
    return "p" + std::to_string(index - 10);
}

std::string formulaWithLetters(
    const Formula::Ptr& formula,
    const std::map<std::string, std::string>& letters) {
    if (formula->kind() == FormulaKind::Atom) {
        const auto found = letters.find(formula->atomName());
        return found == letters.end() ? formula->atomName() : found->second;
    }
    if (formula->kind() == FormulaKind::Not)
        return "¬(" + formulaWithLetters(formula->left(), letters) + ")";
    const std::string operation =
        formula->kind() == FormulaKind::And ? " ∧ " :
        formula->kind() == FormulaKind::Or ? " ∨ " :
        formula->kind() == FormulaKind::Implies ? " → " : " ↔ ";
    return "(" + formulaWithLetters(formula->left(), letters) + operation +
           formulaWithLetters(formula->right(), letters) + ")";
}

void setDiagramView(HWND window, bool showDiagram) {
    diagramMode = showDiagram;
    if (showDiagram) {
        EnumChildWindows(
            window,
            [](HWND child, LPARAM) -> BOOL {
                ShowWindow(child, SW_HIDE);
                return TRUE;
            },
            0);
        MoveWindow(GetDlgItem(window, DiagramTitleId), 44, 28, 300, 30, TRUE);
        MoveWindow(GetDlgItem(window, DiagramId), 44, 70, 700, 780, TRUE);
        MoveWindow(GetDlgItem(window, BackDiagramId), 610, 28, 134, 32, TRUE);
        ShowWindow(GetDlgItem(window, DiagramTitleId), SW_SHOW);
        ShowWindow(GetDlgItem(window, DiagramId), SW_HIDE);
        ShowWindow(GetDlgItem(window, BackDiagramId), SW_SHOW);
        BringWindowToTop(GetDlgItem(window, DiagramId));
        InvalidateRect(window, nullptr, TRUE);
        UpdateWindow(window);
    } else {
        EnumChildWindows(
            window,
            [](HWND child, LPARAM) -> BOOL {
                ShowWindow(child, SW_SHOW);
                return TRUE;
            },
            0);
        ShowWindow(GetDlgItem(window, DiagramTitleId), SW_HIDE);
        ShowWindow(GetDlgItem(window, DiagramId), SW_HIDE);
        ShowWindow(GetDlgItem(window, BackDiagramId), SW_HIDE);
        InvalidateRect(window, nullptr, TRUE);
    }
}

std::wstring toWide(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(),
                                         static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), size);
    return result;
}

std::string toUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                         static_cast<int>(value.size()), nullptr, 0,
                                         nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), size, nullptr, nullptr);
    return result;
}

void setText(HWND control, const std::string& value) {
    SetWindowTextW(control, toWide(value).c_str());
}

void showResultView(HWND window, const std::string& value, bool tree) {
    HWND resultControl = GetDlgItem(window, ResultId);
    setText(resultControl, value);
    SendMessageW(resultControl, WM_SETFONT,
                 reinterpret_cast<WPARAM>(tree ? treeFont : bodyFont), TRUE);
    RECT clientArea{};
    GetClientRect(window, &clientArea);
    MoveWindow(resultControl, 44, 447,
               std::max(400, static_cast<int>(clientArea.right) - 88),
               tree ? std::max(200, static_cast<int>(clientArea.bottom) - 490) : 155,
               TRUE);
    SendMessageW(resultControl, EM_SETSEL, 0, 0);
    SendMessageW(resultControl, EM_SCROLL, SB_TOP, 0);
    ShowWindow(resultControl, SW_SHOW);
    ShowWindow(GetDlgItem(window, AtomsId), tree ? SW_HIDE : SW_SHOW);
    treeResultMode = tree;
    InvalidateRect(resultControl, nullptr, TRUE);
    UpdateWindow(resultControl);
}

std::string getText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring value(length, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    return toUtf8(value);
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty() && line.find_first_not_of(" \t") != std::string::npos)
            lines.push_back(line);
    }
    return lines;
}

void appendLine(std::string& output, const std::string& line) {
    if (!output.empty()) output += "\r\n";
    output += line;
}

void showError(HWND owner, const std::string& message) {
    MessageBoxW(owner, toWide(message).c_str(), L"No se pudo analizar",
                MB_OK | MB_ICONWARNING);
}

void appendSimpleTree(const Formula::Ptr& node,
                      const std::map<std::string, std::string>& labels,
                      const std::string& prefix, bool root, std::string& output) {
    output += prefix + (root ? "- " : "|_ ") +
              node->toLabeledString(labels) + "\r\n";
    if (node->kind() == FormulaKind::Atom) return;

    appendSimpleTree(node->left(), labels, prefix + "    ", false, output);
    if (node->right()) {
        appendSimpleTree(node->right(), labels, prefix + "    ", false, output);
    }
}

void analyze(HWND window, bool buildTableau = false) {
    const HWND input = GetDlgItem(window, InputId);
    const std::vector<std::string> lines = splitLines(getText(input));
    if (lines.empty()) {
        showError(window, "Agrega al menos una proposición antes de analizar.");
        return;
    }

    LogicEngine engine;
    try {
        engine.loadPhrases(learnedPhrasesPath);
        for (const auto& line : lines) engine.addRule(line);
        engine.savePhrases(learnedPhrasesPath);
        const ModelReport report = engine.evaluate();
        tableauResult.clear();
        std::vector<Formula::Ptr> formulas;
        if (buildTableau) {
            for (const auto& rule : engine.rules()) formulas.push_back(rule.formula);
            const Tableau tableau(formulas, report.atoms, report.atomLabels);
            tableauResult = tableau.render();
        }

        std::string result;
        appendLine(result, report.satisfiable()
                              ? "VEREDICTO: COMPATIBLE"
                              : "VEREDICTO: INCOMPATIBLE");
        appendLine(result, report.satisfiable()
                              ? "Las frases pueden ser verdaderas al mismo tiempo."
                              : "No existe una asignación que satisfaga todas las frases.");
        appendLine(result, "");
        appendLine(result, "MODELOS VÁLIDOS: " + std::to_string(report.models.size()));
        appendLine(result, "ÁTOMOS: " + std::to_string(report.atoms.size()));
        appendLine(result, "");

        for (std::size_t modelIndex = 0; modelIndex < report.models.size(); ++modelIndex) {
            std::string model = "Modelo " + std::to_string(modelIndex + 1) + ": { ";
            for (std::size_t atomIndex = 0; atomIndex < report.atoms.size(); ++atomIndex) {
                if (atomIndex) model += ", ";
                const auto label = report.atomLabels.find(report.atoms[atomIndex]);
                model += label == report.atomLabels.end() ? report.atoms[atomIndex]
                                                            : label->second;
                model += "=";
                model += report.models[modelIndex].at(report.atoms[atomIndex]) ? "V" : "F";
            }
            model += " }  One Hot: [";
            for (std::size_t valueIndex = 0;
                 valueIndex < report.oneHotModels[modelIndex].size(); ++valueIndex) {
                if (valueIndex) model += ", ";
                model += report.oneHotModels[modelIndex][valueIndex] ? "1" : "0";
            }
            model += "]";
            appendLine(result, model);
        }

        setText(GetDlgItem(window, ResultId), result);
        SendMessageW(GetDlgItem(window, ResultId), WM_SETFONT,
                     reinterpret_cast<WPARAM>(bodyFont), TRUE);
        lastResult = result;
        semanticTreeResult =
            "============================================================\r\n"
            "                    ARBOL SEMANTICO\r\n"
            "============================================================\r\n\r\n";
        for (std::size_t index = 0; index < engine.rules().size(); ++index) {
            const auto& rule = engine.rules()[index];
            semanticTreeResult += "REGLA " + std::to_string(index + 1) + "\r\n";
            semanticTreeResult += "Frase original: " + rule.original + "\r\n";
            semanticTreeResult += "Estructura:\r\n";
            appendSimpleTree(rule.formula, report.atomLabels, "    ", true,
                             semanticTreeResult);
            semanticTreeResult +=
                "------------------------------------------------------------\r\n\r\n";
        }
        atomsResult = "==================== TABLA DE ATOMOS ====================\r\n\r\n";
        atomsResult += "Letra\tSignificado\r\n";
        atomsResult += "-----\t-----------\r\n";
        for (std::size_t index = 0; index < report.atoms.size(); ++index) {
            const auto label = report.atomLabels.find(report.atoms[index]);
            atomsResult += propositionLetter(index) + "\t" +
                           (label == report.atomLabels.end() ? report.atoms[index]
                                                              : label->second) +
                           "\r\n";
        }
        atomsResult += "\r\nTotal de atomos: " + std::to_string(report.atoms.size()) + "\r\n";
        rulesResult = "==================== REGLAS ====================\r\n\r\n";
        std::map<std::string, std::string> formulaLetters;
        for (std::size_t index = 0; index < report.atoms.size(); ++index)
            formulaLetters[report.atoms[index]] = propositionLetter(index);
        for (std::size_t index = 0; index < engine.rules().size(); ++index) {
            const auto& rule = engine.rules()[index];
            rulesResult += "R" + std::to_string(index + 1) + ":\r\n";
            rulesResult += "Original: " + rule.original + "\r\n";
            rulesResult += "Formula: " + formulaWithLetters(rule.formula, formulaLetters) +
                           "\r\n\r\n";
        }
        treeResultMode = false;
        SetWindowTextW(GetDlgItem(window, AnalyzeId), L"MODELO");
        SetWindowTextW(GetDlgItem(window, ShowDiagramId), L"ARBOL SEMANTICO");
        diagramFormulas.clear();
        diagramOriginals.clear();
        for (const auto& rule : engine.rules()) {
            diagramFormulas.push_back(rule.formula);
            diagramOriginals.push_back(rule.original);
        }
        diagramLabels = report.atomLabels;
        showResultView(window, result, false);
        setDiagramView(window, false);
        if (diagramWindow) InvalidateRect(diagramWindow, nullptr, TRUE);
        InvalidateRect(GetDlgItem(window, DiagramId), nullptr, TRUE);
        std::string atomSummary;
        for (const auto& atom : report.atoms) {
            const auto label = report.atomLabels.find(atom);
            if (!atomSummary.empty()) atomSummary += "  ·  ";
            atomSummary += label == report.atomLabels.end() ? atom : label->second;
        }
        setText(GetDlgItem(window, AtomsId),
                atomSummary.empty() ? "No se detectaron átomos." : atomSummary);
        setText(GetDlgItem(window, StatusId), "Análisis completado correctamente");
    } catch (const std::exception& error) {
        showError(window, error.what());
        setText(GetDlgItem(window, StatusId), "Corrige las reglas e intenta nuevamente");
    }
}

LRESULT CALLBACK diagramProcedure(HWND window, UINT message, WPARAM, LPARAM) {
    if (message == WM_DESTROY) {
        if (window == diagramWindow) diagramWindow = nullptr;
        return 0;
    }
    if (message == WM_SIZE) {
        InvalidateRect(window, nullptr, TRUE);
        return 0;
    }
    if (message != WM_PAINT) return DefWindowProcW(window, message, 0, 0);

    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(window, &paint);
    RECT area{};
    GetClientRect(window, &area);
    HBRUSH canvasBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &area, canvasBrush);
    DeleteObject(canvasBrush);
    SetBkMode(dc, TRANSPARENT);

    if (diagramFormulas.empty()) {
        SelectObject(dc, smallFont);
        SetTextColor(dc, RGB(129, 145, 153));
        DrawTextW(dc, L"Analiza tus proposiciones para visualizar el diagrama.",
                  -1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(window, &paint);
        return 0;
    }

    struct NodePosition {
        Formula::Ptr formula;
        int x;
        int y;
    };
    constexpr int nodeWidth = 132;
    constexpr int nodeHeight = 52;
    constexpr int levelGap = 74;
    constexpr int canvasMargin = 82;
    int currentY = 38;

    auto labelFor = [](const Formula::Ptr& formula) {
        if (formula->kind() == FormulaKind::Atom) {
            const auto label = diagramLabels.find(formula->atomName());
            return label == diagramLabels.end() ? formula->atomName() : label->second;
        }
        switch (formula->kind()) {
        case FormulaKind::Not: return std::string("¬");
        case FormulaKind::And: return std::string("∧");
        case FormulaKind::Or: return std::string("∨");
        case FormulaKind::Implies: return std::string("→");
        case FormulaKind::Iff: return std::string("↔");
        default: return std::string();
        }
    };

    for (std::size_t ruleIndex = 0; ruleIndex < diagramFormulas.size(); ++ruleIndex) {
        const Formula::Ptr& root = diagramFormulas[ruleIndex];
        std::vector<NodePosition> nodes;
        std::vector<std::pair<int, int>> edges;
        std::function<int(const Formula::Ptr&, int, int&, int)> layout =
            [&](const Formula::Ptr& formula, int depth, int& nextLeaf, int leafCount) {
                const int index = static_cast<int>(nodes.size());
                nodes.push_back({formula, 0, currentY + depth * levelGap});
                if (formula->kind() == FormulaKind::Atom) {
                    const int usableWidth =
                        std::max(1, static_cast<int>(area.right) - 2 * canvasMargin);
                    nodes[index].x = canvasMargin +
                                     (nextLeaf++ * usableWidth) /
                                         std::max(1, leafCount - 1);
                    return index;
                }
                const int left = layout(formula->left(), depth + 1, nextLeaf, leafCount);
                int right = -1;
                if (formula->right())
                    right = layout(formula->right(), depth + 1, nextLeaf, leafCount);
                nodes[index].x = right < 0
                                     ? nodes[left].x
                                     : (nodes[left].x + nodes[right].x) / 2;
                edges.push_back({index, left});
                if (right >= 0) edges.push_back({index, right});
                return index;
            };
            std::function<int(const Formula::Ptr&)> countLeaves =
                [&](const Formula::Ptr& formula) {
                    if (formula->kind() == FormulaKind::Atom) return 1;
                    return countLeaves(formula->left()) +
                           (formula->right() ? countLeaves(formula->right()) : 0);
                };
            int nextLeaf = 0;
            layout(root, 0, nextLeaf, countLeaves(root));

            SelectObject(dc, smallFont);
            SetTextColor(dc, RGB(77, 93, 104));
            std::wstring title = L"Proposición " + std::to_wstring(ruleIndex + 1);
            TextOutW(dc, 18, currentY - 28, title.c_str(), static_cast<int>(title.size()));

        HPEN edgePen = CreatePen(PS_SOLID, 2, RGB(176, 195, 201));
        HGDIOBJ oldPen = SelectObject(dc, edgePen);
        for (const auto& edge : edges) {
            const auto& parent = nodes[edge.first];
            const auto& child = nodes[edge.second];
            MoveToEx(dc, parent.x, parent.y + nodeHeight, nullptr);
            LineTo(dc, child.x, child.y);
        }
        SelectObject(dc, oldPen);
        DeleteObject(edgePen);

        for (const auto& node : nodes) {
            RECT box{node.x - nodeWidth / 2, node.y, node.x + nodeWidth / 2,
                     node.y + nodeHeight};
            const bool atom = node.formula->kind() == FormulaKind::Atom;
            const bool rootNode = node.y == currentY;
            HBRUSH fill = CreateSolidBrush(
                rootNode ? RGB(246, 119, 108)
                         : (atom ? RGB(253, 236, 233) : RGB(255, 164, 154)));
            HPEN border = CreatePen(PS_SOLID, 1, RGB(40, 40, 40));
            HGDIOBJ oldBrush = SelectObject(dc, fill);
            HGDIOBJ oldNodePen = SelectObject(dc, border);
            RoundRect(dc, box.left, box.top, box.right, box.bottom, 12, 12);
            SelectObject(dc, oldNodePen);
            SelectObject(dc, oldBrush);
            DeleteObject(border);
            DeleteObject(fill);

            std::wstring text = toWide(labelFor(node.formula));
            SelectObject(dc, smallFont);
            SetTextColor(dc, rootNode ? RGB(255, 255, 255) : RGB(31, 31, 31));
            DrawTextW(dc, text.c_str(), -1, &box,
                      DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
        }
        currentY += std::max(190, 3 * levelGap + nodeHeight + 20);
    }

    EndPaint(window, &paint);
    return 0;
}

void createControls(HWND window) {
    CreateWindowW(L"STATIC", L"LOGICLAB", WS_VISIBLE | WS_CHILD,
                  42, 28, 180, 34, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"MOTOR DE INFERENCIA LÓGICO-PROPOSICIONAL",
                  WS_VISIBLE | WS_CHILD, 44, 65, 430, 24, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Escribe tus premisas en español y analiza su compatibilidad.",
                  WS_VISIBLE | WS_CHILD, 44, 103, 540, 24, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"PREMISAS Y REGLAS",
                  WS_VISIBLE | WS_CHILD, 44, 153, 220, 22, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"EDIT", L"Si una persona estudia, entonces aprende.\r\nUna persona estudia.",
                  WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
                      ES_AUTOHSCROLL | WS_HSCROLL |
                      WS_VSCROLL, 44, 180, 920, 155, window,
                  reinterpret_cast<HMENU>(InputId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"MODELO",
                  WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 44, 350, 180, 42, window,
                  reinterpret_cast<HMENU>(AnalyzeId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"LIMPIAR", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  975, 180, 105, 42, window, reinterpret_cast<HMENU>(ClearId),
                  nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"ARBOL SEMANTICO",
                  WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 232, 350, 180, 42, window,
                  reinterpret_cast<HMENU>(ShowDiagramId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"ATOMOS",
                  WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 420, 350, 140, 42, window,
                  reinterpret_cast<HMENU>(AtomsViewId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"REGLAS",
                  WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 568, 350, 140, 42, window,
                  reinterpret_cast<HMENU>(RulesViewId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"TABLEAU",
                  WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 718, 350, 105, 42, window,
                  reinterpret_cast<HMENU>(TableauViewId), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"RESULTADO",
                  WS_VISIBLE | WS_CHILD, 44, 420, 160, 22, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Listo para analizar",
                  WS_VISIBLE | WS_CHILD, 780, 420, 300, 22, window,
                  reinterpret_cast<HMENU>(StatusId), nullptr, nullptr);
    CreateWindowW(L"EDIT", L"Tu resultado aparecerá aquí.",
                  WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
                                ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL | ES_READONLY,
                            44, 447, 1020, 155, window,
                  reinterpret_cast<HMENU>(ResultId), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"DIAGRAMA SEMÁNTICO",
                  WS_CHILD, 44, 625, 260, 22, window,
                  reinterpret_cast<HMENU>(DiagramTitleId), nullptr, nullptr);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"LogicTreeDiagram", L"",
                    WS_CHILD, 44, 655, 700, 235, window,
                    reinterpret_cast<HMENU>(DiagramId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"REGRESAR", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                  650, 625, 94, 32, window,
                  reinterpret_cast<HMENU>(BackDiagramId), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"ÁTOMOS DETECTADOS",
                  WS_VISIBLE | WS_CHILD, 44, 910, 300, 22, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Sin análisis",
                  WS_VISIBLE | WS_CHILD, 44, 938, 1020, 35, window,
                  reinterpret_cast<HMENU>(AtomsId), nullptr, nullptr);
}

void paintDiagram(HDC dc, const RECT& area) {
    HBRUSH canvas = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &area, canvas);
    DeleteObject(canvas);
    SelectObject(dc, headingFont);
    SetTextColor(dc, RGB(40, 40, 40));
    SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, 36, 20, L"DIAGRAMA SEMÁNTICO", 18);

    std::function<std::string(const Formula::Ptr&)> logicLabel =
        [&](const Formula::Ptr& node) {
            if (node->kind() == FormulaKind::Atom) {
                const auto label = diagramLabels.find(node->atomName());
                return label == diagramLabels.end() ? node->atomName() : label->second;
            }
            if (node->kind() == FormulaKind::Not)
                return "¬(" + logicLabel(node->left()) + ")";
            const char* operation = node->kind() == FormulaKind::And ? " ∧ "
                                  : node->kind() == FormulaKind::Or ? " ∨ "
                                  : node->kind() == FormulaKind::Implies ? " → "
                                  : " ↔ ";
            return "(" + logicLabel(node->left()) + operation +
                   logicLabel(node->right()) + ")";
        };

    int top = 72;
    for (std::size_t ruleIndex = 0; ruleIndex < diagramFormulas.size(); ++ruleIndex) {
        const Formula::Ptr& root = diagramFormulas[ruleIndex];
        std::vector<Formula::Ptr> leaves;
        std::function<void(const Formula::Ptr&)> collect =
            [&](const Formula::Ptr& node) {
                if (node->kind() == FormulaKind::Atom) {
                    leaves.push_back(node);
                    return;
                }
                collect(node->left());
                if (node->right()) collect(node->right());
            };
        collect(root);
        const int height = std::max(1, static_cast<int>(area.bottom) - 120);
        const int gap = height /
                        static_cast<int>(std::max<std::size_t>(1, leaves.size()));
        std::map<const Formula*, POINT> positions;
        int leafIndex = 0;
        std::function<POINT(const Formula::Ptr&, int)> place =
            [&](const Formula::Ptr& node, int depth) -> POINT {
                if (node->kind() == FormulaKind::Atom) {
                    POINT point{70 + depth * 190, top + leafIndex++ * gap + gap / 2};
                    positions[node.get()] = point;
                    return point;
                }
                POINT left = place(node->left(), depth + 1);
                POINT right = node->right() ? place(node->right(), depth + 1) : left;
                POINT point{70 + depth * 190, (left.y + right.y) / 2};
                positions[node.get()] = point;
                return point;
            };
        place(root, 0);

        HPEN connector = CreatePen(PS_SOLID, 2, RGB(55, 55, 55));
        HGDIOBJ oldPen = SelectObject(dc, connector);
        std::function<void(const Formula::Ptr&)> lines =
            [&](const Formula::Ptr& node) {
                if (node->kind() == FormulaKind::Atom) return;
                POINT parent = positions[node.get()];
                POINT left = positions[node->left().get()];
                MoveToEx(dc, parent.x + 62, parent.y, nullptr);
                LineTo(dc, left.x - 62, left.y);
                lines(node->left());
                if (node->right()) {
                    POINT right = positions[node->right().get()];
                    MoveToEx(dc, parent.x + 62, parent.y, nullptr);
                    LineTo(dc, right.x - 62, right.y);
                    lines(node->right());
                }
            };
        lines(root);
        SelectObject(dc, oldPen);
        DeleteObject(connector);

        std::function<void(const Formula::Ptr&)> boxes =
            [&](const Formula::Ptr& node) {
                POINT center = positions[node.get()];
                RECT box{center.x - 62, center.y - 25, center.x + 62, center.y + 25};
                const bool atom = node->kind() == FormulaKind::Atom;
                HBRUSH fill = CreateSolidBrush(atom ? RGB(253, 236, 233)
                                                     : RGB(255, 164, 154));
                HPEN border = CreatePen(PS_SOLID, 1, RGB(35, 35, 35));
                HGDIOBJ oldBrush = SelectObject(dc, fill);
                HGDIOBJ oldNodePen = SelectObject(dc, border);
                RoundRect(dc, box.left, box.top, box.right, box.bottom, 10, 10);
                SelectObject(dc, oldNodePen);
                SelectObject(dc, oldBrush);
                DeleteObject(border);
                DeleteObject(fill);
                std::string label = logicLabel(node);
                std::wstring text = toWide(label);
                SelectObject(dc, smallFont);
                SetTextColor(dc, RGB(35, 35, 35));
                DrawTextW(dc, text.c_str(), -1, &box,
                          DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
                if (node->kind() != FormulaKind::Atom) {
                    boxes(node->left());
                    if (node->right()) boxes(node->right());
                }
            };
        boxes(root);
        top += std::max(270, static_cast<int>(leaves.size()) * 70);
    }
}

void paintTableau(HDC dc, const RECT& area) {
    HBRUSH canvas = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &area, canvas);
    DeleteObject(canvas);
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, headingFont);
    SetTextColor(dc, RGB(40, 40, 40));
    TextOutW(dc, 36, 20, L"TABLEAU SEMANTICO", 18);

    std::istringstream input(tableauResult);
    std::string line;
    int y = 72;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.find("====") != std::string::npos ||
            line.find("TABLEAU SEMANTICO") != std::string::npos ||
            line.find("RAMAS ABIERTAS") != std::string::npos ||
            line.find("Todas las ramas") != std::string::npos ||
            line.find("La base") != std::string::npos)
            continue;
        const bool closed = line.find("CERRADA") != std::string::npos;
        const bool open = line.find("ABIERTA") != std::string::npos;
        const bool applied = line.find("Aplicada") != std::string::npos;
        const int left = line.find("Rama ") != std::string::npos ? 82 : 48;
        RECT box{left, y, std::max(left + 360, static_cast<int>(area.right) - 48), y + 34};
        HBRUSH fill = CreateSolidBrush(closed ? RGB(255, 220, 220)
                                              : open ? RGB(220, 245, 230)
                                                     : applied ? RGB(239, 244, 248)
                                                               : RGB(248, 248, 248));
        HPEN border = CreatePen(PS_SOLID, 1, closed ? RGB(190, 70, 70)
                                                    : open ? RGB(70, 145, 90)
                                                           : RGB(130, 145, 155));
        HGDIOBJ oldBrush = SelectObject(dc, fill);
        HGDIOBJ oldPen = SelectObject(dc, border);
        RoundRect(dc, box.left, box.top, box.right, box.bottom, 8, 8);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(border);
        DeleteObject(fill);
        SelectObject(dc, treeFont);
        SetTextColor(dc, RGB(35, 45, 55));
        DrawTextW(dc, toWide(line).c_str(), -1, &box,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        y += 46;
        if (y > area.bottom - 40) break;
    }
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        createControls(window);
        return 0;
    case WM_SIZE:
        if (treeResultMode) {
            RECT clientArea{};
            GetClientRect(window, &clientArea);
            HWND resultControl = GetDlgItem(window, ResultId);
            MoveWindow(resultControl, 44, 447,
                       std::max(400, static_cast<int>(clientArea.right) - 88),
                       std::max(200, static_cast<int>(clientArea.bottom) - 490),
                       TRUE);
        }
        return 0;
    case WM_PAINT:
        if (diagramMode) {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            RECT area{};
            GetClientRect(window, &area);
            if (tableauCanvasMode) paintTableau(dc, area);
            else paintDiagram(dc, area);
            EndPaint(window, &paint);
            return 0;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == AnalyzeId) {
            analyze(window);
            if (!lastResult.empty()) showResultView(window, lastResult, false);
        }
        if (LOWORD(wParam) == AtomsViewId) {
            analyze(window);
            if (!atomsResult.empty()) showResultView(window, atomsResult, false);
        }
        if (LOWORD(wParam) == RulesViewId) {
            analyze(window);
            if (!rulesResult.empty()) showResultView(window, rulesResult, false);
        }
        if (LOWORD(wParam) == TableauViewId) {
            analyze(window, true);
            if (!tableauResult.empty()) {
                tableauCanvasMode = true;
                setDiagramView(window, true);
                InvalidateRect(window, nullptr, TRUE);
            }
        }
        if (LOWORD(wParam) == ShowDiagramId) {
            if (diagramFormulas.empty()) {
                showError(window, "Primero analiza las proposiciones.");
            } else {
                tableauCanvasMode = false;
                showResultView(window, semanticTreeResult, true);
                SetWindowTextW(GetDlgItem(window, StatusId),
                               L"Arbol semantico mostrado");
            }
        }
        if (LOWORD(wParam) == BackDiagramId) {
            tableauCanvasMode = false;
            setDiagramView(window, false);
            SetWindowTextW(GetDlgItem(window, StatusId), L"Resultado listo");
        }
        if (LOWORD(wParam) == ClearId) {
            SetWindowTextW(GetDlgItem(window, InputId), L"");
            SetWindowTextW(GetDlgItem(window, ResultId), L"Tu resultado aparecerá aquí.");
            lastResult.clear();
            semanticTreeResult.clear();
            tableauResult.clear();
            treeResultMode = false;
            SetWindowTextW(GetDlgItem(window, AnalyzeId), L"MODELO");
            SetWindowTextW(GetDlgItem(window, ShowDiagramId), L"ARBOL SEMANTICO");
            atomsResult.clear();
            rulesResult.clear();
            ShowWindow(GetDlgItem(window, ResultId), SW_SHOW);
            SetWindowTextW(GetDlgItem(window, AtomsId), L"Sin análisis");
            MoveWindow(GetDlgItem(window, ResultId), 44, 447, 1020, 155, TRUE);
            ShowWindow(GetDlgItem(window, AtomsId), SW_SHOW);
            SetWindowTextW(GetDlgItem(window, StatusId), L"Listo para analizar");
            diagramFormulas.clear();
            diagramOriginals.clear();
            diagramLabels.clear();
            setDiagramView(window, false);
            if (diagramWindow) {
                DestroyWindow(diagramWindow);
                diagramWindow = nullptr;
            }
        }
        return 0;
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wParam), RGB(77, 93, 104));
        SetBkColor(reinterpret_cast<HDC>(wParam), RGB(247, 250, 250));
        return reinterpret_cast<LRESULT>(backgroundBrush);
    case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(wParam), RGB(31, 48, 60));
        SetBkColor(reinterpret_cast<HDC>(wParam), RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    case WM_DRAWITEM: {
        const auto* draw = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        if (draw->CtlID != AnalyzeId && draw->CtlID != ShowDiagramId &&
            draw->CtlID != AtomsViewId && draw->CtlID != RulesViewId &&
            draw->CtlID != TableauViewId)
            break;
        HBRUSH buttonBrush = CreateSolidBrush(RGB(9, 27, 40));
        FillRect(draw->hDC, &draw->rcItem, buttonBrush);
        DeleteObject(buttonBrush);
        SetTextColor(draw->hDC, RGB(255, 255, 255));
        SetBkMode(draw->hDC, TRANSPARENT);
        RECT buttonRect = draw->rcItem;
        const wchar_t* label = draw->CtlID == AnalyzeId
                                   ? L"MODELO"
                                   : draw->CtlID == ShowDiagramId
                                         ? L"ARBOL SEMANTICO"
                                         : draw->CtlID == AtomsViewId ? L"ATOMOS"
                                                                       : draw->CtlID == RulesViewId
                                                                             ? L"REGLAS"
                                                                             : L"TABLEAU";
        DrawTextW(draw->hDC, label, -1, &buttonRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }
    case WM_DESTROY:
        DeleteObject(bodyFont);
        DeleteObject(headingFont);
        DeleteObject(smallFont);
        DeleteObject(treeFont);
        DeleteObject(backgroundBrush);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
}

int main() {
    INITCOMMONCONTROLSEX controls{sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    bodyFont = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                           DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    headingFont = CreateFontW(-26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    smallFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    treeFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Consolas");
    backgroundBrush = CreateSolidBrush(RGB(247, 250, 250));

    const wchar_t className[] = L"LogicLabWindow";
    const wchar_t diagramClassName[] = L"LogicTreeDiagram";
    WNDCLASSW diagramClass{};
    diagramClass.hInstance = GetModuleHandleW(nullptr);
    diagramClass.lpfnWndProc = diagramProcedure;
    diagramClass.lpszClassName = diagramClassName;
    diagramClass.style = CS_HREDRAW | CS_VREDRAW;
    diagramClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    diagramClass.hbrBackground =
        reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    RegisterClassW(&diagramClass);

    WNDCLASSW windowClass{};
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = backgroundBrush;
    RegisterClassW(&windowClass);

    HWND window = CreateWindowExW(
        0, className, L"LogicLab - Motor de Inferencia",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
            WS_MAXIMIZEBOX | WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 1140, 1000, nullptr, nullptr,
        windowClass.hInstance, nullptr);
    if (!window) return 1;
    SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(bodyFont), TRUE);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
