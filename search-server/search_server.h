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
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <vector>
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
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

    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
                     const std::vector<int> &ratings);

    // REMOVE DOCUMENT

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy &policy, int document_id);

    void RemoveDocument(const std::execution::parallel_policy &policy, int document_id);

    // GET DOCUMENTS

    auto GetDocuments() {
        return documents_;
    }

    [[nodiscard]] int GetDocumentCount() const;

    // FIND DOCUMENTS

    [[nodiscard]] std::vector<Document>
    FindTopDocuments(std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    template<typename KeyMapper>
    [[nodiscard]] std::vector<Document>
    FindTopDocuments(std::string_view raw_query, KeyMapper key_mapper) const;

    // MATCH DOCUMENTS

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::string_view raw_query, int document_id) const;

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(const std::execution::sequenced_policy &policy, std::string_view raw_query,
                  int document_id) const;

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(const std::execution::parallel_policy &policy, std::string_view raw_query,
                  int document_id) const;

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

    std::set<std::string, std::less<>> stop_words_;
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
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    struct QueryVector {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    // PRIVATE METHODS

    static bool IsValidWord(std::string_view word);

    [[nodiscard]] bool IsStopWord(std::string_view word) const;

    [[nodiscard]] std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    // QUERY METHODS

    [[nodiscard]] QueryWord ParseQueryWord(std::string_view word) const;

    [[nodiscard]] Query ParseQuery(std::string_view text) const;

    [[nodiscard]] QueryVector ParseQueryVector(std::string_view text) const;

    // Existence required
    [[nodiscard]] double ComputeWordInverseDocumentFreq(std::string_view word) const {
        return log(GetDocumentCount() / static_cast<double>(word_to_document_freqs_.at(word).size()));
    }

    template<typename KeyMapper>
    std::vector<Document> FindAllDocuments(const Query &query, KeyMapper key_mapper) const;
};

template<typename StringCollection>
SearchServer::SearchServer(const StringCollection &stop_words): stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Stop words should not contain special characters.");
    }
}

template<typename KeyMapper>
[[nodiscard]] std::vector<Document>
SearchServer::FindTopDocuments(std::string_view raw_query, KeyMapper key_mapper) const {
#if TEST_MODE
    std::cout << "Результаты поиска по запросу: " << raw_query << std::endl;
    SEARCH_SERVER_DURATION;
#endif


    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, key_mapper);

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

template<typename KeyMapper>
std::vector<Document> SearchServer::FindAllDocuments(const Query &query, KeyMapper key_mapper) const {
    std::map<int, double> document_to_relevance;

    for (auto word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
            const auto document = documents_.at(document_id);
            if (key_mapper(document_id, document.status, document.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (auto word: query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance]: document_to_relevance) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return matched_documents;
}

void AddDocument(SearchServer &search_server, int document_id, std::string_view document, DocumentStatus status,
                 const std::vector<int> &ratings);

std::vector<std::vector<Document>>
ProcessQueries(const SearchServer &search_server, const std::vector<std::string> &queries);

std::vector<Document> ProcessQueriesJoined(const SearchServer &search_server, const std::vector<std::string> &queries);

#endif //SEARCH_SERVER_SEARCH_SERVER_H
