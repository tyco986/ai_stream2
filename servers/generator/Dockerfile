FROM python:3.12-slim AS base

RUN sed -i 's|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list.d/debian.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends ripgrep \
    && rm -rf /var/lib/apt/lists/*
RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV HOST=0.0.0.0
ENV PORT=8091
ENV LOG_ROOT=/root/logs/generator

WORKDIR /app

COPY servers/generator/requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir -r /tmp/requirements.txt


FROM base AS dev

RUN mkdir -p /root/logs/generator

EXPOSE 8091
CMD ["python", "main.py"]


FROM dev AS prod

COPY servers/generator /app
WORKDIR /app

EXPOSE 8091
CMD ["python", "main.py"]
