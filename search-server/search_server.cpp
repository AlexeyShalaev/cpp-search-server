//
// Created by Alex Shalaev on 04.01.2023.
//

#include "search_server.h"
#include "string_processing.h"

using namespace std;

static int ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::SearchServer() = default;

void SearchServer::SetStopWords(const string &text) {
    for (const string &word: SplitIntoWords(text)) {
        if (!IsValidWord(word)) throw invalid_argument("Stop words should not contain special characters.");
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const string &document, DocumentStatus status,
                               const vector<int> &ratings) {
    if (document_id < 0) throw invalid_argument("Document ID must be a natural number.");
    if (documents_.count(document_id) > 0)throw invalid_argument("Document with this ID already exists.");
    ids.push_back(document_id);
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1 / static_cast<double>(words.size());
    for (const string &word: words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}


vector<Document>
SearchServer::FindTopDocuments(const string &raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status,
                                                int rating) {
        return doc_status == status;
    });
}

[[nodiscard]] int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

[[nodiscard]] int SearchServer::GetDocumentId(int index) const {
    if (index < 0 || index >= ids.size())
        throw out_of_range("Index is out of range.");
    return ids[index];
}

[[nodiscard]] tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string &raw_query,
                                                                                int document_id) const {
    const Query query = ParseQuery(raw_query);

    vector<string> matched_words;
    for (const string &word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string &word: query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsValidWord(const string &word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

[[nodiscard]] bool SearchServer::IsStopWord(const string &word) const {
    return stop_words_.count(word) > 0;
}

[[nodiscard]] vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const {
    vector<string> words;
    for (const string &word: SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Document's content should not contains special characters.");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


[[nodiscard]] SearchServer::QueryWord SearchServer::ParseQueryWord(string word) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || !IsValidWord(word)) {
        throw invalid_argument("Query has incorrect symbols in " + word + ".");
    }
    if (is_minus && word[0] == '-') {
        throw invalid_argument("Query has incorrect minus-words:" + word + ".");
    }
    return {word, is_minus, IsStopWord(word)};
}


[[nodiscard]] SearchServer::Query SearchServer::ParseQuery(const string &text) const {
    Query query;
    for (const string &word: SplitIntoWords(text)) {
        const SearchServer::QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}


