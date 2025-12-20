package parser

import (
	"encoding/json"
	"fmt"
	"io"
	"net/url"
	"strings"

	"github.com/PuerkitoBio/goquery"
)

const categoryName string = "Categories"

type Parser struct{}

func NewParser() *Parser {
	return &Parser{}
}

// ExtractParagraphsFromContent парсит страницу и достаёт текст всех <p> из блока
func (p *Parser) ExtractParagraphsFromContent(body io.ReadCloser, rawURL string) (string, []string, error) {
	doc, err := goquery.NewDocumentFromReader(body)
	if err != nil {
		return "", []string{}, fmt.Errorf("failed to parse HTML: %w", err)
	}

	result := make(map[string]interface{})

	// Ищем блок с двумя классами
	doc.Find(".mw-content-ltr.mw-parser-output p").Each(func(i int, s *goquery.Selection) {
		bold := s.Find("b").First()
		if bold.Length() == 0 {
			return
		}

		key := cleanKey(bold.Text())
		if key == "" {
			return
		}

		fullText := strings.TrimSpace(s.Text())

		// Убираем ключ из текста
		value := strings.TrimSpace(
			strings.TrimPrefix(fullText, key),
		)

		// Убираем ведущие символы
		value = strings.TrimLeft(value, " -—:")

		result[key] = value
	})

	doc.Find(".mw-content-ltr.mw-parser-output ul li").Each(func(i int, li *goquery.Selection) {
		bold := li.Find("b").First()
		if bold.Length() == 0 {
			return
		}

		key := cleanKey(bold.Text())
		if key == "" {
			return
		}

		fullText := strings.TrimSpace(li.Text())

		// Убираем ключ из текста
		value := strings.TrimSpace(
			strings.TrimPrefix(fullText, key),
		)

		// Убираем ведущие символы: "-", "—", ":"
		value = strings.TrimLeft(value, " -—:")

		result[key] = value
	})
	var categories []string
	doc.Find(".container ul li").Each(func(i int, s *goquery.Selection) {
		text := strings.TrimSpace(s.Text())
		if text != "" {
			categories = append(categories, text)
		}
	})

	var links []string

    doc.Find("a[href]").Each(func(_ int, s *goquery.Selection) {
        href, ok := s.Attr("href")
        if !ok {
            return
        }
		baseURL, err := url.Parse("https://anime-characters-fight.fandom.com/ru/wiki/Служебная:Все_страницы?from=Kyoukai+no+Kanata")
		if err!= nil {
			panic("bad base url")
		}

        link, err := baseURL.Parse(href)
        if err != nil {
            return
        }

        // Фильтр: только ссылки на тот же домен
        if link.Host != baseURL.Host {
            return
        }

        links = append(links, link.String())
    })
	result[categoryName] = categories

	jsonBytes, err := json.MarshalIndent(result, "", "  ")
	if err != nil {
		return "", []string{}, err
	}

	return string(jsonBytes), links, nil
}

func cleanKey(s string) string {
	s = strings.TrimSpace(s)
	s = strings.Trim(s, `"“”«»`)
	return s
}
