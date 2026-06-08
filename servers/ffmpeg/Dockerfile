FROM python:3.12-slim

RUN sed -i 's|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list.d/debian.sources
RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple

RUN apt-get update && apt-get install -y --no-install-recommends ffmpeg \
    && rm -rf /var/lib/apt/lists/*

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
# Container paths; 2_build_container.sh mounts host dirs onto these:
#   attachments/videos -> /app/video
#   outputs/ffmpeg     -> /app/output
#   logs/ffmpeg        -> /app/log
ENV OUTPUT_ROOT=/app/output
ENV LOG_ROOT=/app/log
ENV VIDEO_ROOT=/app/video
ENV VIDEOS_ROOT=/app/videos
ENV MEDIAMTX_HOST=ai_stream2_mediamtx
ENV MEDIAMTX_RTSP_PORT=8554

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY . .
RUN mkdir -p /app/output /app/log /app/videos

EXPOSE 8080
CMD ["python", "main.py", "--host", "0.0.0.0", "--port", "8080"]
