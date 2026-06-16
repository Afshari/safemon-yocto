# Developer Guide

## Table of Contents

- [Developer Guide](#developer-guide)
  - [Table of Contents](#table-of-contents)
  - [Prerequisites](#prerequisites)
  - [Repository Structure](#repository-structure)
  - [AWS Instance Setup](#aws-instance-setup)
  - [Scripts Reference](#scripts-reference)
    - [Cross-compiling a single binary](#cross-compiling-a-single-binary)
  - [Building Images](#building-images)
  - [Useful kas / BitBake Commands](#useful-kas--bitbake-commands)
    - [Interactive shell inside the build environment](#interactive-shell-inside-the-build-environment)
    - [Rebuild a single recipe](#rebuild-a-single-recipe)
    - [Inspect a recipe](#inspect-a-recipe)
    - [Check recipe build status](#check-recipe-build-status)
    - [View build logs for a failed task](#view-build-logs-for-a-failed-task)
    - [Clean and rebuild from scratch](#clean-and-rebuild-from-scratch)
  - [Flashing](#flashing)
    - [Raspberry Pi 4](#raspberry-pi-4)
    - [Jetson Orin Nano](#jetson-orin-nano)
    - [QEMU](#qemu)
  - [Device Access](#device-access)
    - [SSH](#ssh)
    - [Copying Files to a Device](#copying-files-to-a-device)
    - [Service Management](#service-management)
    - [CAN Bus Testing](#can-bus-testing)
    - [Redis](#redis)
    - [gRPC Fault Client](#grpc-fault-client)
  - [Platform Configuration Files](#platform-configuration-files)
  - [Working with the lib/ Layer](#working-with-the-lib-layer)
    - [Build the Docker image](#build-the-docker-image)
    - [Run all tests](#run-all-tests)
    - [Interactive shell](#interactive-shell)
    - [Inside the container](#inside-the-container)
  - [Adding a New Library](#adding-a-new-library)
  - [Commit Message Convention](#commit-message-convention)

---

## Prerequisites

- [kas](https://kas.readthedocs.io/) 5.3 -- Yocto build tool
- Docker and Docker Compose plugin -- for lib/ layer tests only
- Git
- For QEMU: `qemu-system-aarch64` installed on WSL2 (Windows 11) or Linux

Install kas:

    pip install kas==5.3

For QEMU on Windows 11, install WSL2 with Ubuntu and then:

    sudo apt install qemu-system-aarch64

WSLg is included by default on Windows 11 and provides the display window for QEMU.

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
    ├── kas-jetson.yml               -- kas build config for Jetson Orin Nano
    └── kas-qemu.yml                 -- kas build config for QEMU (qemuarm64)

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
| `scripts/fetch_yocto_image.sh` | Download the latest RPi4 image from the build server |
| `scripts/fetch_qemu_image.sh` | Download the latest QEMU kernel and rootfs from the build server |
| `scripts/start-qemu.sh` | Launch the QEMU VM with display and SSH port forwarding |

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

The QEMU build uses `distro: safemon-qemu` which differs from the RPi4 distro -- it
excludes Wi-Fi, uses Mesa virgl instead of V3D, and installs a QEMU-specific
`safemon.conf` with `drm_device=/dev/dri/card0`. See
[Platform Configuration Files](#platform-configuration-files) for details.

---

## Useful kas / BitBake Commands

### Interactive shell inside the build environment

    kas shell kas-rpi4.yml

This drops you into a BitBake shell with all environment variables set --
useful for running `bitbake` commands directly.

### Rebuild a single recipe

    kas shell kas-rpi4.yml -c "bitbake safemon-app"

Force a clean rebuild of just that recipe:

    kas shell kas-rpi4.yml -c "bitbake safemon-app -c cleansstate"
    kas shell kas-rpi4.yml -c "bitbake safemon-app"

### Inspect a recipe

Show all variables and their final computed values for a recipe:

    kas shell kas-rpi4.yml -c "bitbake -e safemon-app"

Show only a specific variable (e.g. SRC_URI):

    kas shell kas-rpi4.yml -c "bitbake -e safemon-app" | grep ^SRC_URI=

Show the dependency tree for a recipe:

    kas shell kas-rpi4.yml -c "bitbake -g safemon-app"

This generates `task-depends.dot`, `pn-depends.dot`, and `pn-buildlist`
in the current directory.

### Check recipe build status

    kas shell kas-rpi4.yml -c "bitbake-layers show-recipes safemon-app"

### View build logs for a failed task

If a task fails, BitBake prints the log path. To find it manually:

    find build/tmp/work -path "*safemon-app*/temp/log.do_compile*"

### Clean and rebuild from scratch

    kas shell kas-rpi4.yml -c "bitbake safemon-app -c cleanall"
    kas build kas-rpi4.yml

---

## Flashing

After a successful build, find the image with:

    ls -t build*/tmp/deploy/images/*/*.wic.bz2 build*/tmp/deploy/images/*/*.tegraflash.tar.gz 2>/dev/null | head -1

### Raspberry Pi 4

The image is located at:

    build/tmp/deploy/images/raspberrypi4-64/core-image-base-raspberrypi4-64.rootfs-<datetime>.wic.bz2

This is a compressed disk image. Flash it to a microSD card or USB stick using any of the following tools:

**Linux**

Decompress and write directly with `bzcat` and `dd` (replace `/dev/sdX` with your target device -- double-check with `lsblk` first):

    bzcat core-image-base-raspberrypi4-64.rootfs-<datetime>.wic.bz2 | sudo dd of=/dev/sdX bs=4M status=progress conv=fsync

Or use a graphical tool such as [GNOME Disks](https://wiki.gnome.org/Apps/Disks) or [Balena Etcher](https://etcher.balena.io/) (cross-platform).

**Windows**

Decompress the `.bz2` file (e.g. with [7-Zip](https://www.7-zip.org/)) to get the `.wic` file, then flash it with [Win32 Disk Imager](https://sourceforge.net/projects/win32diskimager/) or [Balena Etcher](https://etcher.balena.io/).

**macOS**

[Balena Etcher](https://etcher.balena.io/).

After flashing, insert the card/stick into the Raspberry Pi 4 and power on.

### Jetson Orin Nano

The image is located at:

    build-jetson/tmp/deploy/images/jetson-orin-nano-devkit/core-image-base-jetson-orin-nano-devkit.rootfs-<datetime>.tegraflash.tar.gz

Extract the archive, then flash with `tegraflash` and a powered USB hub. Put the Jetson into recovery mode by shorting Pin 10 (REC) to Pin 9 (GND) on the 12-pin header, then power on, wait 3 seconds, and remove the jumper. The fan must not spin.

    tar xf core-image-base-jetson-orin-nano-devkit.rootfs-<datetime>.tegraflash.tar.gz
    sudo BOARDID=3767 BOARDSKU=0005 FAB=000 BOARDREV=J.1 ./doflash.sh

### QEMU

No flashing needed. The image runs directly as a virtual machine.

**Step 1 -- Download the image** (run in WSL2):

    ./scripts/fetch_qemu_image.sh

This downloads the kernel and rootfs to `~/safemon-qemu/` and renames them to
`Image` and `rootfs.ext4`.

**Step 2 -- Launch QEMU** (run in WSL2):

    ./scripts/start-qemu.sh

Or manually:

    qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a57 \
        -m 2048 \
        -kernel ~/safemon-qemu/Image \
        -drive file=~/safemon-qemu/rootfs.ext4,format=raw,if=virtio \
        -append "root=/dev/vda rw console=ttyAMA0" \
        -device virtio-gpu \
        -display gtk \
        -serial mon:stdio \
        -netdev user,id=net0,hostfwd=tcp::2222-:22 \
        -device virtio-net-pci,netdev=net0

A GTK window opens showing the display output. The serial console is available
in the same terminal. Login: `root` (no password).

**Notes:**
- `-device virtio-gpu` creates `/dev/dri/card0` inside the guest -- required for DRM/KMS
- `-display gtk` renders the framebuffer via WSLg on Windows 11
- `-netdev ... hostfwd=tcp::2222-:22` forwards host port 2222 to guest SSH port 22
- The image files must be in the WSL2 native filesystem (`~/`), not on a Windows
  drive (`/mnt/d/`), to avoid severe I/O performance degradation

**Important -- QEMU-specific config files:**

The QEMU image uses `drm_device=/dev/dri/card0` in `safemon.conf` (RPi4 uses
`card1`). This is baked in at build time -- do not copy an RPi4 config to a QEMU
image or vice versa. If you need to replace the config file on a running QEMU
instance, re-sign it with the correct key and copy both files. See
[Platform Configuration Files](#platform-configuration-files).

---

## Device Access

### SSH

    ssh root@PI_IP                          -- Raspberry Pi 4
    ssh root@jetson-orin-nano-devkit.local  -- Jetson Orin Nano (mDNS)
    ssh -p 2222 root@localhost              -- QEMU (WSL2, port forwarded)

Find RPi4 IP:

    nmap -sn 192.168.1.0/24

### Copying Files to a Device

**Raspberry Pi 4 / Jetson**

    scp file root@PI_IP:/destination/path/

**QEMU** (note the uppercase -P flag for scp):

    scp -P 2222 file root@localhost:/destination/path/

Example -- update config on a running QEMU instance:

    scp -P 2222 safemon-qemu.conf root@localhost:/etc/safemon/safemon.conf
    scp -P 2222 safemon-qemu.conf.sig root@localhost:/etc/safemon/safemon.conf.sig
    ssh -p 2222 root@localhost "systemctl restart safemon-display.service"

### Service Management

    systemctl status safemon-app.service
    systemctl status safemon-display.service
    journalctl -u safemon-app.service -f
    journalctl -u safemon-display.service --no-pager -n 50

### CAN Bus Testing

    cansend vcan0 123#DEADBEEF      -- known ID, expect OK
    cansend vcan0 999#CAFEBABE      -- unknown ID, expect WARN
    # wait 5 seconds -- expect ERROR:TIMEOUT

On QEMU, vcan0 is set up automatically at boot by the vcan service. If it is
missing, check that the kernel was built with the vcan config fragment:

    modprobe vcan
    ip link add dev vcan0 type vcan
    ip link set up vcan0

### Redis

    redis-cli GET safemon:faults:current
    redis-cli LLEN safemon:can:frames
    redis-cli LINDEX safemon:can:frames 0

### gRPC Fault Client

    cd safemon/tools/fault-client
    python3 fault_client.py --host PI_IP

For QEMU, use `localhost` as the host (add a port forward to the QEMU launch
command if needed):

    python3 fault_client.py --host localhost

---

## Platform Configuration Files

Each platform has its own `safemon.conf` with platform-specific settings.
The correct file is selected automatically at build time by the recipe.

| Setting | Raspberry Pi 4 | QEMU | Jetson Orin Nano |
|---------|----------------|------|-----------------|
| `drm_device` | `/dev/dri/card1` | `/dev/dri/card0` | N/A (Wayland) |
| Config file in recipe | `safemon.conf` | `safemon-qemu.conf` | `safemon.conf` |

The config file is ECDSA-signed. Both `safemon.conf` and `safemon.conf.sig`
must always be updated together. To generate a new signed config:

    cd safemon/tools
    python3 sign_config.py safemon-qemu.conf safemon-qemu.conf.sig

After signing, copy both files to the device (see
[Copying Files to a Device](#copying-files-to-a-device)).

---

## Working with the lib/ Layer

All reusable libraries live under `safemon/lib/`. They build and test
independently from the Yocto image -- no cross-compiler or device needed.

Current libraries:

| Library | Description |
|---------|-------------|
| `ecdsa` | secp256k1 ECDSA sign/verify |
| `config` | Parses `safemon.conf` into a `SafemonConfig` struct |
| `fault_detector` | Fault evaluation rules (timeout, unknown ID) and Redis-backed detector |

### Build the Docker image

    cd safemon/lib
    docker compose build

### Run all tests

    docker compose run --rm safemon-dev

### Interactive shell

    docker compose run --rm safemon-dev bash

### Inside the container

    # Build (first time, or after CMakeLists changes)
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel $(nproc)

    # Re-run all tests
    ctest --test-dir build --output-on-failure

    # Run tests for one library only
    ctest --test-dir build --output-on-failure -R config
    ctest --test-dir build --output-on-failure -R fault
    ctest --test-dir build --output-on-failure -R ecdsa

    # List all discovered tests without running them
    ctest --test-dir build -N

    # Run a test binary directly for full gtest output
    ./build/ecdsa/test/ecdsa_tests
    ./build/config/test/config_tests
    ./build/fault_detector/test/fault_detector_tests

    # Run a single test by name
    ./build/ecdsa/test/ecdsa_tests --gtest_filter=ECDSA.VerifyValidSignature

    # List all tests in a binary
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