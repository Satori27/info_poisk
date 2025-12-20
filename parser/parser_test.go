package parser

import (
	"github.com/stretchr/testify/assert"
	"net/http"
	"testing"
)

func TestExtractTextFromURLByClass(t *testing.T) {
	url := "https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)"
	p := NewParser()

	resp, err := http.Get(url)
	assert.Nil(t, err)

	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		t.Error("status code is not ok")
	}

	texts, links, err := p.ExtractParagraphsFromContent(resp.Body, url)
	if err != nil {
		t.Fatalf("ExtractTextFromURLByClass failed: %v", err)
	}
	assert.Greater(t, len(texts), 0)
	assert.Greater(t, len(links), 0)

	t.Log(texts)
	t.Log(links)

}
