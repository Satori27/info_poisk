
FROM golang:1.25-alpine AS builder

WORKDIR /app


COPY go.mod go.sum ./
RUN go mod download

COPY . .


RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -o crawler main.go


FROM alpine:latest

WORKDIR /app


COPY --from=builder /app/crawler .

ENV STORAGE_DSN="postgres://postgres:postgres@db:5432/crawler?sslmode=disable"
ENV CRAWL_TTL="48h"

# Запуск приложения
CMD ["./crawler"]
