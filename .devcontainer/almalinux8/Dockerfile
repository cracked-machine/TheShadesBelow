# Portable Linux release image: glibc 2.28 (RHEL8+, Ubuntu 20.04+, Debian 10+,
# Steam Deck) with GCC 14 from gcc-toolset-14. Linux binaries built here run on
# any distro with glibc >= 2.28. Also usable as a devcontainer for troubleshooting
# old-glibc issues. The Windows (mingw) cross-build stays on ../debian-trixie.
FROM almalinux:8

ARG USER=user
ARG CMAKE_VERSION=3.31.6

# EPEL (ccache, glslang) + PowerTools (ninja-build, some -devel packages)
RUN dnf install -y dnf-plugins-core epel-release && \
    dnf config-manager --set-enabled powertools && \
    dnf clean all

# general stuff plus GCC 14 toolset
RUN dnf install -y \
    gcc-toolset-14-gcc \
    gcc-toolset-14-gcc-c++ \
    gcc-toolset-14-libstdc++-devel \
    git \
    make \
    ninja-build \
    pkgconf-pkg-config \
    ccache \
    binutils \
    which \
    tar \
    gzip \
    sudo \
    clang-tools-extra \
    jq \
    python39 \
    python39-pip \
    glibc-langpack-en \
    && dnf clean all

# EL8 ships CMake 3.26; the project needs >= 3.28
RUN curl -fsSL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
    | tar -xz --strip-components=1 -C /usr/local

# python yq (provides `yq -y`, same as Debian's yq package) used by scripts/build.sh
RUN pip3.9 install --no-cache-dir yq

# SFML3 reqs
RUN dnf install -y \
    libX11-devel \
    libXext-devel \
    libXrandr-devel \
    libXcursor-devel \
    libXfixes-devel \
    libXi-devel \
    libxkbcommon-devel \
    libdrm-devel \
    mesa-libgbm-devel \
    mesa-libGL-devel \
    mesa-libEGL-devel \
    systemd-devel \
    freetype-devel \
    alsa-lib-devel \
    pulseaudio-libs-devel \
    && dnf clean all

# GLSL linter used by .vscode/settings.json
# RUN dnf install -y glslang-devel && dnf clean all

# Make GCC 14 the default gcc/g++ (what `scl enable gcc-toolset-14` would do)
ENV PATH=/opt/rh/gcc-toolset-14/root/usr/bin:$PATH

# scripts/build.sh appends this to the build dir name so builds from this image
# (build-<target>-el8-<type>) never collide with the trixie image's build dirs
ENV BUILD_DIR_SUFFIX=-el8

ENV LANG=en_GB.UTF-8

RUN useradd -m $USER && echo "$USER:$USER" | chpasswd && usermod -aG wheel $USER
RUN echo "user            ALL = (ALL) NOPASSWD: ALL" >> /etc/sudoers

RUN printf "alias ll='ls -la'" >> /home/$USER/.bashrc

CMD /bin/bash
