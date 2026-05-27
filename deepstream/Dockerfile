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

# Static FFmpeg for recording tools (place ffmpeg-release-amd64-static.tar.xz next to this Dockerfile)
COPY ffmpeg-release-amd64-static.tar.xz /tmp/ffmpeg.tar.xz
RUN tar xJ -f /tmp/ffmpeg.tar.xz --strip-components=1 -C /usr/local/bin \
    --wildcards '*/ffmpeg' '*/ffprobe' && rm -f /tmp/ffmpeg.tar.xz
# Place mediamtx_v1.17.1_linux_amd64.tar.gz next to this Dockerfile before build.
COPY mediamtx_v1.17.1_linux_amd64.tar.gz /tmp/mediamtx.tar.gz
RUN tar xzf /tmp/mediamtx.tar.gz -C /usr/local/bin mediamtx && rm -f /tmp/mediamtx.tar.gz

WORKDIR /app
COPY . /app
RUN rm -f /app/mediamtx_v*_linux_amd64.tar.gz /app/ffmpeg-release-amd64-static.tar.xz

RUN cd /app/ext && python3 setup.py build_ext --inplace \
    && cp nvdssr_ext*.so /app/ \
    && rm -rf ext/build

RUN mkdir -p /app/storage/recordings

ENV NVIDIA_DRIVER_CAPABILITIES=${NVIDIA_DRIVER_CAPABILITIES},video

EXPOSE 9000 8554 8889

# ENTRYPOINT ["python3", "main.py"]
ENTRYPOINT ["sleep"]
CMD ["infinity"]