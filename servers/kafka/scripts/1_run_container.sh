#!/usr/bin/env bash
# Run from project root. Redpanda dev container on the project network.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

IMAGE="docker.redpanda.com/redpandadata/redpanda:v24.1.9"
HOST_PORT="${KAFKA_HOST_PORT:-19092}"

mkdir -p "${ROOT}/logs/kafka"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker volume create "${PROJECT_NAME}_kafka_data" >/dev/null
docker run --rm --user 0 \
  -v "${PROJECT_NAME}_kafka_data:/var/lib/redpanda/data" \
  --entrypoint /bin/bash \
  "${IMAGE}" \
  -c 'mkdir -p /var/lib/redpanda/data && chown -R redpanda:redpanda /var/lib/redpanda/data'
docker rm -f "${PROJECT_NAME}_kafka" 2>/dev/null || true

echo "Kafka API: 127.0.0.1:${HOST_PORT}"
echo "Mode:      dev image=${IMAGE}"
echo "Log:       ${ROOT}/logs/kafka/kafka.log"

docker run -d \
  --name "${PROJECT_NAME}_kafka" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}_default" \
  --entrypoint /bin/bash \
  -p "${HOST_PORT}:19092" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${PROJECT_NAME}_kafka_data:/var/lib/redpanda/data" \
  -v "${ROOT}/logs/kafka:/logs/kafka" \
  "${IMAGE}" \
  -c "set -euo pipefail
/usr/bin/rpk redpanda start --kafka-addr internal://0.0.0.0:9092,external://0.0.0.0:19092 --advertise-kafka-addr internal://${PROJECT_NAME}_kafka:9092,external://localhost:${HOST_PORT} --mode dev-container --smp 1 >/dev/null 2>&1 &
until /usr/bin/rpk cluster info >/dev/null 2>&1; do sleep 0.2; done
: > /logs/kafka/kafka.log
declare -A seen=()
while true; do
  while read -r name rest; do
    case \"\$name\" in
      ai_stream2_*)
        if [[ -z \${seen[\$name]+x} ]]; then
          seen[\$name]=1
          stdbuf -oL /usr/bin/rpk topic consume \"\$name\" -o start -f '{\"topic\":\"%t\",\"partition\":%p,\"offset\":%o,\"payload\":%v}\n' >> /logs/kafka/kafka.log &
        fi
        ;;
    esac
  done < <(/usr/bin/rpk topic list)
  sleep 0.5
done"
