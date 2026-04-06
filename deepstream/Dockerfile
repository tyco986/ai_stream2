FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch

# China mirror acceleration (DeepStream image is Ubuntu-based)
RUN sed -i 's|archive.ubuntu.com|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|security.ubuntu.com|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list
RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple

# Install pyservicemaker (wheel ships inside the container) + Python deps
RUN pip install --break-system-packages \
    /opt/nvidia/deepstream/deepstream/service-maker/python/pyservicemaker*.whl \
    pyyaml \
    confluent-kafka \
    pynvml

# Kafka protocol adapter + GStreamer RTSP Server bindings
RUN apt-get update && apt-get install -y \
    librdkafka-dev \
    libgstrtspserver-1.0-dev \
    gir1.2-gstrtspserver-1.0 \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN mkdir -p /app/recordings/rolling /app/recordings/locked /app/screenshots

ENV NVIDIA_DRIVER_CAPABILITIES=${NVIDIA_DRIVER_CAPABILITIES},video

EXPOSE 9000 8554

ENTRYPOINT ["python3", "main.py"]
