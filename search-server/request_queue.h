#pragma once
#include <vector>
#include <string>
#include <deque>

#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query,
                                         DocumentStatus status = DocumentStatus::ACTUAL);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        int relevance_time;
    };

    void saveRequest(const std::vector<Document>& result);

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server;
    int current_time_ = 0;
};

//============TEMPLATES========================

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentPredicate document_predicate) {
    const std::vector<Document> result = server.FindTopDocuments(raw_query, document_predicate);
    saveRequest(result);
    return result;
}