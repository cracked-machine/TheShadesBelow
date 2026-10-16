# Default dev image: day-to-day development and the Windows (mingw) cross-build.
# Linux binaries built here need glibc 2.41+; portable Linux release builds use
# ../almalinux8/Dockerfile instead.
FROM debian:trixie

ARG USER=user
ENV DEBIAN_FRONTEND=noninteractive

# general stuff plus mingw cross compiler
RUN apt update -y && apt install -y \
    git \
    build-essential \
    make \
    pkg-config \
    cmake \
    ninja-build \
    clangd \
    clang-format \
    sudo \
    locales \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64 \
    doxygen \
    ccache \
    jq \
    yq \
    python3-pip \
    curl \
    lld

# gcc-mingw-w64's -fuse-ld=lld only searches for a linker named
# <target-triplet>-ld.lld, not the generic ld.lld lld installs - without
# this symlink the flag is silently ignored and bfd ld is used instead.
RUN ln -s /usr/bin/ld.lld /usr/bin/x86_64-w64-mingw32-ld.lld

# SDL3 reqs
RUN apt update -y && apt install -y \
    gnome-desktop-testing \
    libasound2-dev \
    libpulse-dev \
    libaudio-dev \
    libjack-dev \
    libsndio-dev \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxfixes-dev \
    libxi-dev \
    libxss-dev \
    libxtst-dev \
    libxkbcommon-dev \
    libdrm-dev \
    libgbm-dev \
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev \
    libdbus-1-dev \
    libibus-1.0-dev \
    libudev-dev

# SFML3 reqs
RUN apt update -y && apt install -y \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libudev-dev \
    libfreetype-dev \
    libflac-dev \
    libvorbis-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libfreetype-dev

RUN apt update -y && apt install -y glslang-tools

RUN locale-gen en_GB.UTF-8 && update-locale

RUN useradd -m $USER && echo "$USER:$USER" | chpasswd && adduser $USER sudo
RUN echo "user            ALL = (ALL) NOPASSWD: ALL" >> /etc/sudoers

RUN printf "alias ll='ls -la'" >> /home/$USER/.bashrc

CMD /bin/bash