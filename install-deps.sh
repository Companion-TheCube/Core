#!/bin/bash
set -euo pipefail

usage() {
  echo "Usage: $0 [dev|deploy]" >&2
  echo "  dev    Install build-time dependencies (development)" >&2
  echo "  deploy Install runtime dependencies (deployment)" >&2
}

if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
  usage
  exit 0
fi

MODE=${1:-dev}
if [[ "$MODE" != "dev" && "$MODE" != "deploy" ]]; then
  usage
  exit 1
fi

sudo apt-get update

if [[ "$MODE" == "dev" ]]; then
  # Development (build) dependencies
  sudo apt-get install -y \
    build-essential cmake pkg-config git python3 \
    libglew-dev libfreetype6-dev libgl1-mesa-dev libglu1-mesa-dev \
    libasound2-dev libpulse-dev \
    libssl-dev libsodium-dev libsqlite3-dev libglm-dev libpq-dev gettext \
    # Windowing/X11 headers used by SFML
    libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libxi-dev libudev-dev \
    # SFML audio stack
    libopenal-dev libsndfile1-dev libflac-dev libvorbis-dev libogg-dev
    # Images and compression (used by SFML/graphics)
    libjpeg-dev libpng-dev zlib1g-dev
else
  # Deployment (runtime) dependencies
  sudo apt-get install -y \
    ca-certificates \
    # Core runtime libs
    libglew2.2 libfreetype6 libgl1 libglu1-mesa \
    libasound2 libpulse0 \
    libssl3 libsodium23 libsqlite3-0 libpq5 \
    # Windowing/X11 runtime used by SFML
    libx11-6 libxrandr2 libxcursor1 libxinerama1 libxi6 libudev1 \
    # SFML audio runtime codecs
    libopenal1 libsndfile1 libflac8 libvorbis0a libvorbisfile3 libogg0
    # Images and compression runtime
    libjpeg-turbo8 libpng16-16 zlib1g
fi

echo "[$MODE] dependency installation complete."
