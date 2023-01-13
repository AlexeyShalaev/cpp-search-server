//
// -------- Поисковой сервер ----------
//

#ifndef SEARCH_SERVER_SEARCH_SERVER_H
#define SEARCH_SERVER_SEARCH_SERVER_H
#define TEST false

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <vector>
#include "document.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;


class SearchServer {
public:

    SearchServer();

    template<typename StringCollectionOrType>
    explicit SearchServer(const StringCollectionOrType &stop_words);

    template<template<class E> typename Container>
    void SetStopWords(const Container<std::string> &collection);

    void SetStopWords(const std::string &text);

    void
    AddDocument(int document_id, const std::string &document, DocumentStatus status, const std::vector<int> &ratings);

    void RemoveDocument(int document_id);

    auto GetDocuments() {
        return documents_;
    }

    [[nodiscard]] std::vector<Document>
    FindTopDocuments(const std::string &raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    template<typename KeyMapper>
    [[nodiscard]] std::vector<Document>
    FindTopDocuments(const std::string &raw_query, KeyMapper key_mapper) const;

    [[nodiscard]] int GetDocumentCount() const;

    //[[nodiscard]] int GetDocumentId(int index) const;

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

    [[nodiscard]] std::tuple<std::vector<std::string>, DocumentStatus>
    MatchDocument(const std::string &raw_query, int document_id) const;

    [[nodiscard]] const std::map<std::string, double> &GetWordFrequencies(int document_id) const;

private:
    struct DocumentData {
        std::set<std::string> content;
        std::map<std::string, double> freqs;
        int rating;
        DocumentStatus status;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> ids;

    static bool IsValidWord(const std::string &word);

    [[nodiscard]] bool IsStopWord(const std::string &word) const;

    [[nodiscard]] std::vector<std::string> SplitIntoWordsNoStop(const std::string &text) const;

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    [[nodiscard]] QueryWord ParseQueryWord(std::string word) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    [[nodiscard]] Query ParseQuery(const std::string &text) const;

    // Existence required
    [[nodiscard]] double ComputeWordInverseDocumentFreq(const std::string &word) const {
        return log(GetDocumentCount() / static_cast<double>(word_to_document_freqs_.at(word).size()));
    }

    template<typename KeyMapper>
    std::vector<Document> FindAllDocuments(const Query &query, KeyMapper key_mapper) const;
};


template<typename StringCollectionOrType>
SearchServer::SearchServer(const StringCollectionOrType &stop_words) {
    SetStopWords(stop_words);
}

template<template<class E> typename Container>
void SearchServer::SetStopWords(const Container<std::string> &collection) {
    for (const auto &word: collection) {
        if (!IsValidWord(word)) throw std::invalid_argument("Stop words should not contain special characters.");
        stop_words_.insert(word);
    }
}

template<typename KeyMapper>
[[nodiscard]] std::vector<Document>
SearchServer::FindTopDocuments(const std::string &raw_query, KeyMapper key_mapper) const {
#if TEST
    std::cout << "Результаты поиска по запросу: " << raw_query << std::endl;
    SEARCH_SERVER_DURATION;
#endif


    const Query query = ParseQuery(raw_query);

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
    for (const auto &word: query.plus_words) {
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

    for (const auto &word: query.minus_words) {
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

void AddDocument(SearchServer &search_server, int document_id, const std::string &document, DocumentStatus status,
                 const std::vector<int> &ratings);

#endif //SEARCH_SERVER_SEARCH_SERVER_H
