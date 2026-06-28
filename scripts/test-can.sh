#!/bin/bash
# test-can.sh - send test CAN frames to vcan0

INTERFACE=${1:-vcan0}

echo "[test-can] Sending known frame 0x123..."
cansend $INTERFACE 123#DEADBEEF
sleep 3

echo "[test-can] Sending known frame 0x456..."
cansend $INTERFACE 456#12345678
sleep 3

echo "[test-can] Sending unknown frame 0x999 (triggers WARN)..."
cansend $INTERFACE 999#CAFEBABE
sleep 3

echo "[test-can] Sending known frame 0x789..."
cansend $INTERFACE 789#AABBCCDD
sleep 6

echo "[test-can] Waiting for timeout fault (ERROR after 5s)..."
echo "[test-can] Done."