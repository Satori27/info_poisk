#pragma once
#include <vector>
#include <string>
#include <postgresql/libpq-fe.h>


struct SearchResult {
    std::string url;
    int score;
};

inline std::vector<SearchResult> search(
    PGconn* conn,
    const std::vector<std::string>& tokens
) {
    if (tokens.empty()) return {};

    std::string in;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i) in += ",";
        in += "'" + tokens[i] + "'";
    }

    std::string query = R"(
        SELECT pv.url, SUM(dt.frequency) AS score
        FROM tokens t
        JOIN document_tokens dt ON dt.token_id = t.id
        JOIN page_visits pv ON pv.id = dt.document_id
        WHERE t.token IN ()" + in + R"()
        GROUP BY pv.url
        ORDER BY score DESC
        LIMIT 20;
    )";

    PGresult* res = PQexec(conn, query.c_str());
    std::vector<SearchResult> results;

    for (int i = 0; i < PQntuples(res); ++i) {
        results.push_back({
            PQgetvalue(res, i, 0),
            std::stoi(PQgetvalue(res, i, 1))
        });
    }

    PQclear(res);
    return results;
}
