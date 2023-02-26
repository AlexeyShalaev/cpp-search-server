//
// -------- Обработка строк ----------
//

#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(str.substr(0, space));
        if (str.find(' ') != str.npos) {
            str.remove_prefix(space);
            str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
        } else {
            str.remove_prefix(str.size());
        }
    }
    return result;
}