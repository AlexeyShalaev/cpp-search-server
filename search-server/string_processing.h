//
// -------- Обработка строк ----------
//

#ifndef SEARCH_SERVER_STRING_PROCESSING_H
#define SEARCH_SERVER_STRING_PROCESSING_H


#include <string>
#include <vector>
#include <set>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

template<typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer &strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (auto &str: strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::string(str));
        }
    }
    return non_empty_strings;
}

#endif //SEARCH_SERVER_STRING_PROCESSING_H
