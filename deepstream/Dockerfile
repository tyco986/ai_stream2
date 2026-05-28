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
    confluent-kafka

# Runtime C libs
RUN apt-get update && apt-get install -y --no-install-recommends \
    librdkafka1 \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

ENV NVIDIA_DRIVER_CAPABILITIES=${NVIDIA_DRIVER_CAPABILITIES},video

EXPOSE 9000

# ENTRYPOINT ["python3", "main.py"]
ENTRYPOINT ["sleep"]
CMD ["infinity"]
