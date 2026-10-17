FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch

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
        > /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends g++ cmake nlohmann-json3-dev libyaml-cpp-dev libzip-dev unzip ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
