"""
Based on `learning_gem5/part1/simple.py`
"""

# import the m5 (gem5) library created when gem5 is built
import m5

# import all of the SimObjects
from m5.objects import *

# create the system we are going to simulate
system = System()

# Set the clock frequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = "timing"  # Use timing accesses
system.mem_ranges = [AddrRange("512MiB")]  # Create an address range

# Create a simple CPU
# You can use ISA-specific CPU models for different workloads:
# `RiscvTimingSimpleCPU`, `ArmTimingSimpleCPU`.
system.cpu = RiscvTimingSimpleCPU()

# Create a memory bus, a system crossbar, in this case
system.membus = SystemXBar()

# Hook the CPU ports up to the membus
system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports

# create the interrupt controller for the CPU and connect to the membus
system.cpu.createInterruptController()

# Create a DDR3 memory controller and connect it to the membus
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports
# system.mem_ctrl.turnPolicy = QoSTurnaroundPolicyIdeal() # Switch BusState depending on which of ReadQueue/WriteQueue have still elements inside
# system.mem_ctrl.turnPolicy = None

# Connect the system up to the membus
system.system_port = system.membus.cpu_side_ports

# allocate 40MiB of DRAM memory for huge page pool (we will be using for PIM Space)
system.huge_page_pool_base = 0x0F7000000
system.huge_pages_nr = 20
system.huge_page_size = "2MiB"

# Here we set the X86 "hello world" binary. With other ISAs you must specify
# workloads compiled to those ISAs. Other "hello world" binaries for other ISAs
# can be found in "tests/test-progs/hello".
thispath = os.path.dirname(os.path.realpath(__file__))
binary = os.path.join(
    thispath,
    "../tests/cim/src/target/riscv/pim_full_program",
    # "./test"
    # "tests/cim/pim_malloc_syscall",
    # "tests/cim/test_my_add42",
    # "workloads/MIMDRAM-microworkloads/bitweave-buddy_gem5-CIM.exe",
    # "workloads/MIMDRAM-microworkloads/00_addition-baseline_gem5-CIM.exe",
)

system.workload = SEWorkload.init_compatible(binary)

# Create a process for a simple "Hello World" application
process = Process()
# Set the command
# cmd is a list which begins with the executable (like argv)
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
