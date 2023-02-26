//
// -------- Тестирование ----------
//

#include "test_example_functions.h"

using namespace std;

void AssertImpl(bool value, const string &expr_str, const string &file,
                const string &func, unsigned line, const string &hint) {
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

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении
// документов
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
    search_server.AddDocument(0, "белый кот и модный ошейник"s,
                              DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,
                              DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
                              DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,
                              DocumentStatus::BANNED, {9});
    vector<Document> test =
            search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(test.size(), 3);
    ASSERT_EQUAL(test[0].id, 1);
    ASSERT_EQUAL(test[1].id, 0);
    ASSERT_EQUAL(test[2].id, 2);

    test = search_server.FindTopDocuments(
            "пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) {
                return status == DocumentStatus::ACTUAL;
            });
    ASSERT_EQUAL(test.size(), 3);
    ASSERT_EQUAL(test[0].id, 1);
    ASSERT_EQUAL(test[1].id, 0);
    ASSERT_EQUAL(test[2].id, 2);

    test = search_server.FindTopDocuments(
            "пушистый ухоженный кот"s,
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
    search_server.AddDocument(0, "белый кот и модный ошейник"s,
                              DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,
                              DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
                              DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,
                              DocumentStatus::BANNED, {9});
    vector<Document> test =
            search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(test.size(), 3);
    ASSERT_EQUAL(test[0].rating, (7 + 2 + 7) / 3);
    ASSERT_EQUAL(test[1].rating, (8 - 3) / 2);
    ASSERT_EQUAL(test[2].rating, (5 - 12 + 2 + 1) / 4);
}

void TestCorrectComputingRelevance() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,
                              DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,
                              DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
                              DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,
                              DocumentStatus::BANNED, {9});
    vector<Document> test =
            search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(test.size(), 3);
    ASSERT(abs(test[0].relevance - 0.866434) < EPSILON);
    ASSERT(abs(test[1].relevance - 0.173287) < EPSILON);
    ASSERT(abs(test[2].relevance - 0.173287) < EPSILON);

    test = search_server.FindTopDocuments(
            "пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) {
                return status == DocumentStatus::ACTUAL;
            });
    ASSERT_EQUAL(test.size(), 3);
    ASSERT(abs(test[0].relevance - 0.866434) < EPSILON);
    ASSERT(abs(test[1].relevance - 0.173287) < EPSILON);
    ASSERT(abs(test[2].relevance - 0.173287) < EPSILON);

    test = search_server.FindTopDocuments(
            "пушистый ухоженный кот"s,
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
    search_server.AddDocument(0, "белый кот и модный ошейник"s,
                              DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,
                              DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
                              DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,
                              DocumentStatus::BANNED, {9});
    auto predicate = [](int document_id, DocumentStatus doc_status, int rating) {
        return document_id != 0 && doc_status == DocumentStatus::ACTUAL &&
               rating > 1;
    };
    vector<Document> test =
            search_server.FindTopDocuments("пушистый ухоженный кот"s, predicate);
    ASSERT_EQUAL(test.size(), 1);
    ASSERT(test[0].id == 1);
}

void TestFindDocumentsByStatus() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,
                              DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,
                              DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
                              DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,
                              DocumentStatus::BANNED, {9});
    vector<Document> test = search_server.FindTopDocuments(
            "пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(test.size(), 1);
    ASSERT(test[0].id == 3);
}

void TestEmptyRequests() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    ASSERT_EQUAL(request_queue.GetNoResultRequests(), 1437);
}

void TestPaginator() {
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    ASSERT_EQUAL(pages.size(), 2);
}

void TestRemovingDuplicates() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    ASSERT_EQUAL(search_server.GetDocumentCount(), 9);
    RemoveDuplicates(search_server, false);
    ASSERT_EQUAL(search_server.GetDocumentCount(), 5);
}

void TestProcessQueries() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (const string &text: {"funny pet and nasty rat"s,
                              "funny pet with curly hair"s, "funny pet and not very nasty rat"s,
                              "pet with rat and rat and rat"s, "nasty rat with curly hair"s,}) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {"nasty rat -not"s,
                                    "not very funny nasty pet"s, "curly hair"s};
    id = 0;
    for (const auto &documents: ProcessQueries(search_server, queries)) {
        cout << documents.size() << " documents for query ["s << queries[id++]
             << "]"s << '\n';
    }
}

void StressTestProcessQueries() {
    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 10000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 100'000, 10);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }

    const auto queries = GenerateQueries(generator, dictionary, 10'000, 7);

    SEARCH_SERVER_DURATION;
    const auto documents_lists = ProcessQueries(search_server, queries);
}

void SimpleTestProcessQueriesJoined() {

    SearchServer search_server("and with"s);

    int id = 0;
    for (const string &text: {"funny pet and nasty rat"s,
                              "funny pet with curly hair"s, "funny pet and not very nasty rat"s,
                              "pet with rat and rat and rat"s, "nasty rat with curly hair"s,}) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {"nasty rat -not"s,
                                    "not very funny nasty pet"s, "curly hair"s};
    for (const Document &document: ProcessQueriesJoined(search_server, queries)) {
        cout << "Document "s << document.id << " matched with relevance "s << document.relevance << '\n';
    }
}

void StressTestProcessQueriesJoined() {
    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 10000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 100'000, 10);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }

    const auto queries = GenerateQueries(generator, dictionary, 10'000, 7);

    SEARCH_SERVER_DURATION;
    const auto documents_lists = ProcessQueriesJoined(search_server, queries);
}

void SimpleTestRemoveDocument() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (const string &text: {"funny pet and nasty rat"s,
                              "funny pet with curly hair"s, "funny pet and not very nasty rat"s,
                              "pet with rat and rat and rat"s, "nasty rat with curly hair"s,}) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        cout << search_server.GetDocumentCount() << " documents total, "s
             << search_server.FindTopDocuments(query).size()
             << " documents for query ["s << query << "]"s << endl;
    };

    report();

    // single-threaded version
    search_server.RemoveDocument(5);
    report();

    // single-threaded version
    search_server.RemoveDocument(execution::seq, 1);
    report();

    // multithreaded version
    search_server.RemoveDocument(execution::par, 2);
    report();
}

void StressTestRemoveDocument() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 10000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 100);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1,
                                                                            2, 3});
    }

    {
        SEARCH_SERVER_DURATION;
        const int document_count = search_server.GetDocumentCount();
        for (int id = 0; id < document_count; ++id) {
            search_server.RemoveDocument(execution::seq, id);
        }
        cout << search_server.GetDocumentCount() << endl;
    }

    {
        SEARCH_SERVER_DURATION;
        const int document_count = search_server.GetDocumentCount();
        for (int id = 0; id < document_count; ++id) {
            search_server.RemoveDocument(execution::par, id);
        }
        cout << search_server.GetDocumentCount() << endl;
    }
}

void TestDifferentVersionsMatchDocument() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (const string &text: {"funny pet and nasty rat"s,
                              "funny pet with curly hair"s, "funny pet and not very nasty rat"s,
                              "pet with rat and rat and rat"s, "nasty rat with curly hair"s,}) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq,
                                                                 query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par,
                                                                 query, 3);
        cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }
}

void TestDifferentVersionsFindTopDocuments() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (const string &text: {"white cat and yellow hat"s,
                              "curly cat curly tail"s, "nasty dog with big eyes"s,
                              "nasty pigeon john"s,}) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    cout << "ACTUAL by default:"s << endl;
    //single-threaded version
    for (const Document &document: search_server.FindTopDocuments(
            "curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    //single-threaded version
    for (const Document &document: search_server.FindTopDocuments(
            execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    // multithreaded version
    for (const Document &document: search_server.FindTopDocuments(
            execution::par, "curly nasty cat"s,
            [](int document_id, DocumentStatus status, int rating) {
                return document_id % 2 == 0;
            })) {
        PrintDocument(document);
    }
}

void StressTestDifferentVersionsFindTopDocuments() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1,
                                                                            2, 3});
    }

    const auto queries = GenerateQueries(generator, dictionary, 100, 70);

    {
        SEARCH_SERVER_DURATION;
        double total_relevance = 0;
        for (const string_view query: queries) {
            for (const auto &document: search_server.FindTopDocuments(execution::seq, query)) {
                total_relevance += document.relevance;
            }
        }
        cout << total_relevance << endl;
    }

    {
        SEARCH_SERVER_DURATION;
        double total_relevance = 0;
        for (const string_view query: queries) {
            for (const auto &document: search_server.FindTopDocuments(execution::par, query)) {
                total_relevance += document.relevance;
            }
        }
        cout << total_relevance << endl;
    }

}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    CODE_DURATION("TOTAL TEST");
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsContainingMinusWords);
    RUN_TEST(TestMatching);
    RUN_TEST(TestSorting);
    RUN_TEST(TestCorrectComputingRating);
    RUN_TEST(TestCorrectComputingRelevance);
    RUN_TEST(TestFindDocumentsByPredicate);
    RUN_TEST(TestFindDocumentsByStatus);
    RUN_TEST(TestEmptyRequests);
    RUN_TEST(TestPaginator);
    RUN_TEST(TestRemovingDuplicates);
    RUN_TEST(TestProcessQueries);
    RUN_TEST(StressTestProcessQueries);
    RUN_TEST(SimpleTestProcessQueriesJoined);
    RUN_TEST(StressTestProcessQueriesJoined);
    RUN_TEST(SimpleTestRemoveDocument);
    RUN_TEST(StressTestRemoveDocument);
    RUN_TEST(TestDifferentVersionsMatchDocument);
    RUN_TEST(TestDifferentVersionsFindTopDocuments);
    RUN_TEST(StressTestDifferentVersionsFindTopDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------

string GenerateWord(mt19937 &generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937 &generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937 &generator, const vector<string> &dictionary, int word_count, double minus_prob) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string>
GenerateQueries(mt19937 &generator, const vector<string> &dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

// --------- Генераторы -----------