/*
 * Copyright 2020 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sim/se_workload.hh"

#include "base/intmath.hh"
#include "cpu/thread_context.hh"
#include "debug/HugePage.hh"
#include "debug/RowOp.hh"
#include "params/SEWorkload.hh"
#include "sim/process.hh"
#include "sim/syscall_debug_macros.hh"
#include "sim/system.hh"
#include <chrono>

namespace gem5
{

SEWorkload::SEWorkload(const Params &p, Addr page_shift) :
    Workload(p), memPools(page_shift)
{}

void
SEWorkload::setSystem(System *sys)
{
    Workload::setSystem(sys);

    AddrRangeList memories = sys->getPhysMem().getConfAddrRanges();
    const auto &m5op_range = sys->m5opRange();

    if (m5op_range.valid()) {
        memories -= m5op_range;
	}

	initHugePagePool(sys);

	// hugePagePoolRange = sys->hugePagePoolrange(); // TODO: remove huge page pool logic from system and move into `SEWorkload`?
	memories -= hugePagePoolRange; // reserved for huge page pool !

	// make sure hugePagePoolRange doesn't overlap with any other memPool
	DPRINTF(HugePage, "HugePagePoolRange is [0x%x-0x%x]\n" , hugePagePoolRange.start(), hugePagePoolRange.end());
	for(auto &memory: memories) {
		DPRINTF(HugePage, "MemoryPool in [0x%x-0x%x]\n" , memory.start(), memory.start()+memory.size());
	}

    memPools.populate(memories);
}

void
SEWorkload::initHugePagePool(System *sys)
{
	hugePagePool = MemPool(ceilLog2(system->hugePageSize()), system->hugePagePoolrange().start(), system->hugePagePoolrange().end());
	hugePagesNr = system->hugePagePoolrange().size() / system->hugePageSize();
	_hugePageSize = system->hugePageSize();
		// TODO: no! - this is the <u>physical</u> address range !
	hugePagePoolRange = AddrRange(system->hugePagePoolrange());
}

void
SEWorkload::serialize(CheckpointOut &cp) const
{
    memPools.serialize(cp);
}

void
SEWorkload::unserialize(CheckpointIn &cp)
{
    memPools.unserialize(cp);
}

void
SEWorkload::syscall(ThreadContext *tc)
{
    tc->getProcessPtr()->syscall(tc);
}

Addr
SEWorkload::allocPhysPimHugePages(int npages)
{
    auto pim_paddr = hugePagePool.allocate(npages);

	// auto pim_paddr = hugePagePoolRange.start() + hugePageSize() * huge_page_frame_number;
	DPRINTF(RowOp, "Allocated huge page for PIM with paddr=0x%x\n", pim_paddr);
	return pim_paddr;
}

Addr
SEWorkload::allocPhysPages(int npages, int pool_id)
{
    return memPools.allocPhysPages(npages, pool_id);
}

void
SEWorkload::deallocPhysPage(Addr paddr, int pool_id)
{
	if(hugePagePoolRange.contains(paddr)) {
		hugePagePool.deallocate(paddr, 1);
	} else {
		memPools.deallocPhysPages(paddr, 1, pool_id);
	}
}

Addr
SEWorkload::memSize(int pool_id) const
{
    return memPools.memSize(pool_id) + hugePagePool.totalBytes();
}

Addr
SEWorkload::freeMemSize(int pool_id) const
{
    return memPools.freeMemSize(pool_id) + + hugePagePool.freeBytes();
}

} // namespace gem5
