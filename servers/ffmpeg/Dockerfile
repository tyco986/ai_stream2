FROM debian:bookworm-slim

RUN printf '%s\n' \
        'Types: deb' \
        'URIs: http://mirrors.tuna.tsinghua.edu.cn/debian' \
        'Suites: bookworm bookworm-updates' \
        'Components: main' \
        'Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg' \
        '' \
        'Types: deb' \
        'URIs: http://mirrors.tuna.tsinghua.edu.cn/debian-security' \
        'Suites: bookworm-security' \
        'Components: main' \
        'Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg' \
        > /etc/apt/sources.list.d/debian.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends g++ ffmpeg ca-certificates nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*
