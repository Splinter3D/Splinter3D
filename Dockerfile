FROM debian:trixie-slim AS vcpkg-manifest
COPY ./vcpkg.json /vcpkg.json
RUN sed -Ei 's/"version-semver": *"[^"]*"/"version-semver": "0.0.0"/' /vcpkg.json

FROM debian:trixie-slim

LABEL org.opencontainers.image.source="https://github.com/Splinter3D/Splinter3D"

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake curl git pkg-config \
    python3 python3-pip python3-venv ninja-build g++ \
    libgl1-mesa-dev libglu1-mesa-dev \
    libgl1-mesa-dri libglx-mesa0 xterm linux-libc-dev \
    libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev \
    tar unzip xorg-dev zip libltdl-dev \
    autoconf autoconf-archive automake libtool bison \
    xvfb x11vnc novnc websockify fluxbox x11-utils feh \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

RUN useradd -m -u 1000 -s /bin/bash appuser
RUN chown appuser:appuser /app

COPY --from=vcpkg-manifest /vcpkg.json /app/vcpkg.json
COPY ./vcpkg-configuration.json /app/vcpkg-configuration.json
COPY ./triplets /app/triplets
COPY ./overlay-ports /app/overlay-ports
RUN arch="$(uname -m | sed -e s/x86_64/x64/ -e s/aarch64/arm64/)" && \
    git clone https://github.com/microsoft/vcpkg.git vcpkg && \
    ./vcpkg/bootstrap-vcpkg.sh && \
    ./vcpkg/vcpkg install --triplet="${arch}-linux-release" --x-install-root=build/vcpkg_installed

COPY --chown=appuser:appuser ./src /app/src
COPY --chown=appuser:appuser ./cmake /app/cmake
COPY --chown=appuser:appuser ./assets /app/assets
COPY --chown=appuser:appuser ./scripts /app/scripts
COPY --chown=appuser:appuser ./vcpkg.json /app/vcpkg.json
COPY --chown=appuser:appuser ./CMakeLists.txt /app/CMakeLists.txt

RUN scripts/build.sh --ci --releasemode --release --skip-submodules

COPY ./docker/entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

ENV DISPLAY=:99
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

EXPOSE 6080

USER appuser

ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["build/bin/splinter3D"]
