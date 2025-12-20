package main

import (
	"context"
	"flag"
	"log"
	"sync"
	"sync/atomic"
	"time"

	"github.com/Satori27/info_poisk/fetcher"
	"github.com/Satori27/info_poisk/parser"
	"github.com/Satori27/info_poisk/storage"
	_ "github.com/lib/pq"
)

const (
	workersCount = 5
	batchSize    = 10
)

func main() {
	maxURLs := flag.Int("limit", 10000, "max number of urls to crawl")
	startURL := flag.String(
		"start",
		"https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)",
		"start url",
	)
	flag.Parse()

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	parser := parser.NewParser()
	storage, err := storage.NewStorage(time.Hour*24)
	if err!=nil{
		panic("can't connect to db " + err.Error())
	}
	crawler := fetcher.NewCrawler(*startURL, storage, parser)

	var processed int64

	workCh := make(chan string, batchSize)

	workCh <- *startURL
	workCh <- "https://anime-characters-fight.fandom.com/ru/wiki/Служебная:Все_страницы"
	workCh <- "https://harrypotter.fandom.com/ru/wiki/Нимфадора_Тонкс"
	var wg sync.WaitGroup

	worker := func(id int) {
		defer wg.Done()

		for {
			time.Sleep(100 * time.Millisecond)
			select {
			case <-ctx.Done():
				log.Printf("worker stopped %d\n", id)
				return

			case url, ok := <-workCh:
				if !ok {
					log.Printf("worker stopped %d\n", id)
					return
				}

				if atomic.LoadInt64(&processed) >= int64(*maxURLs) {
					log.Printf("worker1 %d\n", id)
					cancel()
					log.Printf("worker stopped %d\n", id)
					return
				}

				links, err := crawler.Crawl(ctx, url)
				if err != nil {
					log.Printf("[worker %d] crawl error: %v", id, err)
					continue
				}
				if len(links) == 0 {
					continue
				}

				atomic.AddInt64(&processed, 1)
				if ctx.Err() != nil {
					log.Printf("worker stopped %d\n", id)
				}

				go func() {
					for _, link := range links {
						if _, ok := storage.LastVisit(link); ok {
							continue
						}
						workCh <- link
					}
				}()

			}
		}

	}

	// стартуем воркеры
	wg.Add(workersCount)
	for i := 0; i < workersCount; i++ {
		go worker(i)
	}

	// мониторинг
	go func() {
		ticker := time.NewTicker(2 * time.Second)
		defer ticker.Stop()

		for range ticker.C {
			log.Printf(
				"processed=%d queued=%d",
				atomic.LoadInt64(&processed),
				len(workCh),
			)

			if atomic.LoadInt64(&processed) >= int64(*maxURLs) {
				cancel()
				return
			}
		}
	}()

	wg.Wait()
	close(workCh)

	log.Printf("DONE. total processed: %d", processed)
}
