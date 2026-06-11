# Developer Guide

## Table of Contents

- [Prerequisites](#prerequisites)
- [Repository Structure](#repository-structure)
- [AWS Instance Setup](#aws-instance-setup)
- [Scripts Reference](#scripts-reference)
- [Building Images](#building-images)
- [Flashing](#flashing)
- [Device Access](#device-access)
- [Working with the lib/ Layer](#working-with-the-lib-layer)
- [Adding a New Library](#adding-a-new-library)
- [Commit Message Convention](#commit-message-convention)

---

## Prerequisites

- [kas](https://kas.readthedocs.io/) 5.3 -- Yocto build tool
- Docker and Docker Compose plugin -- for lib/ layer tests only
- Git
- For QEMU: `qemu-system-aarch64` installed on WSL2

Install kas:

    pip install kas==5.3

---

## Repository Structure

    safemon-yocto/
    ├── docs/                        -- documentation and architecture diagrams
    ├── meta-safemon/                -- custom Yocto layer
    │   ├── conf/                    -- distro configs (RPi4, Jetson, QEMU)
    │   ├── recipes-bsp/             -- board support patches
    │   ├── recipes-connectivity/    -- Wi-Fi, vcan network config
    │   ├── recipes-kernel/          -- kernel config fragments
    │   └── recipes-safemon/         -- safemon-app and safemon-display recipes
    ├── safemon/                     -- C++ application source
    │   ├── src/                     -- application source files
    │   ├── inc/                     -- headers
    │   ├── lib/                     -- ECDSA library (builds independently)
    │   ├── proto/                   -- gRPC protobuf definitions
    │   └── tools/                   -- Python tools (signing, fault client)
    ├── scripts/                     -- build and test helper scripts
    ├── kas-rpi4.yml                 -- kas build config for Raspberry Pi 4
    └── kas-jetson.yml               -- kas build config for Jetson Orin Nano

---

## AWS Instance Setup

Run once on a fresh Ubuntu 24.04 instance:

    chmod +x scripts/setup-aws.sh
    ./scripts/setup-aws.sh

Then log out and back in for the Docker group change to take effect.

One-time AppArmor fix required on Ubuntu 24.04 before running kas:

    sudo sysctl -w kernel.apparmor_restrict_unprivileged_userns=0

To make it permanent:

    echo 'kernel.apparmor_restrict_unprivileged_userns=0' | sudo tee /etc/sysctl.d/99-bitbake.conf

---

## Scripts Reference

| Script | Purpose |
|--------|---------|
| `scripts/setup-aws.sh` | One-time setup for a fresh Ubuntu 24.04 AWS instance |
| `scripts/cross-compile-rpi4.sh` | Cross-compile a single binary for Raspberry Pi 4 |
| `scripts/cross-compile-jetson.sh` | Cross-compile a single binary for Jetson Orin Nano |

### Cross-compiling a single binary

Use this during development to rebuild and deploy one binary without a full image build.

**Raspberry Pi 4**

    ./scripts/cross-compile-rpi4.sh safemon-app
    ./scripts/cross-compile-rpi4.sh safemon-display

    scp out/safemon-app root@PI_IP:/usr/bin/
    ssh root@PI_IP "systemctl restart safemon-app.service"

**Jetson Orin Nano**

    ./scripts/cross-compile-jetson.sh safemon-app
    ./scripts/cross-compile-jetson.sh safemon-display

    scp out/safemon-app-jetson root@jetson-orin-nano-devkit.local:/usr/bin/safemon-app
    ssh root@jetson-orin-nano-devkit.local "systemctl restart safemon-app.service"

Note: cross-compile requires a completed `kas build` first -- the scripts use the
sysroot from the build directory.

---

## Building Images

**Raspberry Pi 4**

    kas build kas-rpi4.yml

Output: `build/tmp/deploy/images/raspberrypi4-64/`

**Jetson Orin Nano**

    KAS_BUILD_DIR=build-jetson kas build kas-jetson.yml

Output: `build-jetson/tmp/deploy/images/jetson-orin-nano-devkit/`

**QEMU**

    KAS_BUILD_DIR=build-qemu kas build kas-qemu.yml

Output: `build-qemu/tmp/deploy/images/qemuarm64/`

---

## Flashing

### Raspberry Pi 4

Flash over SSH to the USB stick (boot device is `/dev/sda`, not `/dev/mmcblk0`):

    pv image.wic.bz2 | bzcat | ssh root@PI_IP "dd of=/dev/sda bs=4M"

### Jetson Orin Nano

Flashing requires a powered USB hub and WSL2 with usbipd. Put the Jetson into
recovery mode by shorting Pin 10 (REC) to Pin 9 (GND) on the 12-pin header,
then power on, wait 3 seconds, and remove the jumper. The fan must not spin.

    cd ~/tegraflash-native
    sudo BOARDID=3767 BOARDSKU=0005 FAB=000 BOARDREV=J.1 ./doflash.sh

### QEMU

No flashing needed. Download the image from AWS and run locally on WSL2:

    scp user@AWS_IP:~/safemon-yocto/build-qemu/tmp/deploy/images/qemuarm64/Image ~/safemon-qemu/Image
    scp user@AWS_IP:~/safemon-yocto/build-qemu/tmp/deploy/images/qemuarm64/*.ext4 ~/safemon-qemu/rootfs.ext4

    qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a57 \
        -m 2048 \
        -kernel ~/safemon-qemu/Image \
        -drive file=~/safemon-qemu/rootfs.ext4,format=raw,if=virtio \
        -append "root=/dev/vda rw console=ttyAMA0" \
        -device virtio-gpu \
        -display gtk \
        -serial mon:stdio

Login: `root` (no password)

---

## Device Access

### SSH

    ssh root@PI_IP        -- Raspberry Pi 4
    ssh root@jetson-orin-nano-devkit.local    -- Jetson Orin Nano (mDNS)

Find RPi4 IP:

    nmap -sn 192.168.1.0/24

### Service Management

    systemctl status safemon-app.service
    systemctl status safemon-display.service
    journalctl -u safemon-app.service -f

### CAN Bus Testing

    cansend vcan0 123#DEADBEEF      -- known ID, expect OK
    cansend vcan0 999#CAFEBABE      -- unknown ID, expect WARN
    # wait 5 seconds -- expect ERROR:TIMEOUT

### Redis

    redis-cli GET safemon:faults:current
    redis-cli LLEN safemon:can:frames
    redis-cli LINDEX safemon:can:frames 0

### gRPC Fault Client

    cd safemon/tools/fault-client
    python3 fault_client.py --host PI_IP

---

## Working with the lib/ Layer

All reusable libraries live under `safemon/lib/`. They build and test
independently from the Yocto image -- no cross-compiler or device needed.

### Build the Docker image

    cd safemon/lib
    docker compose build

### Run all tests

    docker compose run --rm safemon-dev

### Interactive shell

    docker compose run --rm safemon-dev bash

### Inside the container

    # Re-run all tests
    ctest --test-dir build --output-on-failure

    # Run test binary directly
    ./build/ecdsa/test/ecdsa_tests

    # Run a single test
    ./build/ecdsa/test/ecdsa_tests --gtest_filter=ECDSA.VerifyValidSignature

    # List all tests
    ./build/ecdsa/test/ecdsa_tests --gtest_list_tests

    # Rebuild after editing source
    cmake --build build --parallel

---

## Adding a New Library

1. Create `safemon/lib/<name>/` with its own `CMakeLists.txt`, `inc/`, `src/`, `test/`
2. Add `add_subdirectory(<name>)` to `safemon/lib/CMakeLists.txt`
3. Add any new system dependencies to `safemon/lib/Dockerfile`
4. Rebuild the image: `docker compose build`

---

## Commit Message Convention

    feat:      new feature
    fix:       bug fix
    refactor:  restructuring without behaviour change
    docs:      documentation only
    test:      adding or fixing tests
    chore:     build, tooling, dependencies

One line only. Example: `fix: correct expected value in BigInt.Modulo test`