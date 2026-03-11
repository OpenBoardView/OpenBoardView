FROM debian:11-slim AS linux-build-env

ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS=yes
RUN apt-get update && \
    apt-get -y install --no-install-recommends ca-certificates g++ make cmake rpm libgtk-3-dev libfontconfig1-dev libsqlite3-dev libglib2.0-dev git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# fixed-minor-version image to avoid force updating when new version is released
# The g++-mingw-w64-i686-posix version 10.3.x is the only version that succeeds crosscompiling openboardview as of 2025.02
FROM ubuntu:jammy-20250714 AS mingw-deb-based-build-env
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS=yes
RUN apt-get update && \
    apt-get -y install --no-install-recommends make cmake g++-mingw-w64-i686-posix pkg-config python3 wget ca-certificates git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
