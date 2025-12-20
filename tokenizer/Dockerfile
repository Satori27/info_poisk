FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive


RUN apt-get update && apt-get install -y \
    build-essential \
    libpq-dev \
    locales \
    ca-certificates \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8


WORKDIR /app

COPY . .

RUN make

EXPOSE 8080

CMD ["./app"]
