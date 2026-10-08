FROM drogonframework/drogon AS base

RUN sed -i \
    -e 's|http://archive.ubuntu.com/ubuntu|https://mirrors.tuna.tsinghua.edu.cn/ubuntu|g' \
    -e 's|http://security.ubuntu.com/ubuntu|https://mirrors.tuna.tsinghua.edu.cn/ubuntu|g' \
    /etc/apt/sources.list

RUN apt-get update \
    && apt-get install -y --no-install-recommends libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
EXPOSE 8000


FROM base AS dev


FROM base AS build

COPY servers/backend /src
WORKDIR /src
RUN cmake -S . -B /tmp/build && cmake --build /tmp/build


FROM base AS prod

COPY --from=build /tmp/build/backend /usr/local/bin/backend
CMD ["backend"]
