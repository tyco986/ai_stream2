FROM node:20-alpine

WORKDIR /app
RUN corepack enable
RUN corepack prepare pnpm@9 --activate

RUN npm config set registry https://registry.npmmirror.com

COPY .npmrc package.json pnpm-lock.yaml ./
RUN pnpm install --frozen-lockfile

EXPOSE 8848

CMD ["pnpm", "dev"]
