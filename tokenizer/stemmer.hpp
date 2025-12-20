#pragma once
#include <string>

#include <string>
#include <vector>

// Простейшая проверка окончания для UTF-8
inline bool utf8_ends_with(const std::string &str, const std::string &suffix) {
    if (str.size() < suffix.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Стемминг для английского и русского (упрощённый)
inline std::string stem(const std::string& w) {
    std::string s = w;

    // Список простых окончаний (англ + рус)
    const std::vector<std::string> suffixes = {
        "ing", "ed", "es", "s",       // английский
        "ами", "ями", "ов", "ев", "ах", "ях", "ою", "ей", "ие", "ое", "ая", "ые", "ых", "ая", "ого", "ему" // русский
    };

    // Обрезаем первое совпадение
    for (const auto &suffix : suffixes) {
        if (utf8_ends_with(s, suffix) && s.size() > suffix.size()) {
            s.resize(s.size() - suffix.size());
            break;
        }
    }

    return s;
}
