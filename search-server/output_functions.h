//
// -------- Перегруженные операторы ввода и вывода ----------
//

#ifndef SEARCH_SERVER_OUTPUT_FUNCTIONS_H
#define SEARCH_SERVER_OUTPUT_FUNCTIONS_H

#include <iostream>
#include <map>
#include <set>
#include <vector>

using namespace std::string_literals;

template<typename Key, typename Value>
std::ostream &operator<<(std::ostream &out, const std::pair<Key, Value> &pair) {
    out << '(' << pair.first << ", "s << pair.second << ')';
    return out;
}

template<typename Container>
void Print(std::ostream &out, const Container &container) {
    bool is_first = true;
    for (const auto &element: container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
}

template<typename Element>
std::ostream &operator<<(std::ostream &out, const std::vector<Element> &container) {
    out << '[';
    Print(out, container);
    out << ']';
    return out;
}

template<typename Element>
std::ostream &operator<<(std::ostream &out, const std::set<Element> &container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

template<typename Key, typename Value>
std::ostream &operator<<(std::ostream &out, const std::map<Key, Value> &container) {
    out << '{';
    bool is_first = true;
    for (const auto &element: container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element.first << ": "s << element.second;
    }
    out << '}';
    return out;
}

#endif //SEARCH_SERVER_OUTPUT_FUNCTIONS_H
