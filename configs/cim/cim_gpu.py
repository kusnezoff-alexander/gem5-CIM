# CIM GPU config - based on apu_se.py but modified for CIM benchmarks
# Changes: Allow KVM CPU type to avoid libc AVX requirements

import argparse
import os
import sys

# Add gem5 paths
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + "/../..")
from m5.util import addToPath

addToPath("../")

# Import m5 first so we can patch fatal
import m5

# Now import m5.objects to get the namespace
from m5 import objects

# Save original fatal from m5.objects
original_fatal = objects.fatal


def patched_fatal(msg):
    msg_str = str(msg)
    # Allow KVM CPU
    if "GPU model requires X86TimingSimpleCPU or X86O3CPU" in msg_str:
        print("Warning: Overriding CPU type check to allow KVM", file=sys.stderr)
        return
    # Allow non-timing memory mode
    if "Only the timing memory mode is supported" in msg_str:
        print("Warning: Overriding memory mode check to allow KVM", file=sys.stderr)
        return
    original_fatal(msg)


# Patch fatal in m5.objects BEFORE importing apu_se
objects.fatal = patched_fatal

# Also patch in m5.util
if hasattr(m5.util, "fatal"):
    m5.util.fatal = patched_fatal

# Import everything from apu_se.py
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + "/../example")
import apu_se

# Call the main function from apu_se
if __name__ == "__main__":
    apu_se.main()
