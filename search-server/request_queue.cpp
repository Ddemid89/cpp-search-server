#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : server(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const std::vector<Document> result = server.FindTopDocuments(raw_query, status);
    SaveRequest(result);
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return requests_.size();
}

void RequestQueue::SaveRequest(const std::vector<Document>& result) {
    if (!requests_.empty() && requests_.front().relevance_time <= current_time_) {
        requests_.pop_front();
    }
    if (result.empty()) {
        requests_.push_back({current_time_ + min_in_day_});
    }
    current_time_++;
}