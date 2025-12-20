#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <cwctype>
#include <iostream>

// Конвертация UTF-8 -> wide string
std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(str);
}

// Конвертация wide string -> UTF-8
std::string wstring_to_utf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::wstring current;
    std::wstring wtext = utf8_to_wstring(text);

    std::locale::global(std::locale("en_US.UTF-8"));  // UTF-8 локаль

    for (wchar_t wc : wtext) {
        if (std::iswalnum(wc)) {
            current += std::towlower(wc);
        } else {
            if (!current.empty()) {       // токен любой длины
                tokens.push_back(wstring_to_utf8(current));
            }
            current.clear();
        }
    }

    if (!current.empty()) {
        tokens.push_back(wstring_to_utf8(current));
    }

    return tokens;
}
