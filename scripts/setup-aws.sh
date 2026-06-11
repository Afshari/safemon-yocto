#!/bin/bash
# setup_aws.sh - AWS instance setup for safemon development
# Run once on a fresh Ubuntu 24.04 instance
# Usage: chmod +x setup_aws.sh && ./setup_aws.sh

set -e

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
echo "[INFO] Checking Ubuntu version..."
. /etc/os-release
if [[ "$ID" != "ubuntu" ]]; then
    echo "[ERROR] This script is intended for Ubuntu only."
    exit 1
fi
echo "[INFO] Running on Ubuntu $VERSION_ID"

# ---------------------------------------------------------------------------
# System packages
# ---------------------------------------------------------------------------
echo "[INFO] Updating system packages..."
sudo apt-get update
sudo apt-get upgrade -y

echo "[INFO] Installing build tools and utilities..."
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    vim \
    tmux \
    python3 \
    python3-pip \
    python3-venv \
    ca-certificates

# ---------------------------------------------------------------------------
# Docker (official repo - newer than Ubuntu default)
# ---------------------------------------------------------------------------
echo "[INFO] Installing Docker..."
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
    -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] \
https://download.docker.com/linux/ubuntu \
$(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

sudo apt-get update
sudo apt-get install -y \
    docker-ce \
    docker-ce-cli \
    containerd.io \
    docker-compose-plugin \
    docker-buildx-plugin

sudo usermod -aG docker $USER

# ---------------------------------------------------------------------------
# kas (Yocto build tool)
# ---------------------------------------------------------------------------
echo "[INFO] Installing kas..."
pip3 install kas==5.3
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
export PATH=$HOME/.local/bin:$PATH

# ---------------------------------------------------------------------------
# AppArmor fix (required for BitBake on Ubuntu 24.04)
# ---------------------------------------------------------------------------
echo "[INFO] Applying AppArmor fix for BitBake..."
echo 'kernel.apparmor_restrict_unprivileged_userns=0' | sudo tee /etc/sysctl.d/99-bitbake.conf
sudo sysctl -w kernel.apparmor_restrict_unprivileged_userns=0

# ---------------------------------------------------------------------------
# SSH key for GitHub (hint only)
# ---------------------------------------------------------------------------
echo ""
echo "[INFO] SSH key setup for GitHub:"
echo "  1. Generate a key:  ssh-keygen -t ed25519 -C 'aws-safemon'"
echo "  2. Copy public key: cat ~/.ssh/id_ed25519.pub"
echo "  3. Add to GitHub:   Settings -> SSH and GPG keys -> New SSH key"
echo "  4. Test:            ssh -T git@github.com"

# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------
echo ""
echo "[INFO] Verifying installations..."
echo -n "  Docker:         "; docker --version
echo -n "  Docker Compose: "; docker compose version
echo -n "  Python3:        "; python3 --version
echo -n "  cmake:          "; cmake --version | head -1
echo -n "  kas:            "; ~/.local/bin/kas --version

echo ""
echo "[INFO] Setup complete!"
echo "[WARN] Log out and back in for docker group changes to take effect."
echo "[INFO] Then run:"
echo "  git clone git@github.com:Afshari/safemon-yocto.git"
echo "  cd safemon-yocto"
echo "  kas build kas-rpi4.yml"
echo ""
echo "  To build for Jetson:"
echo "  KAS_BUILD_DIR=build-jetson kas build kas-jetson.yml"