#pragma once
#include <postgresql/libpq-fe.h>
#include <stdexcept>

inline PGconn* connect_db() {
    const char* dsn = std::getenv("STORAGE_DSN");
    if (!dsn)
        throw std::runtime_error("STORAGE_DSN not set");

    PGconn* conn = PQconnectdb(dsn);
    if (PQstatus(conn) != CONNECTION_OK)
        throw std::runtime_error(PQerrorMessage(conn));

    return conn;
}
