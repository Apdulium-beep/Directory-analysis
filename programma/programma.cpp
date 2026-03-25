#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

// Расширенная карта типов файлов (по расширениям)
const std::map<std::string, std::string> FILE_TYPES = {
    {".doc", "Word 97-2003"},
    {".docx", "Word Document"},
    {".xls", "Excel 97-2003"},
    {".xlsx", "Excel Spreadsheet"},
    {".ppt", "PowerPoint 97-2003"},
    {".pptx", "PowerPoint"},
    {".pdf", "PDF Document"},
    {".txt", "Text File"},
    {".rtf", "Rich Text"},
    {".iso", "ISO Image"},
    {".img", "Disk Image"},
    {".vhd", "Virtual Disk"},
    {".zip", "ZIP Archive"},
    {".rar", "RAR Archive"},
    {".7z", "7-Zip"},
    {".tar", "TAR Archive"},
    {".gz", "GZip"},
    {".bz2", "BZip2"},
    {".jpg", "JPEG Image"},
    {".jpeg", "JPEG Image"},
    {".png", "PNG Image"},
    {".gif", "GIF Image"},
    {".bmp", "Bitmap"},
    {".tiff", "TIFF Image"},
    {".webp", "WebP Image"},
    {".svg", "SVG Vector"},
    {".mp4", "MP4 Video"},
    {".avi", "AVI Video"},
    {".mkv", "MKV Video"},
    {".mp3", "MP3 Audio"},
    {".wav", "WAV Audio"},
    {".flac", "FLAC Audio"},
    {".exe", "Executable"},
    {".dll", "Dynamic Library"},
    {".py", "Python Script"},
    {".js", "JavaScript"},
    {".html", "HTML"},
    {".css", "CSS"}
};

std::string get_file_type(const std::string& filename) {
    fs::path p(filename);
    std::string ext = p.has_extension() ? p.extension().string() : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto it = FILE_TYPES.find(ext);
    if (it != FILE_TYPES.end()) {
        return it->second;
    }

    if (!ext.empty()) {
        ext.erase(0, 1); // убираем точку
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
    }
    return "Файл (" + ext + ")";
}

// Форматируем размер с разделителями тысяч
std::string format_size(uintmax_t size) {
    std::string s = std::to_string(size);
    int pos = s.length();
    while (pos > 3) {
        pos -= 3;
        s.insert(pos, ",");
    }
    return s;
}

int main() {
    setlocale(LC_ALL, "Russian");

    std::string start_path;

    // Запрашиваем папку или используем текущую
    std::cout << "Введите путь к папке (Enter = текущая папка): ";
    std::getline(std::cin, start_path);

    if (start_path.empty()) {
        start_path = ".";
    }

    fs::path root(start_path);

    if (!fs::exists(root)) {
        std::cerr << "Ошибка: путь не существует: " << root << std::endl;
        return 1;
    }

    if (!fs::is_directory(root)) {
        std::cerr << "Ошибка: путь не является папкой: " << root << std::endl;
        return 1;
    }

    // Карта: размер -> список пар (имя файла, тип)
    std::map<uintmax_t, std::vector<std::pair<std::string, std::string>>> files_by_size;

    for (const auto& entry : fs::directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;

        fs::path filepath = entry.path();
        std::string filename = filepath.filename().string();

        uintmax_t size = 0;
        try {
            size = fs::file_size(filepath);
        }
        catch (...) {
            continue;
        }

        std::string file_type = get_file_type(filename);
        files_by_size[size].emplace_back(filename, file_type);
    }

    // Вывод таблицы в консоль
    std::cout << "\nВсе файлы в папке (отсортировано по размеру):\n";
    std::cout << std::string(100, '-') << "\n";

    // Заголовки
    std::cout << std::setw(45) << "Название"
        << " | "
        << std::setw(12) << "Размер"
        << " | "
        << std::setw(25) << "Тип"
        << " | "
        << std::setw(8) << "Дубликаты"
        << "\n";
    std::cout << std::string(100, '-') << "\n";

    for (const auto& [size, list] : files_by_size) {
        int count = static_cast<int>(list.size());
        std::string size_str = format_size(size);
        for (const auto& [filename, file_type] : list) {
            std::cout << std::setw(45) << filename
                << " | "
                << std::setw(12) << size_str
                << " | "
                << std::setw(25) << file_type
                << " | [" << count << "]\n";
        }
    }

    std::cout << "\nНажмите Enter для завершения...";
    std::cin.ignore();

    return 0;
}
