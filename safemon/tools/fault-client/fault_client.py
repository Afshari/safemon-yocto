#!/usr/bin/env python3
"""
fault-client -- safemon gRPC fault streaming client.

Connects to safemon-app on the device and prints live fault events.

Usage:
    python3 fault_client.py --host PI_IP
    python3 fault_client.py --host PI_IP --port 50051
    python3 fault_client.py --host PI_IP --verbose
"""

import argparse
import sys
import os
import grpc
from datetime import datetime

# Add proto directory to path
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROTO_DIR  = os.path.join(SCRIPT_DIR, '..', '..', 'proto')
sys.path.insert(0, PROTO_DIR)

import fault_pb2
import fault_pb2_grpc

# ---------------------------------------------------------------------------
# Formatting
# ---------------------------------------------------------------------------

def format_timestamp(ms):
    """Convert millisecond unix timestamp to readable string."""
    dt = datetime.fromtimestamp(ms / 1000.0)
    return dt.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]

def format_event(event, verbose=False):
    ts     = format_timestamp(event.timestamp)
    status = event.status

    # Color code by severity
    if status.startswith('OK'):
        color = '\033[32m'   # green
    elif status.startswith('WARN'):
        color = '\033[33m'   # yellow
    elif status.startswith('ERROR'):
        color = '\033[31m'   # red
    else:
        color = '\033[0m'    # default
    reset = '\033[0m'

    line = f"[{ts}] {color}{status}{reset}"

    if verbose:
        line += f"  source={event.source}"
        if event.signature:
            line += f"  sig={event.signature[:16]}..."

    return line

# ---------------------------------------------------------------------------
# Client
# ---------------------------------------------------------------------------

def run(host, port, verbose):
    address = f"{host}:{port}"
    print(f"[fault-client] Connecting to {address}...")

    channel = grpc.insecure_channel(address)
    stub    = fault_pb2_grpc.FaultServiceStub(channel)

    try:
        for event in stub.StreamFaults(fault_pb2.StreamRequest()):
            print(format_event(event, verbose=verbose))
            sys.stdout.flush()

    except grpc.RpcError as e:
        if e.code() == grpc.StatusCode.UNAVAILABLE:
            print(f"[fault-client] Could not connect to {address} -- is safemon-app running?")
        elif e.code() == grpc.StatusCode.CANCELLED:
            print("[fault-client] Stream cancelled.")
        else:
            print(f"[fault-client] gRPC error: {e.code()} -- {e.details()}")
        sys.exit(1)

    except KeyboardInterrupt:
        print("\n[fault-client] Stopped.")

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='fault-client -- safemon gRPC fault streaming client'
    )
    parser.add_argument('--host',    required=True,
                        help='Device IP address (e.g. 192.168.1.42)')
    parser.add_argument('--port',    default=50051, type=int,
                        help='gRPC port (default: 50051)')
    parser.add_argument('--verbose', action='store_true',
                        help='Show source and signature fields')
    args = parser.parse_args()

    run(args.host, args.port, args.verbose)

if __name__ == '__main__':
    main()