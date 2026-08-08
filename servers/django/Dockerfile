FROM python:3.12-slim AS base

RUN sed -i 's|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list.d/debian.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends age openssl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV HOST=0.0.0.0
ENV PORT=8000
ENV LOG_ROOT=/root/logs/django

WORKDIR /app

COPY servers/django/requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir -r /tmp/requirements.txt

COPY servers/django/keys/dev.pub /app/keys/dev.pub
COPY servers/django/keys/ticket.pub /app/keys/ticket.pub


FROM base AS dev

RUN mkdir -p /root/logs/django /secrets/age/payloads
COPY servers/django/scripts/docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

EXPOSE 8000
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["gunicorn", "config.wsgi:application", "--bind", "0.0.0.0:8000", "--reload"]


FROM base AS prod

COPY servers/django /app
WORKDIR /app
RUN mkdir -p /root/logs/django /secrets/age/payloads \
    && chmod +x /app/scripts/docker-entrypoint.sh

EXPOSE 8000
ENTRYPOINT ["/app/scripts/docker-entrypoint.sh"]
CMD ["gunicorn", "config.wsgi:application", "--bind", "0.0.0.0:8000"]
