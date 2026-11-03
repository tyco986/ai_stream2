#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
GENERATE="$(cd "$(dirname "$0")" && pwd)/3_generate.sh"
TEMPLATES="${ROOT}/servers/generator/templates"

while IFS= read -r yaml; do
  "${GENERATE}" --yaml "${yaml}"
done < <(find "${TEMPLATES}" -type f -name '*.yaml' | sort)
