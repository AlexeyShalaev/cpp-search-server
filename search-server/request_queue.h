//
// -------- Очередь запросов ----------
//

#ifndef SEARCH_SERVER_REQUEST_QUEUE_H
#define SEARCH_SERVER_REQUEST_QUEUE_H

#include <deque>
#include <vector>
#include "search_server.h"


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer &search_server);

    // сделаем "обертки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template<typename DocumentPredicate>
    [[maybe_unused]] std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);

    [[maybe_unused]] std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string &raw_query);

    [[nodiscard]] int GetNoResultRequests() const;

private:
    struct QueryResult {
        uint64_t timestamp;
        int results;
    };
    std::deque<QueryResult> requests_;
    const SearchServer &search_server_;
    int no_results_requests_;
    uint64_t current_time_;
    const static int min_in_day_ = 1440;

    void AddRequest(int results_num);
};


template<typename DocumentPredicate>
[[maybe_unused]] std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate) {
    const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddRequest(result.size());
    return result;
}


#endif //SEARCH_SERVER_REQUEST_QUEUE_H