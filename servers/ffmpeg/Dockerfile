FROM debian:bookworm-slim AS build

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
    && apt-get install -y --no-install-recommends g++ cmake make ca-certificates nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN cmake -S . -B /tmp/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/build -j"$(nproc)"


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
    && apt-get install -y --no-install-recommends ffmpeg ca-certificates libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /tmp/build/ffmpeg /usr/local/bin/ffmpeg_server
WORKDIR /app
EXPOSE 8080
CMD ["ffmpeg_server"]
