//
// -------- Поисковой сервер ----------
//

#include "search_server.h"

using namespace std;

// STATIC METHODS

static int ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

// CONSTRUCTORS

SearchServer::SearchServer() = default;

SearchServer::SearchServer(const std::string &stop_words) : SearchServer(string_view(stop_words)) {}

SearchServer::SearchServer(std::string_view stop_words) : SearchServer(SplitIntoWords(stop_words)) {}

// SET STOP WORDS

void SearchServer::SetStopWords(string_view stop_words) {
    stop_words_ = MakeUniqueNonEmptyStrings(SplitIntoWords(stop_words));
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Stop words should not contain special characters.");
    }
}

// ADD DOCUMENT

void
SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int> &ratings) {
    if (document_id < 0) throw invalid_argument("Document ID must be a natural number.");
    if (documents_.count(document_id) > 0) throw invalid_argument("Document with this ID already exists.");

    document_ids_.insert(document_id);
    documents_.emplace(document_id, DocumentData());

    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1 / static_cast<double>(words.size());

    for (auto word: words) {
        const auto it = std::find(dictionary_.begin(), dictionary_.end(), std::string(word));
        if (it == dictionary_.end()) {
            dictionary_.emplace_back(std::string(word));
            word_to_document_freqs_[dictionary_.back()][document_id] += inv_word_count;
            documents_[document_id].freqs[dictionary_.back()] += inv_word_count;
        } else {
            word_to_document_freqs_[*it][document_id] += inv_word_count;
            documents_[document_id].freqs[*it] += inv_word_count;
        }
    }

    documents_[document_id].rating = ComputeAverageRating(ratings);
    documents_[document_id].status = status;
}

void AddDocument(SearchServer &search_server, int document_id, string_view document, DocumentStatus status,
                 const vector<int> &ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}

// REMOVE DOCUMENT

void SearchServer::RemoveDocument(int document_id) {
    if (document_ids_.count(document_id) == 0) return;
    for (auto &[key, val]: word_to_document_freqs_) {
        val.erase(document_id);
    }
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &policy, int document_id) {
    if (document_ids_.count(document_id) == 0) return;
    const auto freqs = documents_.at(document_id).freqs;
    std::vector<string_view> words(freqs.size());
    std::transform(policy,
                   freqs.begin(), freqs.end(),
                   words.begin(),
                   [](const auto &word) {
                       return word.first;
                   }
    );
    std::for_each(policy,
                  words.begin(), words.end(),
                  [&, document_id](const auto &word) {
                      word_to_document_freqs_[word].erase(document_id);
                  }
    );
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

// GET DOCUMENTS

[[nodiscard]] int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

// FIND DOCUMENTS

vector<Document>
SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status,
                                                int rating) {
        return doc_status == status;
    });
}

// MATCH DOCUMENTS

[[nodiscard]] tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query,
                                                                                     int document_id) const {
#if TEST_MODE
    cout << "Матчинг документов по запросу: "s << raw_query << std::endl;
    SEARCH_SERVER_DURATION;
#endif

    if (documents_.count(document_id) == 0) throw out_of_range("A nonexistent document_id was passed.");

    const auto query = ParseQuery(raw_query);
    vector<string_view> matched_words;

    for (auto word: query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return {matched_words, documents_.at(document_id).status};
        }
    }

    for (auto word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

[[nodiscard]] tuple<vector<string_view>, DocumentStatus>
SearchServer::MatchDocument(const execution::sequenced_policy &policy, string_view raw_query,
                            int document_id) const {
    return MatchDocument(raw_query, document_id);
}

[[nodiscard]] tuple<vector<string_view>, DocumentStatus>
SearchServer::MatchDocument(const execution::parallel_policy &policy, string_view raw_query,
                            int document_id) const {
#if TEST_MODE
    cout << "Матчинг документов по запросу: "s << raw_query << std::endl;
    SEARCH_SERVER_DURATION;
#endif

    const auto &query = ParseQueryVector(raw_query);
    vector<string_view> matched_words;

    if (!std::any_of(policy,
                     query.minus_words.begin(), query.minus_words.end(),
                     [&](const auto &word) {
                         return word_to_document_freqs_.count(word) &&
                                word_to_document_freqs_.at(word).count(document_id);
                     })) {
        matched_words.resize(query.plus_words.size());
        const auto &iter = std::copy_if(policy,
                                        query.plus_words.begin(), query.plus_words.end(),
                                        matched_words.begin(),
                                        [&](const auto &word) {
                                            return word_to_document_freqs_.count(word) &&
                                                   word_to_document_freqs_.at(word).count(document_id);
                                        });
        matched_words.resize(std::distance(matched_words.begin(), iter));
    }
    sort(policy, matched_words.begin(), matched_words.end());
    auto range_end = unique(policy, matched_words.begin(), matched_words.end());
    matched_words.erase(range_end, matched_words.end());
    return {matched_words, documents_.at(document_id).status};
}

// BOOLEAN

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

[[nodiscard]] bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

// QUERY

[[nodiscard]] SearchServer::QueryWord SearchServer::ParseQueryWord(string_view word) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || !IsValidWord(word)) {
        throw invalid_argument("Query has incorrect symbols in " + string(word) + ".");
    }
    if (is_minus && word[0] == '-') {
        throw invalid_argument("Query has incorrect minus-words:" + string(word) + ".");
    }
    return {word, is_minus, IsStopWord(word)};
}

[[nodiscard]] SearchServer::Query SearchServer::ParseQuery(string_view text) const {
    Query query;
    for (const auto &word: SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
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

[[nodiscard]] SearchServer::QueryVector SearchServer::ParseQueryVector(string_view text) const {
    QueryVector query;
    for (const auto &word: SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

vector<vector<Document>> ProcessQueries(const SearchServer &search_server, const vector<string> &queries) {
    vector<vector<Document>> documents_lists(queries.size());
    transform(execution::par,
              queries.begin(), queries.end(),
              documents_lists.begin(),
              [&search_server](const auto &query) { return search_server.FindTopDocuments(query); });
    return documents_lists;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer &search_server, const vector<std::string> &queries) {
    vector<Document> documents_lists;
    for (const auto &documents: ProcessQueries(search_server, queries)) {
        documents_lists.insert(end(documents_lists), begin(documents), end(documents));
    }
    return documents_lists;
}

// ITERATORS

set<int>::iterator SearchServer::begin() {
    return std::begin(document_ids_);
}

set<int>::iterator SearchServer::end() {
    return std::end(document_ids_);
}

// TOOLS

const map<string_view, double> &SearchServer::GetWordFrequencies(int document_id) const {
    return documents_.at(document_id).freqs;
}

[[nodiscard]] vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (const auto &word: SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Document's content should not contains special characters.");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}