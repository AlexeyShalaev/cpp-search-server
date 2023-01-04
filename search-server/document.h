//
// Created by Alex Shalaev on 04.01.2023.
//

#ifndef SEARCH_SERVER_DOCUMENT_H
#define SEARCH_SERVER_DOCUMENT_H


#include <iostream>

struct Document {

    Document(int _id = 0, double _relevance = 0.0, int _rating = 0);

    int id;
    double relevance;
    int rating;
};

std::ostream &operator<<(std::ostream &out, const Document &document);

void PrintDocument(const Document &document);

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};


#endif //SEARCH_SERVER_DOCUMENT_H
