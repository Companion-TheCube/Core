#!/usr/bin/env bash
set -euo pipefail

# ------------------------------------------------------------------------------
# Secure & Minimal Installer
#
# This script installs either development or runtime dependencies in a way
# that is safer, smaller, and more reliable than a plain apt-get call.
#
# Key improvements:
#   • Elevates once at the start instead of sprinkling sudo everywhere.
#     (Fail fast if root privileges aren’t available.)
#   • Forces noninteractive installs so it won’t hang waiting for prompts
#     (important in CI pipelines and image builds).
#   • Uses --no-install-recommends so only explicitly listed packages
#     are installed, avoiding bloat and reducing attack surface.
#   • Adds retry and lock-timeout options so apt is more resilient to
#     flaky mirrors or temporary package database locks.
#   • Separates development dependencies (with compilers/headers)
#     from runtime dependencies (just the libs your program needs).
#   • Cleans apt caches after install to keep the final image smaller
#     and remove unnecessary leftover data.
#
# The result: smaller, more predictable, and more secure system images.
# ------------------------------------------------------------------------------

usage() {
  echo "Usage: $0 [dev|deploy]" >&2
  echo "  dev    Install build-time dependencies (development)" >&2
  echo "  deploy Install runtime dependencies (deployment)" >&2
}

# Show usage if help flag is passed
if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
  usage
  exit 0
fi

# Default mode is "dev" unless explicitly set
MODE=${1:-dev}
if [[ "$MODE" != "dev" && "$MODE" != "deploy" ]]; then
  usage
  exit 1
fi

# Require the script to run as root for all apt operations. If not root, re-execute
# itself with sudo once. This avoids sprinkling sudo throughout and ensures CI
# pipelines fail fast if password prompts would otherwise appear.
if [[ $EUID -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    exec sudo -n --preserve-env=DEBIAN_FRONTEND,PATH "$0" "$@"
  else
    echo "This script must run as root (install sudo or run via su -c)." >&2
    exit 100
  fi
fi

# Force noninteractive apt so that package installs never block on prompts
# (for example, tzdata or locales). This makes the script safe in CI/CD and
# when building images.
export DEBIAN_FRONTEND=noninteractive

# Configure apt to be more reliable and minimal: disable pseudo-TTY output for cleaner logs,
# retry downloads up to three times, wait up to 180 seconds if the package lock is held,
# run updates quietly (only show errors), install only explicitly listed packages without extras,
# and automatically answer “yes” to prompts so the script is noninteractive.
APT_Q="-o=Dpkg::Use-Pty=0 -o=Acquire::Retries=3 -o=DPkg::Lock::Timeout=180"
APT_UPDATE_OPTS="-qq ${APT_Q}"
APT_INSTALL_OPTS="--no-install-recommends -y ${APT_Q}"

# Update package indexes once using the reliability flags
apt-get update ${APT_UPDATE_OPTS}

if [[ "$MODE" == "dev" ]]; then
  # Install full build tools and headers needed for compiling (development mode).
  apt-get install ${APT_INSTALL_OPTS} \
    build-essential cmake pkg-config git python3 \
    libglew-dev libfreetype6-dev libgl1-mesa-dev libglu1-mesa-dev \
    libasound2-dev libpulse-dev \
    libssl-dev libsodium-dev libsqlite3-dev libglm-dev libpq-dev gettext libboost-all-dev \
    libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libxi-dev libudev-dev \
    libopenal-dev libsndfile1-dev libflac-dev libvorbis-dev libogg-dev \
    libjpeg-dev libpng-dev zlib1g-dev nlohmann-json3-dev \
    libsdbus-c++-dev libsystemd-dev
else
  # Install only runtime libraries needed by the application, excluding -dev packages
  # to keep the final image smaller and reduce attack surface.
  apt-get install ${APT_INSTALL_OPTS} \
    ca-certificates \
    libglew2.2 libfreetype6 libgl1 libglu1-mesa \
    libpulse0 \
    libssl3 libsodium23 libsqlite3-0 libpq5 \
    libx11-6 libxrandr2 libxcursor1 libxinerama1 libxi6 libudev1 \
    libopenal1 libsndfile1 libvorbis0a libvorbisfile3 libogg0 \
    libjpeg-turbo8 libpng16-16 zlib1g
fi

# Clean out package caches and apt lists to shrink the image size and
# reduce the amount of leftover data that could be exploited.
apt-get clean
rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

echo "Done: $MODE dependencies installed."