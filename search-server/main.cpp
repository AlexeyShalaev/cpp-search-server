#include <bits/stdc++.h>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text) {
    vector<string> words;
    string word;
    for (const char c: text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

static int ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

struct Document {
    int id;
    double relevance;
    int rating;

    Document(int _id = 0, double _relevance = 0.0, int _rating = 0) : id(_id), relevance(_relevance), rating(_rating) {}

};

ostream &operator<<(ostream &out, const Document &document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

void PrintDocument(const Document &document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    SearchServer() = default;

    template<typename StringCollectionOrType>
    explicit SearchServer(const StringCollectionOrType &stop_words) {
        SetStopWords(stop_words);
    }

    template<template<class E> typename Container>
    void SetStopWords(const Container<string> &collection) {
        for (const string &word: collection) {
            if (!IsValidWord(word)) throw invalid_argument("Stop words should not contain special characters.");
            stop_words_.insert(word);
        }
    }

    void SetStopWords(const string &text) {
        for (const string &word: SplitIntoWords(text)) {
            if (!IsValidWord(word)) throw invalid_argument("Stop words should not contain special characters.");
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document, DocumentStatus status,
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
    FindTopDocuments(const string &raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status,
                                                    int rating) {
            return doc_status == status;
        });
    }

    template<typename KeyMapper>
    [[nodiscard]] vector<Document>
    FindTopDocuments(const string &raw_query, KeyMapper key_mapper) const {
        const Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, key_mapper);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

    [[nodiscard]] int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    [[nodiscard]] int GetDocumentId(int index) const {
        if (index < 0 || index >= ids.size())
            throw out_of_range("Index is out of range.");
        return ids[index];
    }

    [[nodiscard]] tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
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


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> ids;

    static bool IsValidWord(const string &word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    [[nodiscard]] bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
    }

    [[nodiscard]] vector<string> SplitIntoWordsNoStop(const string &text) const {
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

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    [[nodiscard]] QueryWord ParseQueryWord(string word) const {
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

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    [[nodiscard]] Query ParseQuery(const string &text) const {
        Query query;
        for (const string &word: SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
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

    // Existence required
    double ComputeWordInverseDocumentFreq(const string &word) const {
        return log(GetDocumentCount() / static_cast<double>(word_to_document_freqs_.at(word).size()));
    }

    template<typename KeyMapper>
    vector<Document> FindAllDocuments(const Query &query, KeyMapper key_mapper) const {
        map<int, double> document_to_relevance;
        for (const string &word: query.plus_words) {
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

        for (const string &word: query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance]: document_to_relevance) {
            matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
        }
        return matched_documents;
    }
};

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer &search_server)
            : search_server_(search_server), no_results_requests_(0), current_time_(0) {
    }

    // сделаем "обертки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template<typename DocumentPredicate>
    vector<Document> AddFindRequest(const string &raw_query, DocumentPredicate document_predicate) {
        const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
        AddRequest(result.size());
        return result;
    }

    vector<Document> AddFindRequest(const string &raw_query, DocumentStatus status) {
        const auto result = search_server_.FindTopDocuments(raw_query, status);
        AddRequest(result.size());
        return result;
    }

    vector<Document> AddFindRequest(const string &raw_query) {
        const auto result = search_server_.FindTopDocuments(raw_query);
        AddRequest(result.size());
        return result;
    }

    int GetNoResultRequests() const {
        return no_results_requests_;
    }

private:
    struct QueryResult {
        uint64_t timestamp;
        int results;
    };
    deque<QueryResult> requests_;
    const SearchServer &search_server_;
    int no_results_requests_;
    uint64_t current_time_;
    const static int min_in_day_ = 1440;

    void AddRequest(int results_num) {
        // новый запрос - новая секунда
        ++current_time_;
        // удаляем все результаты поиска, которые устарели
        while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
            if (0 == requests_.front().results) {
                --no_results_requests_;
            }
            requests_.pop_front();
        }
        // сохраняем новый результат поиска
        requests_.push_back({current_time_, results_num});
        if (0 == results_num) {
            ++no_results_requests_;
        }
    }
};

// -------- Итераторы ----------

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
            : first_(begin), last_(end), size_(distance(first_, last_)) {
    }

    Iterator begin() const {
        return first_;
    }

    Iterator end() const {
        return last_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator first_, last_;
    size_t size_;
};

template<typename Iterator>
ostream &operator<<(ostream &out, const IteratorRange<Iterator> &range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

// -------- Пагинация ----------

template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
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

    size_t size() const {
        return pages_.size();
    }

private:
    vector<IteratorRange<Iterator>> pages_;
};

template<typename Container>
auto Paginate(const Container &c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}


// -------- Перегруженные операторы ввода и вывода ----------

template<typename Key, typename Value>
ostream &operator<<(ostream &out, const pair<Key, Value> &pair) {
    out << '(' << pair.first << ", "s << pair.second << ')';
    return out;
}

template<typename Container>
void Print(ostream &out, const Container &container) {
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
ostream &operator<<(ostream &out, const vector<Element> &container) {
    out << '[';
    Print(out, container);
    out << ']';
    return out;
}

template<typename Element>
ostream &operator<<(ostream &out, const set<Element> &container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

template<typename Key, typename Value>
ostream &operator<<(ostream &out, const map<Key, Value> &container) {
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

// -------- Шаблоны тестирующих функций ----------

template<typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str, const string &u_str, const string &file,
                     const string &func, unsigned line, const string &hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string &expr_str, const string &file, const string &func, unsigned line,
                const string &hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template<typename T, typename U>
void RunTestImpl(T &test, U &func) {
    test();
    cerr << func << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestExcludeDocumentsContainingMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
    }

    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in -cat"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
}

void TestMatching() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("in -cat", 42);
        ASSERT(status == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(words.size(), 0);
    }

    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("in", 42);
        ASSERT(status == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(words.size(), 1);
        ASSERT_EQUAL(words[0], "in");
    }
}

void TestSorting() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    vector<Document> test = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(test.size(), 3);
    ASSERT_EQUAL(test[0].id, 1);
    ASSERT_EQUAL(test[1].id, 0);
    ASSERT_EQUAL(test[2].id, 2);

    test = search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                          [](int document_id, DocumentStatus status, int rating) {
                                              return status == DocumentStatus::ACTUAL;
                                          });
    ASSERT_EQUAL(test.size(), 3);
    ASSERT_EQUAL(test[0].id, 1);
    ASSERT_EQUAL(test[1].id, 0);
    ASSERT_EQUAL(test[2].id, 2);

    test = search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                          [](int document_id, DocumentStatus status, int rating) {
                                              return document_id % 2 == 0;
                                          });
    ASSERT_EQUAL(test.size(), 2);
    ASSERT_EQUAL(test[0].id, 0);
    ASSERT_EQUAL(test[1].id, 2);
}

void TestCorrectComputingRating() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    vector<Document> test = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(test.size(), 3);
    ASSERT_EQUAL(test[0].rating, (7 + 2 + 7) / 3);
    ASSERT_EQUAL(test[1].rating, (8 - 3) / 2);
    ASSERT_EQUAL(test[2].rating, (5 - 12 + 2 + 1) / 4);
}

void TestCorrectComputingRelevance() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    vector<Document> test = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(test.size(), 3);
    ASSERT(abs(test[0].relevance - 0.866434) < EPSILON);
    ASSERT(abs(test[1].relevance - 0.173287) < EPSILON);
    ASSERT(abs(test[2].relevance - 0.173287) < EPSILON);

    test = search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                          [](int document_id, DocumentStatus status, int rating) {
                                              return status == DocumentStatus::ACTUAL;
                                          });
    ASSERT_EQUAL(test.size(), 3);
    ASSERT(abs(test[0].relevance - 0.866434) < EPSILON);
    ASSERT(abs(test[1].relevance - 0.173287) < EPSILON);
    ASSERT(abs(test[2].relevance - 0.173287) < EPSILON);

    test = search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                          [](int document_id, DocumentStatus status, int rating) {
                                              return document_id % 2 == 0;
                                          });
    ASSERT_EQUAL(test.size(), 2);
    ASSERT(abs(test[0].relevance - 0.173287) < EPSILON);
    ASSERT(abs(test[1].relevance - 0.173287) < EPSILON);
}

void TestFindDocumentsByPredicate() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    auto predicate = [](int document_id, DocumentStatus doc_status,
                        int rating) {
        return document_id != 0 && doc_status == DocumentStatus::ACTUAL && rating > 1;
    };
    vector<Document> test = search_server.FindTopDocuments("пушистый ухоженный кот"s, predicate);
    ASSERT_EQUAL(test.size(), 1);
    ASSERT(test[0].id == 1);
}

void TestFindDocumentsByStatus() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    vector<Document> test = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(test.size(), 1);
    ASSERT(test[0].id == 3);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsContainingMinusWords);
    RUN_TEST(TestMatching);
    RUN_TEST(TestSorting);
    RUN_TEST(TestCorrectComputingRating);
    RUN_TEST(TestCorrectComputingRelevance);
    RUN_TEST(TestFindDocumentsByPredicate);
    RUN_TEST(TestFindDocumentsByStatus);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

    return 0;
}