FROM ubuntu:24.04

RUN apt-get update -qq && \
    apt-get install -y -qq cmake g++ libgtk-4-dev libsdl2-dev libsdl2-mixer-dev \
      libgdk-pixbuf-2.0-dev libwayland-dev libx11-dev libxtst-dev zstd \
      ninja-build git ca-certificates wget && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Install googletest (for CadGooseTests target)
RUN git clone --depth 1 --branch v1.15.2 https://github.com/google/googletest.git /tmp/googletest && \
    mkdir -p /tmp/googletest/build && \
    cd /tmp/googletest/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_GMOCK=OFF && \
    make -j$(nproc) install && \
    rm -rf /tmp/googletest

WORKDIR /work

WORKDIR /work
