#include <iostream>
#include <cstdlib>
#include "Tokenizer.h"
#include <postgresql/libpq-fe.h>
#include <unordered_map>
#include <unistd.h>
#include "httplib.h"
#include "stemmer.hpp"
#include "db.hpp"
#include "search.hpp"
#include <nlohmann/json.hpp>


struct IntArray {
    long* data;
    int size;
};

enum BoolOp {
    OP_AND,
    OP_OR,
    OP_INVALID
};

struct BoolQuery {
    std::string left;
    std::string right;
    BoolOp op;
};

BoolQuery parse_bool_query(const std::string& q) {
    BoolQuery bq;
    bq.op = OP_INVALID;

    size_t pos;
    if ((pos = q.find(" AND ")) != std::string::npos) {
        bq.left  = q.substr(0, pos);
        bq.right = q.substr(pos + 5);
        bq.op = OP_AND;
    } else if ((pos = q.find(" OR ")) != std::string::npos) {
        bq.left  = q.substr(0, pos);
        bq.right = q.substr(pos + 4);
        bq.op = OP_OR;
    }

    return bq;
}

IntArray intersect(const IntArray& a, const IntArray& b) {
    IntArray res;
    res.data = (long*)malloc(sizeof(long) * (a.size < b.size ? a.size : b.size));
    res.size = 0;

    for (int i = 0; i < a.size; ++i) {
        for (int j = 0; j < b.size; ++j) {
            if (a.data[i] == b.data[j]) {
                res.data[res.size++] = a.data[i];
                break;
            }
        }
    }
    return res;
}


IntArray unite(const IntArray& a, const IntArray& b) {
    IntArray res;
    res.data = (long*)malloc(sizeof(long) * (a.size + b.size));
    res.size = 0;

    for (int i = 0; i < a.size; ++i)
        res.data[res.size++] = a.data[i];

    for (int i = 0; i < b.size; ++i) {
        int exists = 0;
        for (int j = 0; j < a.size; ++j) {
            if (b.data[i] == a.data[j]) {
                exists = 1;
                break;
            }
        }
        if (!exists)
            res.data[res.size++] = b.data[i];
    }
    return res;
}



long get_token_id(PGconn* conn, const char* token) {
    const char* params[1] = { token };

    PGresult* res = PQexecParams(
        conn,
        "SELECT id FROM tokens WHERE token = $1",
        1, NULL, params, NULL, NULL, 0
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return -1;
    }

    long id = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    return id;
}


IntArray get_documents_by_token(PGconn* conn, long token_id) {
    char id_buf[32];
    snprintf(id_buf, sizeof(id_buf), "%ld", token_id);

    const char* params[1] = { id_buf };

    PGresult* res = PQexecParams(
        conn,
        "SELECT document_id FROM document_tokens WHERE token_id = $1",
        1, NULL, params, NULL, NULL, 0
    );

    int rows = PQntuples(res);
    IntArray arr;
    arr.size = rows;
    arr.data = (long*)malloc(sizeof(long) * rows);

    for (int i = 0; i < rows; ++i)
        arr.data[i] = atol(PQgetvalue(res, i, 0));

    PQclear(res);
    return arr;
}



IntArray boolean_search(
    PGconn* conn,
    const char* token1,
    const char* op,
    const char* token2
) {
    long t1 = get_token_id(conn, token1);
    long t2 = get_token_id(conn, token2);

    if (t1 < 0 || t2 < 0) {
        IntArray empty = { nullptr, 0 };
        return empty;
    }

    IntArray d1 = get_documents_by_token(conn, t1);
    IntArray d2 = get_documents_by_token(conn, t2);

    if (strcmp(op, "AND") == 0)
        return intersect(d1, d2);
    else
        return unite(d1, d2);
}


using json = nlohmann::json;
json boolean_search_http(PGconn* conn, const BoolQuery& bq) {
    const char* op =
        (bq.op == OP_AND) ? "AND" : "OR";

    IntArray docs = boolean_search(
        conn,
        bq.left.c_str(),
        op,
        bq.right.c_str()
    );

    json out = json::array();

    for (int i = 0; i < docs.size; ++i) {
        char id_buf[32];
        snprintf(id_buf, sizeof(id_buf), "%ld", docs.data[i]);

        const char* params[1] = { id_buf };

        PGresult* res = PQexecParams(
            conn,
            "SELECT url FROM page_visits WHERE id = $1",
            1, NULL, params, NULL, NULL, 0
        );

        if (PQntuples(res) == 1) {
            out.push_back({
                {"url", PQgetvalue(res, 0, 0)}
            });
        }

        PQclear(res);
    }

    return out;
}




void http_listen() {

    PGconn* conn = connect_db();
    httplib::Server server;

    server.Get("/search", [&](const httplib::Request& req,
                              httplib::Response& res) {
        if (!req.has_param("q")) {
            res.status = 400;
            res.set_content("Missing q", "text/plain");
            return;
        }

        std::string query = req.get_param_value("q");
        std::string mode  = req.has_param("mode")
                            ? req.get_param_value("mode")
                            : "rank";


        if (mode == "bool") {
            BoolQuery bq = parse_bool_query(query);

            if (bq.op == OP_INVALID) {
                res.status = 400;
                res.set_content(
                    "Invalid boolean query. Use: token AND token",
                    "text/plain"
                );
                return;
            }

            json out = boolean_search_http(conn, bq);
            res.set_content(out.dump(2), "application/json");
            return;
        }

        auto tokens = tokenize(query);
        // for (auto& t : tokens)
        //     t = stem(t);

        auto results = search(conn, tokens);

        json out = json::array();
        for (auto& r : results) {
            out.push_back({
                {"url", r.url},
                {"score", r.score}
            });
        }

        res.set_content(out.dump(2), "application/json");
    });

    server.listen("0.0.0.0", 8080);
}


long get_token_id(PGconn* conn, const std::string& token) {
    const char* values[1];
    int lengths[1];
    int formats[1] = {0}; // text

    values[0]  = token.data();
    lengths[0] = static_cast<int>(token.size());

    PGresult* res = PQexecParams(
        conn,
        "INSERT INTO tokens(token) VALUES ($1) "
        "ON CONFLICT (token) DO NOTHING "
        "RETURNING id;",
        1,
        nullptr,
        values,
        lengths,
        formats,
        0
    );

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
        long id = std::stol(PQgetvalue(res, 0, 0));
        PQclear(res);
        return id;
    }
    PQclear(res);

    res = PQexecParams(
        conn,
        "SELECT id FROM tokens WHERE token = $1;",
        1,
        nullptr,
        values,
        lengths,
        formats,
        0
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << PQerrorMessage(conn) << "\n";
        PQclear(res);
        return -1;
    }

    long id = std::stol(PQgetvalue(res, 0, 0));
    PQclear(res);
    return id;
}


int main()
{
    std::thread thread(http_listen); 
        std::cout.setf(std::ios::unitbuf); // каждый вывод flush автоматически
    const char *dsn = std::getenv("STORAGE_DSN");
    if (!dsn)
    {
        std::cerr << "STORAGE_DSN не задан\n";
        return 1;
    }
    PGconn *conn;
    while(true){
        conn = PQconnectdb(dsn);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            std::cerr << "Ошибка подключения: " << PQerrorMessage(conn) << "\n"<<std::endl;
            PQfinish(conn);
            sleep(1);
        }else{
            break;
        }
    }

    std::cout << "Подключение успешно!\n"<<std::endl;

    // Пакетная (batch) обработка документов: читаем партии по LIMIT/OFFSET
    int batch_size = 5;
    const char *bs_env = std::getenv("BATCH_SIZE");
    if (bs_env)
    {
        int v = std::atoi(bs_env);
        if (v > 0)
            batch_size = v;
    }

    int offset = 0;
    while (true)
    {
        std::string query = "SELECT id, data FROM page_visits ORDER BY id LIMIT " + std::to_string(batch_size) + " OFFSET " + std::to_string(offset) + ";";
        PGresult *res = PQexec(conn, query.c_str());

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            std::cerr << PQerrorMessage(conn) << std::endl;
            PQclear(res);
            PQfinish(conn);
            return 1;
        }

        int rows = PQntuples(res);
        if (rows == 0)
        {
            PQclear(res);
            sleep(1);
            continue;
        }

        std::cout << "Обработка документов " << (offset + 1) << "-" << (offset +1+ rows) << "\n";

        for (int i = 0; i < rows; i++)
        {
            long document_id = std::stol(PQgetvalue(res, i, 0));
            std::string data = PQgetvalue(res, i, 1);

            auto tokens = tokenize(data);

            std::unordered_map<std::string, int> freq;
            for (const auto &t : tokens)
            {
                freq[t]++;
            }

            for (const auto &[token, count] : freq)
            {
                long token_id = get_token_id(conn, token);
                if (token_id < 0)
                    continue;

                const char *params[3];
                std::string doc_id = std::to_string(document_id);
                std::string tok_id = std::to_string(token_id);
                std::string cnt = std::to_string(count);

                params[0] = doc_id.c_str();
                params[1] = tok_id.c_str();
                params[2] = cnt.c_str();

                PGresult *ins = PQexecParams(
                    conn,
                    "INSERT INTO document_tokens(document_id, token_id, frequency) "
                    "VALUES ($1, $2, $3) "
                    "ON CONFLICT (document_id, token_id) "
                    "DO UPDATE SET frequency = document_tokens.frequency + EXCLUDED.frequency;",
                    3, nullptr, params, nullptr, nullptr, 0);

                if (PQresultStatus(ins) != PGRES_COMMAND_OK)
                {
                    std::cerr << PQerrorMessage(conn) << "\n";
                }
                PQclear(ins);
            }
        }

        PQclear(res);
        offset += rows;
    }
    PQfinish(conn);
    return 0;
}


