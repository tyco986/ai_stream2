FROM node:20-alpine AS base

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.tuna.tsinghua.edu.cn/g' /etc/apk/repositories

ENV HOST=0.0.0.0
ENV LOG_ROOT=/root/logs/nodejs
ENV PROJECT_NAME=ai_stream2

WORKDIR /app

RUN npm config set registry https://registry.npmmirror.com

COPY package.json package-lock.json* ./
RUN npm install

RUN mkdir -p "${LOG_ROOT}"


FROM base AS dev

ENV PORT=5173
COPY scripts/dev-both.sh /app/scripts/dev-both.sh
RUN chmod +x /app/scripts/dev-both.sh

EXPOSE 5173 5174
CMD ["npm", "run", "dev"]


FROM base AS prod

ENV PORT=5173
COPY . /app
RUN chmod +x /app/scripts/dev-both.sh \
    && npm run build

EXPOSE 5173
CMD ["npm", "run", "preview"]
