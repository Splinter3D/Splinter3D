FROM debian:trixie-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake curl git pkg-config \
    python3 python3-pip python3-venv ninja-build g++ \
    libgl1-mesa-dev libglu1-mesa-dev \
    libgl1-mesa-dri libglx-mesa0 xterm \
    libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev \
    tar unzip xorg-dev zip libltdl-dev \
    autoconf autoconf-archive automake libtool \
    xvfb x11vnc novnc websockify fluxbox x11-utils feh \
    && rm -rf /var/lib/apt/lists/*

COPY ./src /app/src
COPY ./.git /app/.git
COPY ./cmake /app/cmake
COPY ./locale /app/locale
COPY ./assets /app/assets
COPY ./include /app/include
COPY ./scripts /app/scripts
COPY ./vcpkg.json /app/vcpkg.json
COPY ./CMakeLists.txt /app/CMakeLists.txt
WORKDIR /app

RUN useradd -m -u 1000 -s /bin/bash appuser
RUN chown -R appuser:appuser /app

RUN git config --global --add safe.directory /app
RUN scripts/build.sh --ci --debug-build

COPY ./docker/entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

ENV DISPLAY=:99
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

EXPOSE 6080

USER appuser

ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["./splinter3D"]
