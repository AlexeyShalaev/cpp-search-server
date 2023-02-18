//
// -------- Дедупликатор документов ----------
//

#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer &search_server, bool print_info = true) {
    auto docs = search_server.GetDocuments();
    set<set<string_view>> uniq_contents;
    for (const auto &[id, doc]: search_server.GetDocuments()) {
        set<string_view> uniq_words;
        transform(doc.freqs.begin(), doc.freqs.end(),
                  inserter(uniq_words, uniq_words.begin()),
                  [](const auto p) {
                      return p.first;
                  });
        if (uniq_contents.count(uniq_words) == 0) {
            uniq_contents.insert(uniq_words);
        } else {
            if (print_info) {
                cout << "Found duplicate document id " << id << endl;
            }
            search_server.RemoveDocument(id);
            docs.erase(id);
        }
    }
}