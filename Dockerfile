FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

# ------------------------------------------------------------
# Base system + required packages
# ------------------------------------------------------------
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    libyaml-cpp-dev \
    libpng-dev \
    zlib1g-dev \
    libpugixml-dev \
    # wxWidgets build prereqs
    libgtk-3-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------
# Build wxWidgets 3.2.x
# ------------------------------------------------------------
WORKDIR /opt
RUN wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.2.1/wxWidgets-3.2.2.1.tar.bz2 && \
    tar -xf wxWidgets-3.2.2.1.tar.bz2 && \
    cd wxWidgets-3.2.2.1 && \
    mkdir gtkbuild && cd gtkbuild && \
    ../configure --with-gtk --disable-shared && \
    make -j$(nproc) && \
    make install && ldconfig

# ------------------------------------------------------------
# Build yaml-cpp from source
# ------------------------------------------------------------
WORKDIR /opt
RUN git clone https://github.com/jbeder/yaml-cpp.git && \
    cd yaml-cpp && mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && ldconfig

# ------------------------------------------------------------
# Project build environment
# ------------------------------------------------------------
WORKDIR /workspace


# Default command: open a shell
CMD ["/bin/bash"]
