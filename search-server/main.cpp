#include <numeric>
// -------- Начало модульных тестов поисковой системы ----------
const double EPSILON = 10e-6;
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

//Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
//Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
void AddingTest() {
    SearchServer ss;

    ASSERT(ss.GetDocumentCount() == 0);

    ss.AddDocument(0, "a b c s1 d e f"s, DocumentStatus::ACTUAL, {1});
    ASSERT(ss.GetDocumentCount() == 1);

    ss.AddDocument(1, "b c d e s2 g h"s, DocumentStatus::ACTUAL, {1});
    ASSERT(ss.GetDocumentCount() == 2);

    ss.AddDocument(2, "s3 s2 a o c p"s, DocumentStatus::ACTUAL, {1});
    ASSERT(ss.GetDocumentCount() == 3);

    ASSERT(ss.FindTopDocuments("c"s).size() == 3);
    ASSERT(ss.FindTopDocuments("b"s).size() == 2);
    ASSERT(ss.FindTopDocuments(""s).size() == 0);
    ASSERT(ss.FindTopDocuments("y z x"s).size() == 0);
}

//Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void MinusWordsTest() {
    SearchServer ss;

    ss.AddDocument(0, "a b c"s, DocumentStatus::ACTUAL, {1});
    ss.AddDocument(1, "a b c d e"s, DocumentStatus::ACTUAL, {1});
    ss.AddDocument(2, "a b c d e f g"s, DocumentStatus::ACTUAL, {1});

    vector<Document> res = ss.FindTopDocuments("a -f"s);
    ASSERT(res.size() == 2);
    ASSERT(res[0].id == 0);
    ASSERT(res[1].id == 1);

    res = ss.FindTopDocuments("b -d "s);
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 0);

    res = ss.FindTopDocuments("a -p"s);
    ASSERT(res.size() == 3);
    ASSERT(res[0].id == 0);
    ASSERT(res[1].id == 1);
    ASSERT(res[2].id == 2);
}

//Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
//присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void MatchingTest() {
    SearchServer ss;
    ss.SetStopWords("s1 s2 s3");
    ss.AddDocument(0, "a b c s1", DocumentStatus::ACTUAL, {1});
    ss.AddDocument(1, "a b c d s2", DocumentStatus::IRRELEVANT, {1});
    ss.AddDocument(2, "a b c d e s3", DocumentStatus::BANNED, {1});
    ss.AddDocument(3, "d e f g y s4", DocumentStatus::REMOVED, {1});

    {auto [res, stat] = ss.MatchDocument("a c d e f s1", 0); //Соответствующий документ
    vector<string> expected_result = {"a", "c"};
    ASSERT(res.size() == 2);
    ASSERT_EQUAL(res, expected_result);
    ASSERT(stat == DocumentStatus::ACTUAL);
    }

    //Пустой из-за минус-слова
    {auto [res, stat] = ss.MatchDocument("a c -d e f s1", 1);
    vector<string> expected_result;
    ASSERT(res.size() == 0);
    ASSERT_EQUAL(res, expected_result);
    ASSERT(stat == DocumentStatus::IRRELEVANT);
    }

    //Игнорирует минус-стоп-слово
    {auto [res, stat] = ss.MatchDocument("a c d e f -s3", 2);
    vector<string> expected_result = {"a", "c", "d", "e"};
    ASSERT(res.size() == 4);
    ASSERT_EQUAL(res, expected_result);
    ASSERT(stat == DocumentStatus::BANNED);
    }

    //Игнорирует отсутствующее минус-слово
    {auto [res, stat] = ss.MatchDocument("a c d e -f s1", 0);
    vector<string> expected_result = {"a", "c"};
    ASSERT(res.size() == 2);
    ASSERT_EQUAL(res, expected_result);
    ASSERT(stat == DocumentStatus::ACTUAL);
    }

    //Поиск по стоп-слову = пустой запрос
    {auto [res, stat] = ss.MatchDocument("s3", 3);
    vector<string> expected_result;
    ASSERT(res.size() == 0);
    ASSERT_EQUAL(res, expected_result);
    ASSERT(stat == DocumentStatus::REMOVED);
    }
}

//Сортировка найденных документов по релевантности.
//Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void SortTest() {
    SearchServer ss;
    ss.AddDocument(0, "a b c d e", DocumentStatus::ACTUAL, {1});
    ss.AddDocument(1, "b c d e f", DocumentStatus::ACTUAL, {1});
    ss.AddDocument(2, "c d e f g", DocumentStatus::ACTUAL, {1});
    ss.AddDocument(3, "d e f g h", DocumentStatus::ACTUAL, {1});

    vector<Document> res;

    res = ss.FindTopDocuments("a b c");
    ASSERT(res.size() == 3);
    for (int i = 1; i < res.size(); ++ i) {
        ASSERT(res[i - 1].relevance >= res[i].relevance);
    }
    ASSERT(res[0].id == 0);
    ASSERT(res[1].id == 1);
    ASSERT(res[2].id == 2);

    res = ss.FindTopDocuments("e f g h");
    for (int i = 1; i < res.size(); ++ i) {
        ASSERT(res[i - 1].relevance >= res[i].relevance);
    }
    ASSERT(res.size() == 4);
    ASSERT(res[0].id == 3);
    ASSERT(res[1].id == 2);
    ASSERT(res[2].id == 1);
    ASSERT(res[3].id == 0);
}

int getAverRating(const vector<int>& ratings) {
    if (ratings.empty()) return 0;
    return accumulate(ratings.begin(), ratings.end(), 0, plus<int>()) / static_cast<int>(ratings.size());;
}

//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void RatingTest() {
    SearchServer ss;
    int i = 0;
    vector<vector<int>> ratings = {{2, 3, 4}, {0}, {}, {-3, 2, -5}};
    vector<int> docs_av_rating(ratings.size());

    for (int i = 0; i < ratings.size(); ++i) {
        docs_av_rating[i] = getAverRating(ratings[i]);
    }

    ss.AddDocument(i, "a a a", DocumentStatus::ACTUAL, ratings[i]); ++i; 
    ss.AddDocument(i, "b b b", DocumentStatus::ACTUAL, ratings[i]); ++i; 
    ss.AddDocument(i, "c c c", DocumentStatus::ACTUAL, ratings[i]); ++i; 
    ss.AddDocument(i, "d d d", DocumentStatus::ACTUAL, ratings[i]); ++i; 

    vector<Document> res;
    vector<string> requests = {"a", "b", "c", "d"};

    for (int expected_doc = 0; i < requests.size(); ++i) {
        res = ss.FindTopDocuments(requests[expected_doc]);
        ASSERT(res.size() == 1);
        ASSERT(res[0].id == expected_doc);
        ASSERT(res[0].rating == docs_av_rating[expected_doc]);
    }
}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void FilterTest() {
    SearchServer ss;
    int i = 0;
    ss.AddDocument(i++, "a a1", DocumentStatus::ACTUAL, {-10, 30});
    ss.AddDocument(i++, "a a2", DocumentStatus::ACTUAL, {3, -3, 27});
    ss.AddDocument(i++, "a a3", DocumentStatus::ACTUAL, {9, 7});//8
    ss.AddDocument(i++, "a a4", DocumentStatus::BANNED, {7, 6, 8, 7, 21, -7, 7});
    ss.AddDocument(i++, "a a5", DocumentStatus::BANNED, {6, 14, 7, -3});
    ss.AddDocument(i++, "a a6", DocumentStatus::BANNED, {20, -10});
    ss.AddDocument(i++, "a a7", DocumentStatus::IRRELEVANT, {4});
    ss.AddDocument(i++, "a a8", DocumentStatus::IRRELEVANT, {6, 0});
    ss.AddDocument(i++, "a a9", DocumentStatus::IRRELEVANT, {-4, 8});
    ss.AddDocument(i++, "a a10", DocumentStatus::REMOVED, {1, 1, 1, 1});
    ss.AddDocument(i++, "a a11", DocumentStatus::REMOVED, {1, 2, 3, -6});

    vector <Document> res;

    res = ss.FindTopDocuments("a", [](int document_id,
                                [[maybe_unused]] DocumentStatus doc_stat,
                                [[maybe_unused]] int rat){
                                return document_id % 2 == 0;
                              });
    ASSERT(res.size() == 5);

    res = ss.FindTopDocuments("a", []([[maybe_unused]] int document_id,
                                    DocumentStatus doc_stat, int rat){
                                return doc_stat != DocumentStatus::BANNED && rat > 6;
                              });
    ASSERT(res.size() == 3);

    res = ss.FindTopDocuments("a", []([[maybe_unused]] int document_id,
                            [[maybe_unused]] DocumentStatus doc_stat,
                            int rat){
                                return rat <= 3;
                              });
    ASSERT(res.size() == 4);
}

//Поиск документов, имеющих заданный статус.
void StatusTest() {
    SearchServer ss;
    int i = 0;
    ss.AddDocument(i++, "a a1", DocumentStatus::ACTUAL, {-10, 30});
    ss.AddDocument(i++, "a b a2", DocumentStatus::ACTUAL, {3, -3, 27});
    ss.AddDocument(i++, "a b c a3", DocumentStatus::ACTUAL, {9, 7});
    ss.AddDocument(i++, "a a4", DocumentStatus::BANNED, {7, 6, 8, 7, 21, -7, 7});
    ss.AddDocument(i++, "a b a5", DocumentStatus::BANNED, {6, 14, 7, -3});
    ss.AddDocument(i++, "a b c a6", DocumentStatus::BANNED, {20, -10});

    vector <Document> res;
    res = ss.FindTopDocuments("a", DocumentStatus::IRRELEVANT);
    ASSERT(res.size() == 0);

    ss.AddDocument(i++, "a a7", DocumentStatus::IRRELEVANT, {4});
    ss.AddDocument(i++, "a b a8", DocumentStatus::IRRELEVANT, {6, 0});
    ss.AddDocument(i++, "a b c a9", DocumentStatus::IRRELEVANT, {-4, 8});
    ss.AddDocument(i++, "a a10", DocumentStatus::REMOVED, {1, 1, 1, 1});
    ss.AddDocument(i++, "a b a11", DocumentStatus::REMOVED, {1, 2, 3, -6});

    res = ss.FindTopDocuments("a b c", DocumentStatus::ACTUAL);
    ASSERT(res.size() == 3);
    ASSERT(res[0].id == 2);
    ASSERT(res[1].id == 1);
    ASSERT(res[2].id == 0);

    res = ss.FindTopDocuments("a b c", DocumentStatus::BANNED);
    ASSERT(res.size() == 3);
    ASSERT(res[0].id == 5);
    ASSERT(res[1].id == 4);
    ASSERT(res[2].id == 3);

}

bool equal_double (double a, double b) {
    return abs(a - b) < EPSILON;
}

vector<string> split_into_words(const string& text) {
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

double getIDF(int total_docs, int docs_with_word) {
    return log(static_cast<double>(total_docs) / docs_with_word);
}

//Корректное вычисление релевантности найденных документов.
void RelevanceTest() {
    SearchServer ss;
    DocumentStatus st = DocumentStatus::ACTUAL;
    vector<int> rat = {1, 2, 3};
    vector<string> docs = {"a a a a"s, "a b a a"s, "a a b b"s,
                           "b b a b"s, "b b b b"s, "b c b c b b b b b b"s};

    vector<vector<string>> docs_by_words(docs.size());//Документы с разделенными словами

    for (int i = 0; i < docs.size(); ++i) {
        ss.AddDocument(i, docs[i], st, rat);

        docs_by_words[i] = split_into_words(docs[i]);
    }

    set<string> words_used;//Используемые слова

    for (const auto& doc_by_words : docs_by_words) {
        for (const auto& word : doc_by_words) {
            words_used.insert(word);// или if(words_used.count(word) == 0) words_used.isert(word);
        }
    }

    map<string, int> docs_with_word;//В скольких документах встречается слово
    vector<map<string, double>> words_in_docs_TF(docs.size());

    for (int i = 0; i < docs.size(); ++i) {
        for (const auto& word : words_used) {
            if (count(docs_by_words[i].begin(), docs_by_words[i].end(), word) > 0)
                docs_with_word[word]++;
        }
        double part_size = 1. / docs_by_words[i].size();
        for (const auto& word : docs_by_words[i]) {
            words_in_docs_TF[i][word] += part_size;
        }
    }

    int total_docs = ss.GetDocumentCount();

    map<string, double> words_idfs;
    for (const auto& word : words_used){
        words_idfs[word] = getIDF(total_docs, docs_with_word[word]);
    }

    vector <Document> res;

    string search_word = "a";

    res = ss.FindTopDocuments(search_word);

    ASSERT(res.size() == 4);

    int i = 0;

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 0);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 1);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 2);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 3);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    search_word = "b";
    i = 0;
    res = ss.FindTopDocuments(search_word);

    ASSERT(res.size() == 5);

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 4);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 5);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 3);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 2);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 1);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }

    search_word = "c";
    i = 0;
    res = ss.FindTopDocuments(search_word);

    ASSERT(res.size() == 1);
    {
        Document& current_doc = res[i++];
        int id = current_doc.id;
        ASSERT(id == 5);
        ASSERT(equal_double(current_doc.relevance, words_idfs[search_word] * words_in_docs_TF[id][search_word]));
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(AddingTest);
    RUN_TEST(MinusWordsTest);
    RUN_TEST(MatchingTest);
    RUN_TEST(SortTest);
    RUN_TEST(RatingTest);
    RUN_TEST(FilterTest);
    RUN_TEST(StatusTest);
    RUN_TEST(RelevanceTest);
}
// --------- Окончание модульных тестов поисковой системы -----------
