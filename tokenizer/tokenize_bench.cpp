#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <cctype>
#include <numeric>
#include "Tokenizer.cpp"


// Генерация случайного текста длиной size_kb килобайт
std::string generate_text(size_t size_kb) {
    static const char alphanum[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, sizeof(alphanum) - 2);

    std::string text;
    text.reserve(size_kb * 1024);

    for (size_t i = 0; i < size_kb * 1024; ++i) {
        char c = alphanum[dist(rng)];
        text += c;
        if (i % 7 == 0) text += ' ';  // добавляем пробелы
    }
    return text;
}

int main(int argc, char* argv[]) {
    size_t size_kb = 1024; // по умолчанию 1 МБ
    if (argc > 1) size_kb = std::stoul(argv[1]);

    std::cout << "Генерация текста размером " << size_kb << " КБ..." << std::endl;
    std::string text = generate_text(size_kb);

    std::cout << "Начало токенизации..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    auto tokens = tokenize(text);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;

    // Статистика
    size_t total_length = 0;
    for (const auto& t : tokens) total_length += t.size();
    double avg_length = tokens.empty() ? 0.0 : double(total_length) / tokens.size();
    double speed_kb_per_sec = size_kb / elapsed.count();

    std::cout << "Количество токенов: " << tokens.size() << "\n";
    std::cout << "Средняя длина токена: " << avg_length << "\n";
    std::cout << "Время токенизации: " << elapsed.count() << " сек\n";
    std::cout << "Скорость токенизации: " << speed_kb_per_sec << " КБ/с\n";

    return 0;
}
