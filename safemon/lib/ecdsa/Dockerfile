FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    libgmp-dev \
    libssl-dev \
    libgtest-dev \
    vim \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /safemon/lib/ecdsa

COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel $(nproc)

CMD ["ctest", "--test-dir", "build", "--output-on-failure"]