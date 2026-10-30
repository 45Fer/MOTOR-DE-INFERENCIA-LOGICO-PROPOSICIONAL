#include "LogicEngine.hpp"
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr int InputId = 1001;
constexpr int AnalyzeId = 1002;
constexpr int ClearId = 1003;
constexpr int ResultId = 1004;
constexpr int StatusId = 1005;
constexpr int AtomsId = 1006;

const char* const learnedPhrasesPath = "data/frases_aprendidas.txt";
HFONT bodyFont;
HFONT headingFont;
HFONT smallFont;
HBRUSH backgroundBrush;

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

void analyze(HWND window) {
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

        appendLine(result, "");
        appendLine(result, "ÁRBOL SEMÁNTICO");
        for (const auto& rule : engine.rules()) {
            appendLine(result, "Frase: " + rule.original);
            appendLine(result, SemanticTree(rule.formula).render(report.atomLabels));
        }

        setText(GetDlgItem(window, ResultId), result);
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
                      WS_VSCROLL, 44, 180, 565, 155, window,
                  reinterpret_cast<HMENU>(InputId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"ANALIZAR PROPOSICIONES",
                  WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 44, 350, 245, 42, window,
                  reinterpret_cast<HMENU>(AnalyzeId), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"LIMPIAR", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  305, 350, 105, 42, window, reinterpret_cast<HMENU>(ClearId),
                  nullptr, nullptr);
    CreateWindowW(L"STATIC", L"RESULTADO",
                  WS_VISIBLE | WS_CHILD, 44, 420, 160, 22, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Listo para analizar",
                  WS_VISIBLE | WS_CHILD, 440, 420, 250, 22, window,
                  reinterpret_cast<HMENU>(StatusId), nullptr, nullptr);
    CreateWindowW(L"EDIT", L"Tu resultado aparecerá aquí.",
                  WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
                      WS_VSCROLL | ES_READONLY, 44, 447, 700, 300, window,
                  reinterpret_cast<HMENU>(ResultId), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"ÁTOMOS DETECTADOS",
                  WS_VISIBLE | WS_CHILD, 44, 770, 180, 22, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Sin análisis",
                  WS_VISIBLE | WS_CHILD, 44, 798, 700, 35, window,
                  reinterpret_cast<HMENU>(AtomsId), nullptr, nullptr);
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        createControls(window);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == AnalyzeId) analyze(window);
        if (LOWORD(wParam) == ClearId) {
            SetWindowTextW(GetDlgItem(window, InputId), L"");
            SetWindowTextW(GetDlgItem(window, ResultId), L"Tu resultado aparecerá aquí.");
            SetWindowTextW(GetDlgItem(window, AtomsId), L"Sin análisis");
            SetWindowTextW(GetDlgItem(window, StatusId), L"Listo para analizar");
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
        if (draw->CtlID != AnalyzeId) break;
        FillRect(draw->hDC, &draw->rcItem, CreateSolidBrush(RGB(9, 27, 40)));
        SetTextColor(draw->hDC, RGB(255, 255, 255));
        SetBkMode(draw->hDC, TRANSPARENT);
        RECT buttonRect = draw->rcItem;
        DrawTextW(draw->hDC, L"ANALIZAR PROPOSICIONES", -1, &buttonRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }
    case WM_DESTROY:
        DeleteObject(bodyFont);
        DeleteObject(headingFont);
        DeleteObject(smallFont);
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
    backgroundBrush = CreateSolidBrush(RGB(247, 250, 250));

    const wchar_t className[] = L"LogicLabWindow";
    WNDCLASSW windowClass{};
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = backgroundBrush;
    RegisterClassW(&windowClass);

    HWND window = CreateWindowExW(
        0, className, L"LogicLab - Motor de Inferencia",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 810, 900, nullptr, nullptr,
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
