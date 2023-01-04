//
// Created by Alex Shalaev on 04.01.2023.
//

#include "document.h"

using namespace std;

Document::Document(int _id, double _relevance, int _rating) : id(_id), relevance(_relevance),
                                                              rating(_rating) {}

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

