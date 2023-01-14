//
// -------- Дедупликатор документов ----------
//

#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer &search_server, bool print_info = true) {
    /*
    auto docs = search_server.GetDocuments();
    for (auto it = begin(docs); it != end(docs); it = next(it)) {
        for (auto sub_it = next(it); sub_it != end(docs); sub_it = next(sub_it)) {
            if (sub_it->first > it->first && sub_it->second.content == it->second.content) {
                auto prev_iterator = next(sub_it, -1);
                if (print_info) {
                    cout << "Found duplicate document id " << sub_it->first << endl;
                }
                search_server.RemoveDocument(sub_it->first);
                docs.erase(sub_it->first);
                sub_it = prev_iterator;
            }
        }
    }
     */
    auto docs = search_server.GetDocuments();
    set<set<string>> uniq_contents;
    for (const auto &[id, doc]: search_server.GetDocuments()) {
        if (uniq_contents.count(doc.content) == 0) {
            uniq_contents.insert(doc.content);
        } else {
            if (print_info) {
                cout << "Found duplicate document id " << id << endl;
            }
            search_server.RemoveDocument(id);
            docs.erase(id);
        }
    }
}