//
// -------- Итераторы ----------
//

#ifndef SEARCH_SERVER_ITERATOR_H
#define SEARCH_SERVER_ITERATOR_H

#include <iostream>

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) : first_(begin), last_(end), size_(distance(first_, last_)) {}

    Iterator begin() const { return first_; }

    Iterator end() const { return last_; }

    [[nodiscard]] size_t size() const { return size_; }

private:
    Iterator first_, last_;
    size_t size_;
};

template<typename Iterator>
std::ostream &operator<<(std::ostream &out, const IteratorRange<Iterator> &range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

#endif //SEARCH_SERVER_ITERATOR_H
