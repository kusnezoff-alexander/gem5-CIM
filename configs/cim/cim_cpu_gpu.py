"""
Config for CPU and GPU loads (no PIM/CIM)
Based on MIMDRAM Paper, Table 2
"""

# import the m5 (gem5) library created when gem5 is built
import m5
import optparse
import shlex

# import all of the SimObjects
from m5.objects import *


def addOptions(parser):
    parser.add_option(
        "--cmd",
        type="string",
        default="",
        help="Command to execute (use quotes for args)",
    )


# Define cache classes inline for self-contained configuration
# Specs: L1 32KB 8-way 64B, L2 256KB 4-way 64B
class L1ICache(Cache):
    size = "32KiB"
    assoc = 8
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 16
    tgts_per_mshr = 32
    write_buffers = 400

    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port

    def connectBus(self, bus):
        self.mem_side = bus.cpu_side_ports


class L1DCache(Cache):
    size = "32KiB"
    assoc = 8
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 16
    tgts_per_mshr = 32
    write_buffers = 400

    def connectCPU(self, cpu):
        self.cpu_side = cpu.dcache_port

    def connectBus(self, bus):
        self.mem_side = bus.cpu_side_ports


class L2Cache(Cache):
    size = "256KiB"
    assoc = 4
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    mshrs = 32
    tgts_per_mshr = 24
    write_buffers = 400

    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports


# create the system we are going to simulate
system = System()

# Set the clock frequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = "timing"  # Use timing accesses
system.mem_ranges = [AddrRange("512MiB")]  # Create an address range

# Create a simple CPU
# You can use ISA-specific CPU models for different workloads:
# `RiscvTimingSimpleCPU`, `ArmTimingSimpleCPU`.
system.cpu = X86TimingSimpleCPU()
# system.cpu = X86O3CPU()           # Unfortunately this doesn't work yet

# Create an L1 cache
system.l1icache = L1ICache()
system.l1icache.connectCPU(system.cpu)

system.l1dcache = L1DCache()
system.l1dcache.connectCPU(system.cpu)

# Create a memory bus, a system crossbar, in this case
system.l2bus = SystemXBar()
system.l1icache.connectBus(system.l2bus)
system.l1dcache.connectBus(system.l2bus)

# Create an L2 cache
system.l2cache = L2Cache()
system.l2cache.connectCPUSideBus(system.l2bus)

# Create a memory bus for L2 to memory
system.membus = SystemXBar()
system.l2cache.connectMemSideBus(system.membus)

# Hook the CPU ports up to the L1 caches (already connected above)
# system.cpu.icache_port = system.membus.cpu_side_ports
# system.cpu.dcache_port = system.membus.cpu_side_ports

# create the interrupt controller for the CPU and connect to the membus
system.cpu.createInterruptController()

# For X86 only we make sure the interrupts care connect to memory.
# Note: these are directly connected to the memory bus and are not cached.
# For other isa you should remove the following three lines.
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

# Create a DDR4-2400 memory controller and connect it to the membus
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR4_2400_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.dram.addr_mapping = "RaBaMaRoCh"
system.mem_ctrl.port = system.membus.mem_side_ports
# system.mem_ctrl.turnPolicy = QoSTurnaroundPolicyIdeal() # Switch BusState depending on which of ReadQueue/WriteQueue have still elements inside
# system.mem_ctrl.turnPolicy = None

# Connect the system up to the membus
system.system_port = system.membus.cpu_side_ports

# allocate 40MiB of DRAM memory for huge page pool (we will be using for PIM Space)
# system.huge_page_pool_base = 0x0F7000000
system.huge_page_pool_base = 0x10000000
system.huge_pages_nr = 20
system.huge_page_size = "2MiB"


# Here we set the X86 "hello world" binary. With other ISAs you must specify
# workloads compiled to those ISAs. Other "hello world" binaries for other ISAs
# can be found in "tests/test-progs/hello".
parser = optparse.OptionParser()
addOptions(parser)
(options, args) = parser.parse_args()

thispath = os.path.dirname(os.path.realpath(__file__))
if options.cmd:
    binary = options.cmd
else:
    binary = os.path.join(
        thispath,
        "../../",
        # "tests/cim/pim_malloc_syscall",
        # "tests/cim/src/pim_full_program",
        "tests/test-progs/cim/bin/pim_test_primitives",
        # "tests/cim/bin/pim_workloads",
        # "tests/cim/bin/pim_test_pimmalloc",
        # "tests/cim/bin/combined_knn",
        # "tests/test-progs/cim/bin/hello_world",
    )

system.workload = SEWorkload.init_compatible(binary)

# Create a process for a simple "Hello World" application
process = Process()
# Set the command
# cmd is a list which begins with the executable (like argv)
if options.cmd:
    process.cmd = shlex.split(options.cmd)
else:
    process.cmd = [binary]

# make vaddr=paddr (1:1 mapping)
# m5.instantiate()
# process.map(vaddr= system.aimc_ctrl.pio_addr, paddr= system.aimc_ctrl.pio_addr, size=system.aimc_ctrl.pio_size, cacheable=False) # TODO make this work with dram mem_ctrl

# Set the cpu to use the process as its workload and create thread contexts
system.cpu.workload = process
system.cpu.createThreads()

# set up the root SimObject and start the simulation
root = Root(full_system=False, system=system)
# instantiate all of the objects we've created above
m5.instantiate()

print(f"Beginning simulation!")
exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
