FROM debian:bookworm-slim AS base

RUN rm -f /etc/apt/sources.list.d/debian.sources \
    && printf '%s\n' \
      'deb http://mirrors.aliyun.com/debian/ bookworm main contrib non-free non-free-firmware' \
      'deb http://mirrors.aliyun.com/debian/ bookworm-updates main contrib non-free non-free-firmware' \
      'deb http://mirrors.aliyun.com/debian-security bookworm-security main contrib non-free non-free-firmware' \
      > /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
      ca-certificates \
      cmake \
      g++ \
      git \
      make \
      libjsoncpp-dev \
      libpq-dev \
      libspdlog-dev \
      libssl-dev \
      uuid-dev \
      zip \
      libzip-dev \
      libyaml-cpp-dev \
      zlib1g-dev \
    && rm -rf /var/lib/apt/lists/* \
    && git config --global url."https://gh-proxy.com/https://github.com/".insteadOf "https://github.com/" \
    && git clone --depth 1 --branch v1.9.14 --recursive https://github.com/drogonframework/drogon.git /tmp/drogon \
    && cmake -S /tmp/drogon -B /tmp/drogon-build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_STANDARD=20 \
      -DUSE_COROUTINE=ON \
      -DBUILD_EXAMPLES=OFF \
      -DBUILD_CTL=OFF \
    && cmake --build /tmp/drogon-build -j"$(nproc)" \
    && cmake --install /tmp/drogon-build \
    && rm -rf /tmp/drogon /tmp/drogon-build

WORKDIR /app
EXPOSE 8000


FROM base AS dev


FROM base AS build

COPY servers/backend /src
WORKDIR /src
RUN cmake -S . -B /tmp/build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/build -j"$(nproc)"


FROM base AS prod

COPY --from=build /tmp/build/backend /usr/local/bin/backend
CMD ["backend"]
