# The x86 machine gem5 uses for the Linux 0.11 measurements.
# labs/i386/gem5/launch.sh sets GEM5_ROOT and passes the flags.
# This file builds the machine. It does not choose which kernel path
# to count; launch.sh does that.

import argparse
import os
import sys

import m5
from m5.objects import *
from m5.util import addToPath, fatal

gem5_root = os.environ.get("GEM5_ROOT")
if not gem5_root:
    sys.stderr.write("GEM5_ROOT is not set. Run labs/i386/gem5/launch.sh.\n")
    sys.exit(1)

addToPath(os.path.join(gem5_root, "configs"))

from common import CacheConfig, MemConfig, Options, Simulation
from common.Benchmarks import SysConfig
from common.Caches import IOCache
from common.FSConfig import makeX86System, x86IOAddress

parser = argparse.ArgumentParser(
    description="Build the x86 machine used to measure Linux 0.11."
)
Options.addCommonOptions(parser)
Options.addFSOptions(parser)
parser.add_argument(
    "--timer-interrupt",
    action="store_true",
    help="Start at startup_32. Which path is counted is --count.",
)
parser.add_argument(
    "--count",
    choices=("timer", "schedule", "copy_page"),
    default="timer",
    help="Kernel path to count after startup_32.",
)
args = parser.parse_args()

if not args.kernel:
    fatal("Pass --kernel path/to/tools/system.elf")
if not args.caches:
    fatal("This run needs --caches so the O3 CPU has L1 caches.")

(TestCPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(args)

np = args.num_cpus
test_sys = makeX86System(
    test_mem_mode, np, SysConfig(mem=args.mem_size), X86FsWorkload()
)
test_sys.workload.object_file = os.path.abspath(args.kernel)

test_sys.voltage_domain = VoltageDomain(voltage=args.sys_voltage)
test_sys.clk_domain = SrcClockDomain(
    clock=args.sys_clock, voltage_domain=test_sys.voltage_domain
)
test_sys.cpu_voltage_domain = VoltageDomain()
test_sys.cpu_clk_domain = SrcClockDomain(
    clock=args.cpu_clock, voltage_domain=test_sys.cpu_voltage_domain
)

test_sys.cpu = [
    TestCPUClass(clk_domain=test_sys.cpu_clk_domain, cpu_id=i) for i in range(np)
]
for cpu in test_sys.cpu:
    cpu.createThreads()

# Devices can reach memory. The measured code does not use them.
test_sys.iocache = IOCache(addr_ranges=test_sys.mem_ranges)
test_sys.iocache.cpu_side = test_sys.iobus.mem_side_ports
test_sys.iocache.mem_side = test_sys.membus.cpu_side_ports

CacheConfig.config_cache(args, test_sys)
MemConfig.config_mem(args, test_sys)

if args.timer_interrupt:
    # setup.s has already run on a real boot. Linux 0.11 arms the 8259,
    # not the I/O APIC, and the PIT's IRQ0 is the timer.
    test_sys.workload.command_line = (
        "gem5_boot=startup_32 gem5_count=%s" % args.count
    )
    bridge = test_sys.pc.south_bridge
    # attachIO already ties the PIT to pic1 IRQ0. pic1.output is a
    # source pin, so this adds the CPU's LINT0 beside the I/O APIC.
    bridge.pic1.output = test_sys.cpu[0].interrupts[0].lint0
    # CRT controller ports. The console init pokes them. The timer
    # measurement does not need a screen.
    test_sys.fake_vga = IsaFake(
        pio_addr=x86IOAddress(0x3B0),
        pio_size=0x30,
        ret_data8=0,
        ret_data16=0,
        ret_data32=0,
        ret_data64=0,
    )
    test_sys.fake_vga.pio = test_sys.iobus.mem_side_ports

root = Root(full_system=True, system=test_sys)
Simulation.run(args, root, test_sys, FutureClass)
