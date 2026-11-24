#!/usr/bin/env bash
set -euo pipefail

TARGETS=(
  parse
  print
  minify
  boundary
  reference
  memory_structure
  hooks
  create_and_types
  mutate
  compare
)

cmake -S . -B build-cov -DCMAKE_C_COMPILER=clang \
  -DCMAKE_C_FLAGS="-fprofile-instr-generate -fcoverage-mapping" \
  -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate -fcoverage-mapping"
cmake --build build-cov --parallel

mkdir -p artifacts/cov
rm -f artifacts/cov/*.profraw artifacts/cov/merged.profdata

for t in "${TARGETS[@]}"; do
  echo "==> Replaying corpus/${t} on build-cov/fuzz_${t}"
  if [[ "${t}" == "hooks" ]]; then
    # Known upstream cJSON v1.7.19 OOM leak at cJSON.c:2147; disable LSan during coverage replay only (ASan stays active)
    ASAN_OPTIONS=detect_leaks=0 LLVM_PROFILE_FILE="artifacts/cov/${t}.profraw" "./build-cov/fuzz_${t}" -runs=0 -detect_leaks=0 "corpus/${t}"
  else
    LLVM_PROFILE_FILE="artifacts/cov/${t}.profraw" "./build-cov/fuzz_${t}" -runs=0 "corpus/${t}"
  fi
done

llvm-profdata merge -sparse artifacts/cov/*.profraw -o artifacts/cov/merged.profdata

OBJECTS=()
for t in "${TARGETS[@]:1}"; do
  OBJECTS+=("-object=./build-cov/fuzz_${t}")
done

echo ""
echo "==> Coverage Summary (third_party/cJSON/cJSON.c)"
llvm-cov report "./build-cov/fuzz_${TARGETS[0]}" \
  "${OBJECTS[@]}" \
  -instr-profile=artifacts/cov/merged.profdata \
  third_party/cJSON/cJSON.c

llvm-cov show "./build-cov/fuzz_${TARGETS[0]}" \
  "${OBJECTS[@]}" \
  -instr-profile=artifacts/cov/merged.profdata \
  -format=html \
  -output-dir=artifacts/cov/html \
  third_party/cJSON/cJSON.c

echo ""
echo "HTML report written to artifacts/cov/html/index.html"
