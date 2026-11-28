FROM node:22-bookworm-slim

RUN corepack enable && corepack prepare pnpm@10.33.4 --activate

WORKDIR /app
EXPOSE 5666
CMD ["pnpm", "dev:antd"]
