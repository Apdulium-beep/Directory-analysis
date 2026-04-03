#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <string>
#include <algorithm>
#include <chrono>
#include <cstdio>

namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
#endif

class DirectoryAnalyzer {
private:
    struct FileInfo {
        std::string type;
        std::string name;
        std::string desc;
        uintmax_t size;
        std::string hierarchy;
        std::string extension;
    };

    std::vector<FileInfo> table_data;
    std::unordered_map<uintmax_t, std::vector<fs::path>> duplicates;

    std::string get_size_string(uintmax_t size_bytes) {
        const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
        double size = static_cast<double>(size_bytes);
        int unit_idx = 0;

        while (size >= 1024.0 && unit_idx < 5) {
            size /= 1024.0;
            unit_idx++;
        }

        char buffer[32];
#ifdef _MSC_VER
        sprintf_s(buffer, sizeof(buffer), "%.1f %s", size, units[unit_idx]);
#else
        sprintf(buffer, "%.1f %s", size, units[unit_idx]);
#endif
        return std::string(buffer);
    }

    std::string get_file_extension(const fs::path& path) {
        if (fs::is_directory(path)) {
            return "[DIR] папка";
        }

        std::string ext = path.extension().string();
        if (ext.empty()) {
            return "[NOEXT] без расширения";
        }

        if (!ext.empty() && ext[0] == '.') {
            ext.erase(0, 1);
        }
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext;
    }

    std::string get_hierarchy_symbol(int level) {
        if (level == 0) return "";

        std::string symbols;
        for (int i = 0; i < level - 1; ++i) {
            symbols += "|   ";
        }
        symbols += "+-- ";
        return symbols;
    }

public:
    void analyze_directory(const fs::path& root_path, bool show_duplicates = true) {
        std::cout << "Директория сканируется: " << root_path << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();

        table_data.clear();
        duplicates.clear();

        recursive_analyze(root_path, 0);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        print_results(duration.count());

        if (show_duplicates) {
            show_duplicates_info();
        }
    }

private:
    void recursive_analyze(const fs::path& path_obj, int level) {
        std::string prefix = get_hierarchy_symbol(level);

        try {
            if (!fs::exists(path_obj)) {
                table_data.push_back({
                    prefix + "[ERR]",
                    path_obj.string(),
                    "NOT FOUND",
                    0,
                    prefix + "[ERR]",
                    "[NOEXT] без расширения"
                    });
                return;
            }

            if (fs::is_regular_file(path_obj)) {
                auto size = fs::file_size(path_obj);
                auto extension = get_file_extension(path_obj);

                duplicates[size].push_back(path_obj);

                table_data.push_back({
                    prefix + "[FILE]",
                    path_obj.filename().string(),
                    "Фаил",
                    size,
                    prefix + "[FILE]",
                    extension
                    });
                return;
            }

            if (fs::is_directory(path_obj)) {
                try {
                    uintmax_t total_size = 0;
                    int item_count = 0;

                    for (const auto& entry : fs::recursive_directory_iterator(
                        path_obj, fs::directory_options::skip_permission_denied)) {
                        if (fs::is_regular_file(entry)) {
                            total_size += fs::file_size(entry);
                        }
                    }

                    for (const auto& entry : fs::directory_iterator(path_obj)) {
                        item_count++;
                    }

                    auto extension = get_file_extension(path_obj);

                    table_data.push_back({
                        prefix + "[DIR]",
                        path_obj.filename().string(),
                        "Папка (" + std::to_string(item_count) + " items)",
                        total_size,
                        prefix + "[DIR]",
                        extension
                        });

                    std::vector<fs::path> items;
                    for (const auto& entry : fs::directory_iterator(path_obj)) {
                        items.push_back(entry.path());
                    }

                    std::sort(items.begin(), items.end(),
                        [](const fs::path& a, const fs::path& b) {
                            bool a_file = fs::is_regular_file(a);
                            bool b_file = fs::is_regular_file(b);
                            if (a_file != b_file) return !a_file;
                            return a.filename().string() < b.filename().string();
                        });

                    for (const auto& item : items) {
                        recursive_analyze(item, level + 1);
                    }

                }
                catch (const fs::filesystem_error& e) {
                    table_data.push_back({
                        prefix + "[ERR]",
                        path_obj.filename().string(),
                        "ACCESS DENIED",
                        0,
                        prefix + "[ERR]",
                        "[ERR]"
                        });
                }
            }

        }
        catch (const std::exception& e) {
            table_data.push_back({
                prefix + "[ERR]",
                path_obj.filename().string(),
                "ACCESS ERROR",
                0,
                prefix + "[ERR]",
                "[ERR]"
                });
        }
    }

    void print_results(int duration_ms) {
        std::cout << "\n" << std::string(100, '=') << std::endl;
        std::cout << "Результат анализа" << std::endl;
        std::cout << std::string(100, '=') << std::endl;

        std::cout << std::left << std::setw(10) << "Тип"
            << std::setw(45) << "Имя"
            << std::setw(25) << "Путь"
            << std::setw(15) << "Размер"
            << "Расщирение" << std::endl;
        std::cout << std::string(100, '-') << std::endl;

        uintmax_t total_size = 0;
        int file_count = 0, dir_count = 0, error_count = 0;

        for (const auto& item : table_data) {
            std::cout << std::left << std::setw(10) << item.type
                << std::setw(45) << item.name
                << std::setw(25) << item.desc
                << std::setw(15) << get_size_string(item.size)
                << item.extension << std::endl;

            total_size += item.size;
            if (item.type.find("[FILE]") != std::string::npos) file_count++;
            else if (item.type.find("[DIR]") != std::string::npos) dir_count++;
            else error_count++;
        }

        std::cout << std::string(100, '-') << std::endl;
        std::cout << "Статистика: " << file_count << " Файлов, "
            << dir_count << " Папок, " << error_count << " Ошибок" << std::endl;
        std::cout << "Итоговый размер: " << get_size_string(total_size) << std::endl;
        std::cout << "Время: " << duration_ms << " ms" << std::endl;
        std::cout << std::string(100, '=') << std::endl;
    }

    void show_duplicates_info() {
        int dup_groups = 0;
        int total_dup_files = 0;

        for (const auto& [size, files] : duplicates) {
            if (files.size() > 1) {
                dup_groups++;
                total_dup_files += files.size();
            }
        }

        if (dup_groups == 0) {
            std::cout << "\nДубликатов по размеру не найдено!" << std::endl;
            return;
        }

        std::cout << "\nДубликаты по размеру (" << dup_groups
            << " Группы, " << total_dup_files << " Файлы)" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        std::vector<std::pair<uintmax_t, std::vector<fs::path>>> dup_list;
        for (const auto& [size, files] : duplicates) {
            if (files.size() > 1) {
                dup_list.emplace_back(size, files);
            }
        }

        std::sort(dup_list.rbegin(), dup_list.rend());

        for (const auto& [size, files] : dup_list) {
            std::cout << "\nРазмер: " << get_size_string(size)
                << " (" << files.size() << " Копии)" << std::endl;

            for (size_t i = 0; i < files.size(); ++i) {
                std::cout << "   " << (i + 1) << ". " << files[i] << std::endl;
            }
        }
        std::cout << std::string(80, '-') << std::endl;
    }
};

int main(int argc, char* argv[]) {
#ifdef _WIN32

    setlocale(LC_ALL, "Russian");
    SetConsoleCP(65001);
#endif

    std::cout << "Directory Analyzer v2.7 (C++ Console)" << std::endl;
    std::cout << "======================================" << std::endl;

    fs::path target_path;

    if (argc > 1) {
        target_path = argv[1];
    }
    else {
        std::cout << "Введите директорию (Enter = текущая): ";
        std::string input;
        std::getline(std::cin, input);
        target_path = input.empty() ? fs::current_path() : input;
    }

    if (!fs::exists(target_path)) {
        std::cerr << "Ошибка: Директория не существует: " << target_path << std::endl;
        return 1;
    }

    DirectoryAnalyzer analyzer;
    analyzer.analyze_directory(target_path);

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}