FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch

# China mirror acceleration (Ubuntu 24.04 uses DEB822 format)
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://mirrors.tuna.tsinghua.edu.cn/ubuntu/|g' \
        /etc/apt/sources.list.d/ubuntu.sources && \
    sed -i 's|http://security.ubuntu.com/ubuntu/|http://mirrors.tuna.tsinghua.edu.cn/ubuntu/|g' \
        /etc/apt/sources.list.d/ubuntu.sources
RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple

# Install pyservicemaker (wheel ships inside the container) + Python deps
RUN pip install --break-system-packages \
    /opt/nvidia/deepstream/deepstream/service-maker/python/pyservicemaker*.whl \
    pyyaml \
    requests \
    confluent-kafka \
    nvidia-ml-py \
    Pillow

# Runtime C libs + pkg-config for C ext build
RUN apt-get update && apt-get install -y --no-install-recommends \
    librdkafka1 \
    curl \
    pkg-config \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Static FFmpeg for recording tools; MediaMTX as RTSP/WebRTC server
RUN curl -fsSL "https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-amd64-static.tar.xz" \
    | tar xJ --strip-components=1 -C /usr/local/bin --wildcards '*/ffmpeg' '*/ffprobe'
ARG MEDIAMTX_VERSION=1.17.1
RUN curl -fsSL \
    "https://gh.llkk.cc/https://github.com/bluenviron/mediamtx/releases/download/v${MEDIAMTX_VERSION}/mediamtx_v${MEDIAMTX_VERSION}_linux_amd64.tar.gz" \
    | tar xz -C /usr/local/bin mediamtx

WORKDIR /app
COPY . /app

# Build the SmartRecord C extension (nvdssr_ext)
RUN cd /app/ext && python3 setup.py build_ext --inplace \
    && cp nvdssr_ext*.so /app/ \
    && cd /app && rm -rf ext/build

RUN mkdir -p /app/storage/recordings

ENV NVIDIA_DRIVER_CAPABILITIES=${NVIDIA_DRIVER_CAPABILITIES},video

EXPOSE 9000 8554 8889

ENTRYPOINT ["python3", "main.py"]
