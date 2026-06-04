# Developer Guide

## Prerequisites

- Docker (installed via `scripts/setup-aws.sh` or manually)
- Docker Compose plugin (`docker compose` not `docker-compose`)
- Git

---

## Repository structure

```
safemon-yocto/
├── docs/                        ← you are here
├── scripts/
│   └── setup-aws.sh             ← one-time AWS instance setup
├── meta-safemon/                ← Yocto layer (recipes, distro config)
└── safemon/
    ├── CMakeLists.txt           ← top-level app build
    ├── src/                     ← application source (safemon-app, display, etc.)
    ├── inc/                     ← application headers
    └── lib/                     ← reusable libraries (independent of Yocto)
        ├── CMakeLists.txt
        ├── Dockerfile
        ← docker-compose.yml
        └── ecdsa/               ← secp256k1 ECDSA library
            ├── CMakeLists.txt
            ├── inc/
            ├── src/
            └── test/
```

---

## AWS instance setup

Run once on a fresh Ubuntu 24.04 instance:

```bash
chmod +x scripts/setup-aws.sh
./scripts/setup-aws.sh
```

Then log out and back in for the Docker group change to take effect.

---

## Working with the lib/ layer

All reusable libraries live under `safemon/lib/`. They build and test
independently from the Yocto image — no cross-compiler or Pi needed.

### Build the Docker image

```bash
cd safemon/lib
docker compose build
```

### Run all tests

```bash
docker compose run --rm safemon-dev
```

### Interactive shell

```bash
docker compose run --rm safemon-dev bash
```

### Inside the container — useful commands

```bash
# Re-run all tests
ctest --test-dir build --output-on-failure

# Run test binary directly (full gtest output)
./build/ecdsa/test/ecdsa_tests

# Run a single test
./build/ecdsa/test/ecdsa_tests --gtest_filter=ECDSA.VerifyValidSignature

# List all tests
./build/ecdsa/test/ecdsa_tests --gtest_list_tests

# Rebuild after editing source
cmake --build build --parallel
```

---

## Adding a new library

1. Create `safemon/lib/<name>/` with its own `CMakeLists.txt`, `inc/`, `src/`, `test/`
2. Add `add_subdirectory(<name>)` to `safemon/lib/CMakeLists.txt`
3. Add any new system dependencies to `safemon/lib/Dockerfile`
4. Rebuild the image: `docker compose build`

---

## Branches

| Branch | Purpose |
|--------|---------|
| `main` | stable, all merged features |
| `feature/ecdsa-grpc` | current — ECDSA library + gRPC fault streaming |

---

## Commit message convention

```
feat:     new feature
fix:      bug fix
refactor: restructuring without behaviour change
docs:     documentation only
test:     adding or fixing tests
chore:    build, tooling, dependencies
```

One line only. Example: `fix: correct expected value in BigInt.Modulo test`