# safemon-rpi4-yocto

Custom Yocto layer and application for functional safety monitoring
based on ISO 26262, targeting Raspberry Pi 4.

## Components
- `meta-custom/` — Yocto BSP layer (Wi-Fi, PREEMPT_RT kernel, safemon recipe)
- `safemon/` — Functional safety CAN monitor application (C++/Python)

## Target Hardware
- Raspberry Pi 4 Model B
- Yocto Scarthgap 5.0

## Dependencies (clone separately)
- poky (scarthgap)
- meta-raspberrypi (scarthgap)
- meta-openembedded (scarthgap)

## Setup
See docs/ for build instructions.
