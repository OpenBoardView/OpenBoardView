# Debian10 provides g++ 8.3.0, SDL2 2.0.9 and produces binaries compatible with Ubuntu 20.04+, RHEL-like 8.+
FROM debian:10.13-slim AS linux-build-env

ARG DEBIAN_FRONTEND=noninteractive DEBCONF_NOWARNINGS=yes
RUN sed "s/deb.debian.org/archive.debian.org/g" -i /etc/apt/sources.list && \
    apt-get update && \
    apt-get -y install --no-install-recommends ca-certificates g++ make cmake rpm libsdl2-dev libgtk-3-dev python3-jinja2 python3-pkg-resources git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# fixed-minor-version image to avoid force updating when new version is released
# The g++-mingw-w64-i686-posix version 10.3.x is the only version that succeeds crosscompiling openboardview as of 2025.02
FROM ubuntu:jammy-20250714 AS mingw-deb-based-build-env
ARG DEBIAN_FRONTEND=noninteractive DEBCONF_NOWARNINGS=yes
RUN apt-get update && \
    apt-get -y install --no-install-recommends make cmake g++-mingw-w64-i686-posix pkg-config python3 python3-jinja2 wget ca-certificates git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Windows build links SDL2 statically, so use latest version.
ENV MINGW_SDL2_VER=2.32.0
RUN wget https://www.libsdl.org/release/SDL2-devel-${MINGW_SDL2_VER}-mingw.tar.gz -O /opt/SDL2-devel-${MINGW_SDL2_VER}-mingw.tar.gz && \
    cd /opt && \
    tar xf SDL2-devel-${MINGW_SDL2_VER}-mingw.tar.gz && \
    make -C SDL2-${MINGW_SDL2_VER} cross CROSS_PATH=/usr && \
    rm -rf /opt/SDL2-devel-${MINGW_SDL2_VER}-mingw.tar.gz /opt/SDL2-${MINGW_SDL2_VER}
