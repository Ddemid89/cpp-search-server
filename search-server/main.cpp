#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;//Количество выводимых документов

/**
Получает строку из std::cin с помощью getline
*/
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

/**
Получает int из ctd::cin
*/
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

/**
Разделяет строку на слова, используя в качестве разделителя пробел. Возвращает вектор строк (слов)
*/
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

/**
Хранит идентификатор документа и его релевантность
*/
struct Document {
    int id;
    double relevance;
};


class SearchServer {
public:
/**
Получает на вход строку, разделяет ее на слова и вносит результат в вектор стоп-слов
*/
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

/**
Получает идентификатор и текст документа, разделяет его на слова и вносит в словарь обратных индексов
*/
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (auto word : words) {
            documents_[word][document_id] += 1. / words.size(); //map <string, map <int, int>> documents_;
        }                                      //map <слово, map <ID документа, количество вхождения этого слова в этом док-те>>
        documents_count++;
    }

/**
Получает текст запроса и возвращает отсортированный вектор Document (id, relevance) (не более MAX_RESULT_DOCUMENT_COUNT)
*/
    vector<Document> FindTopDocuments(const string& raw_query) const {
        set<string> query_words = ParseQuery(raw_query);
        set<string> minus_words;

        for (string wrd : query_words) {
            if (wrd[0] == '-') {
                minus_words.insert(wrd.erase(0,1));
            }
        }
        for (auto wrd : minus_words) {
            query_words.erase("-"s + wrd);
        }

        auto matched_documents = FindAllDocuments(query_words, minus_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    int documents_count = 0;
    map<string, map<int, double>> documents_;// map<слово, map<id док-та, TF слова>>
    set<string> stop_words_;

/**
Получает слово и проверяет, содержится ли оно в stop_words_ (является ли оно стоп-словом)
*/
    bool IsStopWord(const string& word) const {
        if (stop_words_.empty()) return false;
        string tmp = word;
        if (word[0] == '-') {
            tmp.erase(0,1);
        }
        return stop_words_.count(tmp) > 0;
    }

/**
Получает строку и разбивает на слова, исключая стоп-слова
*/
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

/**
Формирует множество слов запроса из вектора слов запроса
*/
    set<string> ParseQuery(const string& text) const {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            query_words.insert(word);
        }
        return query_words;
    }

/**
Принимает общее кол-во документов, кол-во документов, в которых содержится слово и вохвращает IDF этого слова
*/
double getIDF(int documents_count, int documents_with_word_count) const {
    return log(static_cast<double>(documents_count) / documents_with_word_count);
}

/**
Ищет все документы, в которых содержится хотя-бы одно слово из запроса и не содержится минус-слов, вычисляет TF-IDF релевантность.
Если количество документов == 1, релевантность будет == 0 в любом случае.
*/
    vector<Document> FindAllDocuments(const set<string>& query_words, const set<string>& minus_words) const {
        vector <Document> matched_documents;
        map <int, double> documents_relevations; //Будем увеличивать rels[doc_id]
                                		//Даже если idf = 0 этот документ все равно добавится в map

        for (const string& word : query_words) {
            double idf;
            if (documents_.count(word) != 0) {
                idf = getIDF(documents_count, documents_.at(word).size());
                for (const auto& [id, tf] : documents_.at(word)) {
                    documents_relevations[id] += tf * idf;
                }
            }
        }

        for(auto word : minus_words) {
            if(documents_.count(word) != 0) {
                for(auto [id, tf] : documents_.at(word)) {
                    documents_relevations.erase(id);
                }
            }
        }

        for (auto [id, rel] : documents_relevations) {
            matched_documents.push_back({id, rel});
        }

        return matched_documents;
    }
};

/**
Считывает стоп-слова (первая строка), int N кол-во документов, N строк с документами, возвращает заполненный SearchServer
*/
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
