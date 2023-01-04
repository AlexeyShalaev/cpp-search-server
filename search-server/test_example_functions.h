//
// Created by Alex Shalaev on 04.01.2023.
//

#ifndef SEARCH_SERVER_TEST_EXAMPLE_FUNCTIONS_H
#define SEARCH_SERVER_TEST_EXAMPLE_FUNCTIONS_H


#include <iostream>



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

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

// --------- Окончание модульных тестов поисковой системы -----------


#endif //SEARCH_SERVER_TEST_EXAMPLE_FUNCTIONS_H
