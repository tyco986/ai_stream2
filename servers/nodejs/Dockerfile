FROM node:20-alpine

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.tuna.tsinghua.edu.cn/g' /etc/apk/repositories

ENV HOST=0.0.0.0
ENV PORT=5173
ENV LOG_ROOT=/root/logs/nodejs
ENV PROJECT_NAME=ai_stream2

WORKDIR /app

RUN npm config set registry https://registry.npmmirror.com

COPY package.json package-lock.json* ./
RUN npm install

RUN mkdir -p "${LOG_ROOT}"
