package fetcher

import (
	"context"
	"io"
	"net/http"
	"net/url"
	"time"
)

type VisitStorage interface {
	LastVisit(url string) (time.Time, bool)
	SaveVisit(url string, data string, t time.Time) error
}

type Parser interface {
	ExtractParagraphsFromContent(body io.ReadCloser, rawURL string) (string, []string, error)
}

type Crawler struct {
	client  *http.Client
	storage VisitStorage
	robots  *RobotsChecker
	domain  string
	parser  Parser
	delay   time.Duration
}

func NewCrawler(domain string, storage VisitStorage, parser Parser) *Crawler {
	u, err := url.Parse(domain)
	if err != nil {
		panic("Invalid url")
	}
	return &Crawler{
		client:  &http.Client{Timeout: 10 * time.Second},
		storage: storage,
		robots:  NewRobotsChecker(),
		domain:  u.Host,
		parser:  parser,
		delay:   1 * time.Second,
	}
}

func (c *Crawler) Crawl(ctx context.Context, rawURL string) ([]string, error) {
	u, err := url.Parse(rawURL)
	if err != nil {
		return []string{}, err
	}

	if u.Host != c.domain {
		return []string{}, nil
	}

	// if !c.robots.Allowed(u.Path) {
	// 	log.Printf("disallowed by robots.txt: %s\n", rawURL)
	// 	return []string{}, nil
	// }

	// Проверка последнего визита
	if last, ok := c.storage.LastVisit(rawURL); ok {
		if time.Since(last) < 24*time.Hour {
			return []string{}, nil
		}
	}

	time.Sleep(c.delay)

	req, _ := http.NewRequestWithContext(ctx, http.MethodGet, rawURL, nil)
	resp, err := c.client.Do(req)
	if err != nil {
		return []string{}, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return []string{}, nil
	}

	data, links, err := c.parser.ExtractParagraphsFromContent(resp.Body, rawURL)
	if err != nil {
		return []string{}, err
	}
	c.storage.SaveVisit(rawURL, data, time.Now())

	return links, nil
}
