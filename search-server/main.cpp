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
    ss.SetStopWords("s1 s2 s3");
    ss.AddDocument(0, "a b c s1 d e f"s, DocumentStatus::ACTUAL, {1});
    ss.AddDocument(1, "b c d e s2 g h"s, DocumentStatus::ACTUAL, {1});
    ss.AddDocument(2, "s3 s2 a o c p"s, DocumentStatus::ACTUAL, {1});
    ASSERT(ss.FindTopDocuments("s1 c"s).size() == 3);
    ASSERT(ss.FindTopDocuments("b s2 s3"s).size() == 2);
    ASSERT(ss.FindTopDocuments("s2 p s1"s).size() == 1);
    ASSERT(ss.FindTopDocuments("s1 s2 s3"s).size() == 0);
    ASSERT(ss.FindTopDocuments("y z x"s).size() == 0);
}

//Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void MinusWordsTest() {
    SearchServer ss;
    ss.SetStopWords("s1 s2 s3");
    ss.AddDocument(0, "a b c s1"s, DocumentStatus::ACTUAL, {1});
    ss.AddDocument(1, "a b c d e s2"s, DocumentStatus::ACTUAL, {1});
    ss.AddDocument(2, "a b c d e f g s3"s, DocumentStatus::ACTUAL, {1});
    ASSERT(ss.FindTopDocuments("a -f s2"s).size() == 2);
    ASSERT(ss.FindTopDocuments("s3 b -d "s).size() == 1);
    ASSERT(ss.FindTopDocuments("-a b s3 c d e s1"s).size() == 0);
    ASSERT(ss.FindTopDocuments("a -s2"s).size() == 3);
    ASSERT(ss.FindTopDocuments("a - p"s).size() == 3);
}

//Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
//присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void MatchingTest() {
    SearchServer ss;
    ss.AddDocument(0, "a b c ", DocumentStatus::ACTUAL, {1});
    ss.AddDocument(1, "a b c d", DocumentStatus::ACTUAL, {1});
    ss.AddDocument(2, "a b c d e", DocumentStatus::ACTUAL, {1});

    {auto [res, stat] = ss.MatchDocument("a c d e f", 0);
    ASSERT(res.size() == 2);}
    {auto [res, stat] = ss.MatchDocument("a c d e f", 1);
    ASSERT(res.size() == 3);}
    {auto [res, stat] = ss.MatchDocument("a c d e f", 2);
    ASSERT(res.size() == 4);}
    {auto [res, stat] = ss.MatchDocument("q w e r t y", 0);
    ASSERT(res.size() == 0);}
    {auto [res, stat] = ss.MatchDocument("q w e r t y", 1);
    ASSERT(res.size() == 0);}
    {auto [res, stat] = ss.MatchDocument("q w e r t y", 2);
    ASSERT(res.size() == 1);}
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
    ASSERT(res[0].id == 0);
    ASSERT(res[1].id == 1);
    ASSERT(res[2].id == 2);

    res = ss.FindTopDocuments("e f g h");
    ASSERT(res.size() == 4);
    ASSERT(res[0].id == 3);
    ASSERT(res[1].id == 2);
    ASSERT(res[2].id == 1);
    ASSERT(res[3].id == 0);
}

//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void RatingTest() {
    SearchServer ss;
    ss.AddDocument(0, "a a a", DocumentStatus::ACTUAL, {2, 3, 4}); // 3
    ss.AddDocument(1, "b b b", DocumentStatus::ACTUAL, {2, 3, -4}); // 0
    ss.AddDocument(2, "c c c", DocumentStatus::ACTUAL, {2, -6, 4}); // 0
    ss.AddDocument(3, "d d d", DocumentStatus::ACTUAL, {2}); // 2
    ss.AddDocument(4, "e e e", DocumentStatus::ACTUAL, {0}); //0
    ss.AddDocument(5, "f f f", DocumentStatus::ACTUAL, {}); //0
    ss.AddDocument(6, "g g g", DocumentStatus::ACTUAL, {-3, 2, -5}); //-2

    vector<Document> res;

    res = ss.FindTopDocuments("a");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 0);
    ASSERT(res[0].rating == 3);

    res = ss.FindTopDocuments("b");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 1);
    ASSERT(res[0].rating == 0);

    res = ss.FindTopDocuments("c");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 2);
    ASSERT(res[0].rating == 0);

    res = ss.FindTopDocuments("d");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 3);
    ASSERT(res[0].rating == 2);

    res = ss.FindTopDocuments("e");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 4);
    ASSERT(res[0].rating == 0);

    res = ss.FindTopDocuments("f");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 5);
    ASSERT(res[0].rating == 0);

    res = ss.FindTopDocuments("g");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 6);
    ASSERT(res[0].rating == -2);

}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void FilterTest() {
    SearchServer ss;
    int i = 0;
    ss.AddDocument(i++, "a a1", DocumentStatus::ACTUAL, {-10, 30});//10
    ss.AddDocument(i++, "a a2", DocumentStatus::ACTUAL, {3, -3, 27});//9
    ss.AddDocument(i++, "a a3", DocumentStatus::ACTUAL, {9, 7});//8
    ss.AddDocument(i++, "a a4", DocumentStatus::BANNED, {7, 6, 8, 7, 21, -7, 7});//7
    ss.AddDocument(i++, "a a5", DocumentStatus::BANNED, {6, 14, 7, -3});//6
    ss.AddDocument(i++, "a a6", DocumentStatus::BANNED, {20, -10});//5
    ss.AddDocument(i++, "a a7", DocumentStatus::IRRELEVANT, {4});//4
    ss.AddDocument(i++, "a a8", DocumentStatus::IRRELEVANT, {6, 0});//3
    ss.AddDocument(i++, "a a9", DocumentStatus::IRRELEVANT, {-4, 8});//2
    ss.AddDocument(i++, "a a10", DocumentStatus::REMOVED, {1, 1, 1, 1});//1
    ss.AddDocument(i++, "a a11", DocumentStatus::REMOVED, {1, 2, 3, -6});//0

    vector <Document> res;

    res = ss.FindTopDocuments("a", [](int document_id, DocumentStatus doc_stat, int rat){
                                doc_stat = doc_stat;
                                rat = rat;
                                return document_id % 2 == 0;
                              });
    ASSERT(res.size() == 5);

    res = ss.FindTopDocuments("a", [](int document_id, DocumentStatus doc_stat, int rat){
                                doc_stat = doc_stat;
                                rat = rat;
                                return document_id % 3 == 0;
                              });
    ASSERT(res.size() == 4);

    res = ss.FindTopDocuments("a", [](int document_id, DocumentStatus doc_stat, int rat){
                                document_id = document_id;
                                rat = rat;
                                return doc_stat != DocumentStatus::BANNED;
                              });
    ASSERT(res.size() == 5);

    res = ss.FindTopDocuments("a", [](int document_id, DocumentStatus doc_stat, int rat){
                                document_id = document_id;
                                return doc_stat != DocumentStatus::BANNED && rat > 6;
                              });
    ASSERT(res.size() == 3);

    res = ss.FindTopDocuments("a", [](int document_id, DocumentStatus doc_stat, int rat){
                                document_id = document_id;
                                doc_stat = doc_stat;
                                return rat > 6;
                              });
    ASSERT(res.size() == 4);

    res = ss.FindTopDocuments("a", [](int document_id, DocumentStatus doc_stat, int rat){
                                document_id = document_id;
                                doc_stat = doc_stat;
                                return rat <= 3;
                              });
    ASSERT(res.size() == 4);
}

//Поиск документов, имеющих заданный статус.
void SearchTest() {
    SearchServer ss;
    int i = 0;
    ss.AddDocument(i++, "a a1", DocumentStatus::ACTUAL, {-10, 30});//10
    ss.AddDocument(i++, "a b a2", DocumentStatus::ACTUAL, {3, -3, 27});//9
    ss.AddDocument(i++, "a b c a3", DocumentStatus::ACTUAL, {9, 7});//8
    ss.AddDocument(i++, "a a4", DocumentStatus::BANNED, {7, 6, 8, 7, 21, -7, 7});//7
    ss.AddDocument(i++, "a b a5", DocumentStatus::BANNED, {6, 14, 7, -3});//6
    ss.AddDocument(i++, "a b c a6", DocumentStatus::BANNED, {20, -10});//5
    ss.AddDocument(i++, "a a7", DocumentStatus::IRRELEVANT, {4});//4
    ss.AddDocument(i++, "a b a8", DocumentStatus::IRRELEVANT, {6, 0});//3
    ss.AddDocument(i++, "a b c a9", DocumentStatus::IRRELEVANT, {-4, 8});//2
    ss.AddDocument(i++, "a a10", DocumentStatus::REMOVED, {1, 1, 1, 1});//1
    ss.AddDocument(i++, "a b a11", DocumentStatus::REMOVED, {1, 2, 3, -6});//0

    vector <Document> res;

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

    res = ss.FindTopDocuments("a b c", DocumentStatus::IRRELEVANT);
    ASSERT(res.size() == 3);
    ASSERT(res[0].id == 8);
    ASSERT(res[1].id == 7);
    ASSERT(res[2].id == 6);

    res = ss.FindTopDocuments("a b c", DocumentStatus::REMOVED);
    ASSERT(res.size() == 2);
    ASSERT(res[0].id == 10);
    ASSERT(res[1].id == 9);
}

bool equal_double (double a, double b) {
    return abs(a - b) < EPSILON;
}

//Корректное вычисление релевантности найденных документов.
void RelevanceTest() {
    SearchServer ss;
    int i = 0;
    DocumentStatus st = DocumentStatus::ACTUAL;
    vector<int> rat = {1, 2, 3};
    ss.AddDocument(i++, "a a a a", st, rat); // a.tf == 1
    ss.AddDocument(i++, "a b a a", st, rat); // a.tf == 0.75 b.tf == 0.25
    ss.AddDocument(i++, "a a b b", st, rat); // a.tf == 0.5 b.tf == 0.5
    ss.AddDocument(i++, "b b a b", st, rat); // a.tf == 0.25 b.tf == 0.75
    ss.AddDocument(i++, "b b b b", st, rat); // b.tf == 1
    ss.AddDocument(i++, "b c b c b b b b b b", st, rat); // b.tf == 0.8 c.tf == 0.2

    double a_idf = log(6. / 4.);
    double b_idf = log(6. / 5.);
    double c_idf = log(6);

    vector <Document> res;

    res = ss.FindTopDocuments("a");

    ASSERT(res.size() == 4);
    ASSERT(res[0].id == 0);
    ASSERT(equal_double(res[0].relevance, a_idf * 1));

    ASSERT(res[1].id == 1);
    ASSERT(equal_double(res[1].relevance, a_idf * .75));

    ASSERT(res[2].id == 2);
    ASSERT(equal_double(res[2].relevance, a_idf * .5));

    ASSERT(res[3].id == 3);
    ASSERT(equal_double(res[3].relevance, a_idf * .25));

    res = ss.FindTopDocuments("b");

    ASSERT(res.size() == 5);

    ASSERT(res[0].id == 4);
    ASSERT(equal_double(res[0].relevance, b_idf * 1.));

    ASSERT(res[1].id == 5);
    ASSERT(equal_double(res[1].relevance, b_idf * .8));

    ASSERT(res[2].id == 3);
    ASSERT(equal_double(res[2].relevance, b_idf * .75));

    ASSERT(res[3].id == 2);
    ASSERT(equal_double(res[3].relevance, b_idf * .5));

    ASSERT(res[4].id == 1);
    ASSERT(equal_double(res[4].relevance, b_idf * .25));

    res = ss.FindTopDocuments("c");
    ASSERT(res.size() == 1);
    ASSERT(res[0].id == 5);
    ASSERT(equal_double(res[0].relevance, c_idf * .2));


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
    RUN_TEST(SearchTest);
    RUN_TEST(RelevanceTest);
}

// --------- Окончание модульных тестов поисковой системы -----------

