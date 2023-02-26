//
// -------- Тестирование ----------
//

#ifndef SEARCH_SERVER_TEST_EXAMPLE_FUNCTIONS_H
#define SEARCH_SERVER_TEST_EXAMPLE_FUNCTIONS_H

#include "log_duration.h"
#include "paginator.h"
#include "request_queue.h"
#include "search_server.h"
#include "remove_duplicates.h"
#include <iostream>
#include <random>


// -------- Шаблоны тестирующих функций ----------

template<typename T, typename U>
void
AssertEqualImpl(const T &t, const U &u, const std::string &t_str, const std::string &u_str, const std::string &file,
                const std::string &func, unsigned line, const std::string &hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "(" << line << "): " << func << ": ";
        std::cout << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cout << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cout << " Hint: " << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void
AssertImpl(bool value, const std::string &expr_str, const std::string &file, const std::string &func, unsigned line,
           const std::string &hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template<typename T, typename U>
void RunTestImpl(T &test, U &func) {
    CODE_DURATION(func);
    test();
    std::cerr << func << " OK" << std::endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();

void TestExcludeDocumentsContainingMinusWords();

void TestMatching();

void TestSorting();

void TestCorrectComputingRating();

void TestCorrectComputingRelevance();

void TestFindDocumentsByPredicate();

void TestFindDocumentsByStatus();

void TestEmptyRequests();

void TestPaginator();

void TestRemovingDuplicates();

void TestProcessQueries();

void StressTestProcessQueries();

void SimpleTestProcessQueriesJoined();

void StressTestProcessQueriesJoined();

void SimpleTestRemoveDocument();

void StressTestRemoveDocument();

void TestDifferentVersionsMatchDocument();

void TestDifferentVersionsFindTopDocuments();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

// --------- Окончание модульных тестов поисковой системы -----------

std::string GenerateWord(std::mt19937 &generator, int max_length);

std::vector<std::string> GenerateDictionary(std::mt19937 &generator, int word_count, int max_length);

std::string GenerateQuery(std::mt19937 &generator, const std::vector<std::string> &dictionary, int word_count,
                          double minus_prob = 0);

std::vector<std::string>
GenerateQueries(std::mt19937 &generator, const std::vector<std::string> &dictionary, int query_count,
                int max_word_count);

// --------- Генераторы -----------

#endif //SEARCH_SERVER_TEST_EXAMPLE_FUNCTIONS_H
