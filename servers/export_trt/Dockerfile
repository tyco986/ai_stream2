FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch AS base

RUN printf '%s\n' \
        'Types: deb' \
        'URIs: https://mirrors.tuna.tsinghua.edu.cn/ubuntu/' \
        'Suites: noble noble-updates noble-backports' \
        'Components: main universe restricted multiverse' \
        'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' \
        '' \
        'Types: deb' \
        'URIs: https://mirrors.tuna.tsinghua.edu.cn/ubuntu/' \
        'Suites: noble-security' \
        'Components: main universe restricted multiverse' \
        'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' \
        > /etc/apt/sources.list.d/ubuntu.sources


FROM base AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ cmake nlohmann-json3-dev libyaml-cpp-dev libzip-dev zipcmp zipmerge ziptool libspdlog-dev unzip ca-certificates \
    && rm -rf /var/lib/apt/lists/*
COPY CMakeLists.txt main.cpp /src/
COPY src /src/src
RUN cmake -S /src -B /tmp/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/build -j"$(nproc)" \
    && mkdir -p /app \
    && cp /tmp/build/export_trt /app/export_trt


FROM base

RUN apt-get update \
    && apt-get install -y --no-install-recommends libyaml-cpp0.8 libzip4t64 libspdlog1.12 libfmt9 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/export_trt ./export_trt
RUN mkdir -p /root/history /root/logs/export_trt
EXPOSE 8080
CMD ["./export_trt"]
