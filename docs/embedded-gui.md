# Safemon Embedded GUI

Two OpenGL ES applications render live CAN bus diagnostics directly on the framebuffer — no browser, no app framework, no desktop environment required. Both are built in C++17 using OpenGL ES 3.0 and run identically on all supported targets.

> **Status:** Active development. See [Planned Features](#planned-features) for upcoming work including keyboard input and unified application.

## Table of Contents

- [Safemon Embedded GUI](#safemon-embedded-gui)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Applications](#applications)
    - [safemon-display](#safemon-display)
    - [safemon-chart](#safemon-chart)
  - [Supported Targets](#supported-targets)
  - [Architecture](#architecture)
    - [Rendering Stack](#rendering-stack)
    - [Data Sources](#data-sources)
    - [Text Rendering](#text-rendering)
    - [UI Components](#ui-components)
  - [Running the Applications](#running-the-applications)
    - [Default Service](#default-service)
    - [Switching to safemon-chart](#switching-to-safemon-chart)
  - [Troubleshooting](#troubleshooting)
    - [Display Not Appearing](#display-not-appearing)
    - [APP DOWN Shown](#app-down-shown)
    - [CAN Interface Shows Linked but No Frames](#can-interface-shows-linked-but-no-frames)
    - [Redis Connection Issues](#redis-connection-issues)
    - [vcan0 Commands](#vcan0-commands)
  - [Planned Features](#planned-features)
  - [Known Limitations](#known-limitations)

---

## Overview

The embedded GUI layer consists of two standalone binaries that read live fault data and render it on the device display. They are completely independent of each other and of the core `safemon-app` service.

![Safemon Embedded GUI](assets/embedded_gui.jpg)

---

## Applications

### safemon-display

A glassmorphism-style status panel showing live system health at a glance.

**What it shows:**
- CAN interface status (LINKED / ERROR) with glowing indicator
- Redis connection status (CONNECTED / ERROR)
- Active fault code with color-coded severity and pulse animation:
  - Green -- `OK:NO_FAULT`
  - Yellow -- `WARN:UNKNOWN_ID`
  - Red -- `ERROR:TIMEOUT`
  - Purple -- `APP DOWN` (safemon-app not running)
- Last CAN frame received (ID + bytes + timestamp)
- Frames in queue (Redis list length)
- System uptime (from `/proc/uptime`)
- Build info (machine name + build date from `/etc/safemon-build-info`)

**Data source:** Redis -- polls `safemon:can:frames` and `safemon:faults:current` once per second.

**Default service:** `safemon-display.service` starts automatically at boot.

---

### safemon-chart

A 3D fault waterfall visualization showing fault severity across time and days.

**What it shows:**
- 3D line chart -- X axis: time of day, Z axis: day index, Y axis: fault severity score (0-100)
- Color mapped from dark navy (healthy) to cyan to orange to pink/red (critical)
- Axis gizmo (bottom-left corner) showing X/Y/Z orientation, tracks camera rotation
- Legend: X = Time of Day, Y = Fault Severity, Z = Day (0 = Today)
- Camera navigation via keyboard (WASD + QE)

**Data source:** JSON file at `/etc/safemon/fault_data.json` -- pre-generated simulated fault data. Redis integration planned for a future release.

**Not a default service** -- must be started manually (see [Switching to safemon-chart](#switching-to-safemon-chart)).

---

## Supported Targets

| Target | Display | Status |
|--------|---------|--------|
| Raspberry Pi 4 | DSI LCD (800x480) or HDMI via GBM/DRM | Working |
| Jetson Orin Nano | HDMI via Wayland/Weston | Working |
| QEMU (qemuarm64) | GTK window via virtio-gpu | Working (slower, software GPU) |

---

## Architecture

### Rendering Stack

![Display Pipeline](./assets/display_pipeline.svg)


No windowing system, no compositor (except Jetson which requires Weston). Renders directly to the framebuffer via DRM page flipping on RPi4 and QEMU.

### Data Sources

```
safemon-app  -->  Redis  -->  safemon-display (polls every 1s)
                         -->  (safemon-chart reads JSON file)
```

`safemon-display` depends on `safemon-app` being alive. If `safemon-app` stops, the fault key in Redis expires after ~3 seconds (TTL-based), and the display switches to `APP DOWN` automatically.

### Text Rendering

Both applications use a shared `TextRenderer` class:
- Font: JetBrains Mono (installed at `/etc/safemon/JetBrainsMono-Regular.ttf`)
- Atlas baked at startup using `stb_truetype` (header-only, no system font dependency)
- Multiple sizes baked per session: `eyebrow`, `title`, `body`, `label`, `code`
- Sizes scale with actual screen resolution via `design_scale` factor

### UI Components

`safemon-display` uses a `GlassPanel` class for all card rendering:
- SDF (signed distance field) rounded rectangle shader
- Alpha-blended fill with top-edge sheen
- Glowing border
- Optional pulse animation ring (used on fault banner)
- Circle variant with soft glow (used for status dots)

---

## Running the Applications

### Default Service

`safemon-display` starts automatically at boot via systemd:

```bash
# check status
systemctl status safemon-display

# restart
sudo systemctl restart safemon-display

# view logs
journalctl -u safemon-display -f
```

### Switching to safemon-chart

`safemon-chart` is not managed by systemd. To run it manually:

```bash
# stop the default display service first
sudo systemctl stop safemon-display

# run the chart
/usr/bin/safemon-chart

# when done, restart the display service
sudo systemctl start safemon-display
```

> **Note:** Both applications write to the same display output. Only one can run at a time. Keyboard-based switching between the two is planned
for a future release -- see [Planned Features](#planned-features).

---


## Troubleshooting

### Display Not Appearing

```bash
# check if service is running
systemctl status safemon-display

# check DRM device
ls /dev/dri/
# RPi4: card1   QEMU: card0   Jetson: uses Wayland

# verify config
cat /etc/safemon/safemon.conf
# drm_device must match the correct card for your platform

# check GL version
journalctl -u safemon-display | grep "gl\]"
```

### APP DOWN Shown

`APP DOWN` means `safemon-app` is not running or its Redis key has expired.

```bash
# check safemon-app
systemctl status safemon-app
sudo systemctl start safemon-app

# verify Redis key directly
redis-cli GET safemon:faults:current
# empty or missing = APP DOWN expected
# stale value with app stopped = key has not expired yet (wait ~3s)
```

### CAN Interface Shows Linked but No Frames

The CAN LINKED indicator reflects Redis frame data, not the physical socket state. It can show LINKED even if `vcan0` is down, as long as Redis still has a cached frame. Wait 5+ seconds -- the fault banner will turn red with `ERROR:TIMEOUT` when `FaultDetector` detects no new frames.

```bash
# check vcan0
ip link show vcan0

# verify frames are arriving in Redis
redis-cli LINDEX safemon:can:frames 0
redis-cli LLEN safemon:can:frames
```

### Redis Connection Issues

```bash
# check Redis service
systemctl status redis

# restart Redis
sudo systemctl restart redis

# after Redis restarts, restart both safemon services
sudo systemctl restart safemon-app
sudo systemctl restart safemon-display
```

> **Note:** Auto-reconnect to Redis is not yet implemented. Both `safemon-app` and `safemon-display` must be restarted after Redis recovers from a failure.

### vcan0 Commands

```bash
# check status
ip link show vcan0
systemctl status vcan0

# bring down
sudo ip link set down vcan0
sudo ip link delete vcan0

# bring back up
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
sudo systemctl restart safemon-app

# send test frames (if cansend is available)
cansend vcan0 123#DEADBEEF
cansend vcan0 456#0102030405060708
```

---

## Planned Features

The following features are planned for upcoming development sessions:

| Feature | Priority | Notes |
|---------|----------|-------|
| Keyboard input via evdev | High | Device path from `safemon.conf` |
| Mouse input via evdev | High | Device path from `safemon.conf` |
| Wayland input for Jetson | High | `wl_seat` / `wl_keyboard` / `wl_pointer` |
| Unified application | High | Single binary, switch views via keyboard |
| Auto-reconnect to Redis | Medium | Both `safemon-app` and `safemon-display` |
| Live chart data from Redis | Medium | Replace static JSON with Redis stream |
| Auto-refresh chart | Medium | Reload data without restart |
| Fault detail from FaultDetector | Medium | Authoritative elapsed/last-ID via Redis |
| DisplayStateReader lib class | Medium | Testable Redis polling abstraction |
| Thick line rendering | Low | GLES workaround for `glLineWidth > 1` |
| Dynamic kernel version | Low | Via `uname()` in footer |

---

## Known Limitations

- **Two separate binaries** -- `safemon-display` and `safemon-chart` cannot run simultaneously. Switching requires stopping one service and starting the other manually. Keyboard-based switching is planned.

- **No Redis auto-reconnect** -- if Redis restarts, both `safemon-app` and `safemon-display` must be restarted manually to reconnect.

- **Chart uses static data** -- `safemon-chart` reads from a pre-generated JSON file, not live Redis data. This will be addressed in a future release.

- **QEMU performance** -- rendering on QEMU uses a software GPU (virtio-gpu), which is noticeably slower than real hardware. Timer updates may lag by ~1-2 seconds. This is expected behavior, not a bug.

- **Jetson circle artifact** -- a subtle transparency artifact appears around status dot circles on Jetson when connected to a large HDMI display. Under investigation.

- **CAN LINKED indicator** -- reflects Redis cache state, not physical socket state. Shows LINKED for up to 5 seconds after `vcan0` goes down, until `FaultDetector` writes `ERROR:TIMEOUT`.