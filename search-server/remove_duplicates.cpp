#include "remove_duplicates.h"

#include <set>
#include <vector>
#include <string>
#include <iostream>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> words_sets;
    std::vector<int> to_delete;

    for (int document_id : search_server) {
        std::set<std::string> current_doc_words;
        for (auto& [word, freq] : search_server.GetWordFrequencies(document_id)) {
            current_doc_words.emplace(word);
        }
        if (words_sets.count(current_doc_words) == 0) {
            words_sets.emplace(current_doc_words);
        } else {
            to_delete.push_back(document_id);
        }
    }

    for(int document_id : to_delete) {
        std::cout << "Found duplicate document id " << document_id << '\n';
        search_server.RemoveDocument(document_id);
    }
}
