//
// -------- Поисковой сервер ----------
//

#ifndef SEARCH_SERVER_SEARCH_SERVER_H
#define SEARCH_SERVER_SEARCH_SERVER_H
#define TEST_MODE false

#include <algorithm>
#include <cmath>
#include <deque>
#include <execution>
#include <future>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <vector>
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const int BUCKETS_NUMBER = 100;
const double EPSILON = 1e-6;


class SearchServer {
public:

    // CONSTRUCTORS

    SearchServer();

    template<typename StringCollection>
    explicit SearchServer(const StringCollection &stop_words);

    explicit SearchServer(const std::string &stop_words);

    explicit SearchServer(std::string_view stop_words);

    // SET STOP WORDS

    void SetStopWords(std::string_view text);

    // ADD DOCUMENT

    void
    AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int> &ratings);

    // REMOVE DOCUMENT

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy &policy, int document_id);

    void RemoveDocument(const std::execution::parallel_policy &policy, int document_id);

    // GET DOCUMENTS

    auto GetDocuments() { return documents_; }

    [[nodiscard]] int GetDocumentCount() const;

    // FIND DOCUMENTS

    template<typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    template<typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy &policy, std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    [[nodiscard]] std::vector<Document>
    FindTopDocuments(std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    template<typename ExecutionPolicy>
    std::vector<Document>
    FindTopDocuments(const ExecutionPolicy &policy, std::string_view raw_query,
                     DocumentStatus status = DocumentStatus::ACTUAL) const;


    // MATCH DOCUMENTS

    using MatchDocumentResult = std::tuple<std::vector<std::string_view>, DocumentStatus>;

    [[nodiscard]] MatchDocumentResult MatchDocument(std::string_view raw_query, int document_id) const;

    [[nodiscard]] MatchDocumentResult
    MatchDocument(const std::execution::sequenced_policy &policy, std::string_view raw_query, int document_id) const;

    [[nodiscard]] MatchDocumentResult
    MatchDocument(const std::execution::parallel_policy &policy, std::string_view raw_query, int document_id) const;

    // TOOLS

    [[nodiscard]] const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;

    // ITERATORS

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

private:
    struct DocumentData {
        std::map<std::string_view, double> freqs;
        int rating;
        DocumentStatus status;
    };

    TransparentStringSet stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::deque<std::string> dictionary_;

    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    // PRIVATE METHODS

    static bool IsValidWord(std::string_view word);

    [[nodiscard]] bool IsStopWord(std::string_view word) const;

    [[nodiscard]] std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    // QUERY METHODS

    [[nodiscard]] QueryWord ParseQueryWord(std::string_view word) const;

    [[nodiscard]] Query ParseQuery(std::string_view text, bool skip_sort = false) const;

    // Existence required
    [[nodiscard]] double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    std::vector<Document>
    FindAllDocuments(const std::execution::sequenced_policy &, const Query &query,
                     DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    std::vector<Document>
    FindAllDocuments(const std::execution::parallel_policy &, const Query &query,
                     DocumentPredicate document_predicate) const;
};

template<typename StringCollection>
SearchServer::SearchServer(const StringCollection &stop_words): stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}


template<typename DocumentPredicate>
std::vector<Document>
SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template<typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy &policy, std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
#if TEST_MODE
    std::cout << "Результаты поиска по запросу: " << raw_query << std::endl;
    SEARCH_SERVER_DURATION;
#endif

    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                 return lhs.rating > rhs.rating;
             } else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template<typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindTopDocuments(const ExecutionPolicy &policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus doc_status,
                                                        int rating) {
        return doc_status == status;
    });
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy &policy, const Query &query,
                                                     DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;

    for (auto word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const auto inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto &[document_id, term_freq]: word_to_document_freqs_.at(word)) {
            const auto &document = documents_.at(document_id);
            if (document_predicate(document_id, document.status, document.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const auto &word: query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto &[document_id, _]: word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto &[document_id, relevance]: document_to_relevance) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }

    return matched_documents;
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy &policy, const Query &query,
                                                     DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(BUCKETS_NUMBER);

    std::for_each(policy,
                  query.plus_words.begin(), query.plus_words.end(),
                  [&](const auto &word) {
                      if (word_to_document_freqs_.count(word) > 0) {
                          const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                          for (const auto &[document_id, term_freq]: word_to_document_freqs_.at(word)) {
                              const auto &document = documents_.at(document_id);
                              if (document_predicate(document_id, document.status, document.rating)) {
                                  document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                              }
                          }
                      }
                  });

    std::for_each(policy,
                  query.minus_words.begin(), query.minus_words.end(),
                  [&](const auto &word) {
                      if (word_to_document_freqs_.count(word) > 0) {
                          for (const auto &[document_id, _]: word_to_document_freqs_.at(word)) {
                              document_to_relevance.erase(document_id);
                          }
                      }
                  });

    std::vector<Document> matched_documents;
    for (const auto &[document_id, relevance]: document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return matched_documents;
}

void AddDocument(SearchServer &search_server, int document_id, std::string_view document, DocumentStatus status,
                 const std::vector<int> &ratings);

std::vector<std::vector<Document>>
ProcessQueries(const SearchServer &search_server, const std::vector<std::string> &queries);

std::vector<Document>
ProcessQueriesJoined(const SearchServer &search_server, const std::vector<std::string> &queries);

#endif //SEARCH_SERVER_SEARCH_SERVER_H
