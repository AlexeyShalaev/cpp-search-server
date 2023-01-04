//
// -------- Пагинация ----------
//

#ifndef SEARCH_SERVER_PAGINATOR_H
#define SEARCH_SERVER_PAGINATOR_H

#include <algorithm>
#include "iterator.h"


template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = std::next(begin, current_page_size);
            pages_.push_back({begin, current_page_end});

            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    [[nodiscard]] size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template<typename Container>
auto Paginate(const Container &c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

#endif //SEARCH_SERVER_PAGINATOR_H
