CREATE TABLE IF NOT EXISTS page_visits (
    id         BIGSERIAL PRIMARY KEY,
    url        TEXT,
    data       TEXT NOT NULL,
    visited_at TIMESTAMPTZ NOT NULL

);
ALTER TABLE page_visits
ADD CONSTRAINT page_visits_url_unique UNIQUE (url);

CREATE INDEX IF NOT EXISTS idx_page_visits_visited_at
    ON page_visits (visited_at);


CREATE TABLE tokens (
    id     BIGSERIAL PRIMARY KEY,
    token  TEXT NOT NULL UNIQUE
);

CREATE TABLE document_tokens (
    document_id BIGINT NOT NULL,
    token_id    BIGINT NOT NULL,
    frequency   INT NOT NULL,

    PRIMARY KEY (document_id, token_id),

    FOREIGN KEY (document_id) REFERENCES page_visits(id) ON DELETE CASCADE,
    FOREIGN KEY (token_id)    REFERENCES tokens(id)    ON DELETE CASCADE
);

CREATE INDEX idx_tokens_token ON tokens(token);
CREATE INDEX idx_document_tokens_token
    ON document_tokens(token_id);

CREATE INDEX idx_document_tokens_document
    ON document_tokens(document_id);
