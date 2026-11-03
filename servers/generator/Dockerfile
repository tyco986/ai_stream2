FROM debian:bookworm-slim AS base

RUN printf '%s\n' \
        'deb http://mirrors.aliyun.com/debian bookworm main' \
        'deb http://mirrors.aliyun.com/debian bookworm-updates main' \
        'deb http://mirrors.aliyun.com/debian-security bookworm-security main' \
        > /etc/apt/sources.list \
    && rm -f /etc/apt/sources.list.d/debian.sources


FROM base AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ make cmake \
        nlohmann-json3-dev libyaml-cpp-dev libzip-dev zipcmp zipmerge ziptool libspdlog-dev libcpp-httplib-dev \
        libopencv-core-dev libopencv-imgproc-dev libopencv-imgcodecs-dev libopencv-videoio-dev \
    && rm -rf /var/lib/apt/lists/*
COPY CMakeLists.txt main.cpp /src/
COPY src /src/src
RUN cmake -S /src -B /tmp/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/build -j"$(nproc)" \
    && mkdir -p /app \
    && cp /tmp/build/generator /app/generator


FROM base

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libyaml-cpp0.7 libzip4 libspdlog1.10 libfmt9 \
        libopencv-core406 libopencv-imgproc406 libopencv-imgcodecs406 libopencv-videoio406 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/generator ./generator
EXPOSE 8091
CMD ["./generator"]
