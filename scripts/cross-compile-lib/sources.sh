# Shared source/include variable definitions for cross-compile scripts.
# Paths are relative to the repo root.

ECDSA_SRC="\
  safemon/lib/ecdsa/src/bigint.cpp \
  safemon/lib/ecdsa/src/ec_point.cpp \
  safemon/lib/ecdsa/src/ecdsa.cpp \
  safemon/lib/ecdsa/src/ecdsa_verify_file.cpp"
ECDSA_INC="-I safemon/lib/ecdsa/inc"

CONFIG_SRC="safemon/lib/config/src/config.cpp"
CONFIG_INC="-I safemon/lib/config/inc"

FAULT_SRC="\
  safemon/lib/fault_detector/src/fault_rules.cpp \
  safemon/lib/fault_detector/src/fault_detector.cpp"
FAULT_INC="-I safemon/lib/fault_detector/inc"