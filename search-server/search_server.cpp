#include "search_server.h"

#include <functional>
#include <numeric>
#include <cmath>
#include <string_view>
#include <cassert>

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(const std::string_view& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(const char* stop_words_text)
    : SearchServer(
        SplitIntoWords(std::string_view(stop_words_text)))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    using namespace std;
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        auto it = documents_words_.insert(std::string(word));
        std::string_view word_view = *(it.first);
        word_to_document_freqs_[word_view][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word_view] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
                                                     const std::string_view raw_query,
                                                     DocumentStatus status) const
{
    return FindTopDocuments(
        policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
                                                     const std::string_view raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
                                                     const std::string_view raw_query,
                                                     DocumentStatus status) const
{
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
                                                     const std::string_view raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> empty_ = {};

    auto it = document_to_word_freqs_.find(document_id);

    if (it == document_to_word_freqs_.end()) {
        return empty_;
    }
    return (*it).second;
}

void SearchServer::RemoveDocument(int document_id){
    auto document_it = document_to_word_freqs_.find(document_id);
    if (document_it == document_to_word_freqs_.end()) {
        return;
    }

    for(auto& [word, freq] : (*document_it).second){
        word_to_document_freqs_[word].erase(document_id);
    }

    document_to_word_freqs_.erase(document_it);
    document_ids_.erase(std::find(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    auto document_it = document_to_word_freqs_.find(document_id);
    if (document_it == document_to_word_freqs_.end()) {
        return;
    }

    std::map<std::string_view, double>& doc = (*document_it).second;
    std::vector<std::string_view> words_list(doc.size());

    std::transform(policy, doc.begin(), doc.end(), words_list.begin(),
               [](const std::pair<const std::string_view, const double>& word_freq){
                    return word_freq.first;
               });

    auto& word_to_doc = word_to_document_freqs_;

    std::for_each(policy, words_list.begin(), words_list.end(),
               [&word_to_doc, document_id](const std::string_view current_word){
                    (word_to_doc)[current_word].erase(document_id);
                    return true;
               });

    document_to_word_freqs_.erase(document_it);
    document_ids_.erase(std::find(policy, document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    RemoveDocument(document_id);
}

std::set<int>::const_iterator SearchServer::begin() {
    return document_ids_.cbegin();
}

std::set<int>::const_iterator SearchServer::end() {
    return document_ids_.cend();
}

SearchServer::MathedDocuments SearchServer::MatchDocument(const std::string_view& raw_query,
                                            int document_id) const {
    auto it = document_to_word_freqs_.find(document_id);
    if (it == document_to_word_freqs_.end()) {
        throw std::out_of_range("No document with id = " + std::to_string(document_id));
    }

    const Query query = ParseQuery(raw_query);

    const std::map<std::string_view, double>& words_map = (*it).second;

    bool no_minus_words = std::all_of(query.minus_words.begin(), query.minus_words.end(),
                    [&words_map](const std::string_view word){
                        return words_map.count(word) == 0;
                    });

    if (!no_minus_words) {
        return {std::vector<std::string_view>(), documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto end_it = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
                                [&words_map](const std::string_view& word){
                                    return words_map.count(word) != 0;
                                });

    matched_words.erase(end_it, matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

SearchServer::MathedDocuments SearchServer::MatchDocument(const std::execution::sequenced_policy& policy,
                                                          const std::string_view& raw_query,
                                                          int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

SearchServer::MathedDocuments SearchServer::MatchDocument(const std::execution::parallel_policy& policy,
                                                          const std::string_view& raw_query,
                                                          int document_id) const
{
    auto it = document_to_word_freqs_.find(document_id);
    if (it == document_to_word_freqs_.end()) {
        throw std::out_of_range("No document with id = " + std::to_string(document_id));
    }

    QueryVec query = ParseQueryVec(raw_query);

    const std::map<std::string_view, double>& words_map = (*it).second;

    bool no_minus_words = std::all_of(query.minus_words.begin(), query.minus_words.end(),
                    [&words_map](const std::string_view& word){
                        return words_map.count(word) == 0;
                    });

    if (!no_minus_words) {
        return {std::vector<std::string_view>(), documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto end_it = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
                                [&words_map](const std::string_view& word){
                                    return words_map.count(word) != 0;
                                });

    matched_words.erase(end_it, matched_words.end());
    std::sort(matched_words.begin(), matched_words.end());

    end_it = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(end_it, matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_view_.count(word) > 0;
}


bool SearchServer::IsValidWord(const std::string_view& word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    using namespace std;
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int result = std::accumulate(ratings.begin(), ratings.end(), 0, std::plus<int>());
    result /= static_cast<int>(ratings.size());
    return result;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word) const {
    using namespace std;
    if (word.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + word.data() + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query result;

    for (const std::string_view& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

SearchServer::QueryVec SearchServer::ParseQueryVec(const std::string_view text) const {
    QueryVec result;

    auto words = SplitIntoWords(text);
    result.minus_words.reserve(words.size());
    result.plus_words.reserve(words.size());

    for (const std::string_view& word : words) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    return result;
}

std::set<std::string_view> SearchServer::MakeStopWordsSet(const std::vector<std::string>& words) {
    std::set<std::string_view> result;
    for (const std::string& word : words) {
        result.emplace(word);
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
