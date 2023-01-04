//
// Created by Alex Shalaev on 04.01.2023.
//

#include "test_example_functions.h"
#include "paginator.h"
#include "request_queue.h"
#include "search_server.h"


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
    RUN_TEST(TestEmptyRequests);
    RUN_TEST(TestPaginator);
}

// --------- Окончание модульных тестов поисковой системы -----------
