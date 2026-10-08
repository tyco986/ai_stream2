FROM python:3.12-slim AS base

RUN sed -i 's|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list.d/debian.sources
RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple

RUN apt-get update && apt-get install -y --no-install-recommends ffmpeg \
    && rm -rf /var/lib/apt/lists/*

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV HOST=0.0.0.0
ENV PORT=8080

WORKDIR /app

COPY servers/ffmpeg/requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir -r /tmp/requirements.txt


FROM base AS dev

RUN mkdir -p /root/tmp /root/outputs/ffmpeg/capture /root/outputs/nob /root/logs/ffmpeg

EXPOSE 8080
CMD ["python", "main.py", "--host", "0.0.0.0", "--port", "8080"]


FROM base AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        ccache \
        patchelf \
    && rm -rf /var/lib/apt/lists/*

COPY servers/ffmpeg/requirements-nuitka.txt /tmp/requirements-nuitka.txt
RUN pip install --no-cache-dir -r /tmp/requirements-nuitka.txt

COPY servers/ffmpeg /src
WORKDIR /src

RUN python -m nuitka --module --include-package=utils utils \
    && python -m nuitka --module main.py \
    && mkdir -p /out \
    && cp -a main*.so /out/ \
    && cp -a utils*.so /out/


FROM base AS prod

COPY --from=builder /out/ /app/
WORKDIR /app
RUN mkdir -p /root/tmp /root/outputs/ffmpeg/capture /root/outputs/nob /root/logs/ffmpeg

EXPOSE 8080
CMD ["python", "-c", "import main; main.main()"]
