FROM debian:bookworm-slim

RUN sed -i \
        -e 's|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g' \
        -e 's|security.debian.org|mirrors.tuna.tsinghua.edu.cn|g' \
        /etc/apt/sources.list.d/debian.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends g++ ffmpeg ca-certificates nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*
