FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \

    clang llvm lld libclang-rt-dev cmake ninja-build git \

    python3 python3-pip ca-certificates build-essential \

    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

CMD ["/bin/bash"]