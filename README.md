# cJSON-libFuzzer-testing

 

## Project Overview

This project performs in-process, coverage-guided fuzz testing on cJSON

using LLVM libFuzzer. The primary security focus is memory-safety violations

and crashes triggered by malformed or boundary JSON inputs.

 

## Proposal Scope

- cJSON_Parse()

- cJSON_Print()

- cJSON_PrintUnformatted()

- cJSON_Minify()

 

The strategy uses equivalence partitioning and boundary value analysis.

Initial seeds focus on long strings, deeply nested arrays/objects, and

unusual Unicode input. Main risks include out-of-bounds reads/writes and

use-after-free.

 

## Tools and Environment

- Docker Ubuntu

- Clang / LLVM libFuzzer

- AddressSanitizer (ASan)

- Git and GitHub Issues / Pull Requests

- LLM-assisted seed generation and ASan stack-trace interpretation

 

## Structure

- fuzz/: libFuzzer harness source files

- corpus/: seed inputs for each target

- scripts/: build, fuzzing and coverage scripts

- reports/: experiment records and analysis

- artifacts/: local crash/log artifacts

- third_party/cJSON/: pinned cJSON source

- .github/workflows/: optional GitHub Actions workflows

 

## Build

docker build -t cjson-libfuzzer-testing .

docker run --rm -it -v "${PWD}:/work" -w /work cjson-libfuzzer-testing

cmake -S . -B build -DCMAKE_C_COMPILER=clang

cmake --build build --parallel

 

## Run

./build/fuzz_parse -max_total_time=60 corpus/parse

./build/fuzz_print -max_total_time=60 corpus/print

./build/fuzz_minify -max_total_time=60 corpus/minify

 

fuzz_print must exercise both cJSON_Print() and

cJSON_PrintUnformatted().

 

## Experiment Records

Record target, command, duration, executions, coverage features,

crash findings, sanitizer output, and reproduction status.

Do not report unverified crash counts or coverage percentages.

 

## AI-Assisted Testing

LLM-generated seeds are candidates only. Review, classify and save

reproducible inputs in the appropriate corpus. AI stack-trace analysis

does not replace manual reproduction and root-cause verification.

 

## Git Workflow

Use feature branches and Pull Requests to merge into main.

Track milestones, tasks and confirmed crashes with GitHub Issues.

 

## References

- https://github.com/DaveGamble/cJSON

- https://llvm.org/docs/LibFuzzer.html

- https://clang.llvm.org/docs/AddressSanitizer.html