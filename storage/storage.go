package storage

import (
	"database/sql"
	"errors"
	"os"
	"time"
)

type Storage struct {
	db  *sql.DB
	ttl time.Duration
}

func NewStorage(ttl time.Duration) (*Storage, error) {

	dsn := os.Getenv("STORAGE_DSN")


	if dsn == "" {
		return nil, errors.New("dsn is required either as argument or via STORAGE_DSN env")
	}

	db, err := sql.Open("postgres", dsn)
	if err != nil {
		return nil, err
	}

	if err := db.Ping(); err != nil {
		return nil, err
	}

	return &Storage{
		db:  db,
		ttl: ttl,
	}, nil
}



func (s *Storage) LastVisit(url string) (time.Time, bool) {
	var visitedAt time.Time

	err := s.db.QueryRow(`
		SELECT visited_at
		FROM page_visits
		WHERE url = $1
	`, url).Scan(&visitedAt)

	if err == sql.ErrNoRows {
		return time.Time{}, false
	}
	if err != nil {
		return time.Time{}, false
	}

	if s.ttl > 0 && time.Since(visitedAt) > s.ttl {
		return time.Time{}, false
	}

	return visitedAt, true
}

func (s *Storage) SaveVisit(url string, data string, t time.Time) error {
	_, err := s.db.Exec(`
		INSERT INTO page_visits (url, data, visited_at)
		VALUES ($1, $2, $3)
		ON CONFLICT (url) DO UPDATE
		SET
			data = EXCLUDED.data,
			visited_at = EXCLUDED.visited_at
	`, url, data, t)

	return err
}
