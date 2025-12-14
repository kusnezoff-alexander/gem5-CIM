/*
 * Copyright (c) 2014 Advanced Micro Devices, Inc.
 * Copyright (c) 2003 The Regents of The University of Michigan
 * All rights reserved.
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

/**
 * @file
 * Definitions of functional page table.
 */
#include "mem/page_table.hh"

#include <string>

#include "base/compiler.hh"
#include "base/trace.hh"
#include "debug/HugePage.hh"
#include "debug/MMU.hh"
#include "sim/faults.hh"
#include "sim/serialize.hh"

namespace gem5
{

void
EmulationPageTable::map(Addr vaddr, Addr paddr, int64_t size, uint64_t flags)
{
    bool clobber = flags & Clobber;
    // starting address must be page aligned
    assert(pageOffset(vaddr) == 0);

    DPRINTF(MMU, "Allocating Page: %#x-%#x\n", vaddr, vaddr + size);

	if(hugePagePoolRange.contains(vaddr)) {
		if (hugePageSize() == 1 << 21) {
			flags = flags | HugePage2MiB;
		} else if (hugePageSize() == 1 << 30) {
			flags = flags | HugePage1GiB;
		}
		mapMultiLevel(vaddr, paddr, size, flags); // WIP: build multi-level pTable in parallel/
												  // separaetely for huge pages for now
		return;
	}

    while (size > 0) {
        auto it = pTable.find(vaddr);
        if (it != pTable.end()) {
            // already mapped
            panic_if(!clobber,
                     "EmulationPageTable::allocate: addr %#x already mapped",
                     vaddr);
            it->second = Entry(paddr, flags);
        } else {
            pTable.emplace(vaddr, Entry(paddr, flags));
        }

        size -= _pageSize;
        vaddr += _pageSize;
        paddr += _pageSize;
    }
}

void
EmulationPageTable::mapMultiLevel(Addr vaddr, Addr paddr, int64_t size, uint64_t flags)
{
    bool clobber = flags & Clobber;
    // starting address must be page aligned
    assert(pageOffset(vaddr) == 0);

    DPRINTF(MMU, "Allocating Page in Multi-Level PTable: %#x-%#x\n", vaddr, vaddr + size);

	auto pageSize = _pageSize; // eg 4KiB is default
	if (flags & HugePage1GiB) {
		pageSize = 1 << 30;
		DPRINTF(HugePage, "Allocating 1GiB Huge Page in Multi-Level PTable: %#x-%#x\n", vaddr, vaddr + size);
	} else if (flags & HugePage2MiB) {
		pageSize = 1 << 21;
		DPRINTF(HugePage, "Allocating 2MiB Huge Page in Multi-Level PTable: %#x-%#x\n", vaddr, vaddr + size);
	}

    while (size > 0) {
        // auto it = pTable.find(vaddr);
        // if (it != pTable.end()) {
        //     // already mapped
        //     panic_if(!clobber,
        //              "EmulationPageTable::allocate: addr %#x already mapped",
        //              vaddr);
        //     it->second = Entry(paddr, flags);
        // } else {
        //     pTable.emplace(vaddr, Entry(paddr, flags));

			// Addresses used to index into corresponding PTables
			// FIXME: are all `uint64_t` for now, but should be smaller (depending on mask size)
			auto pgd_idx = PGD_MASK & vaddr;
			auto p4d_idx= P4D_MASK & vaddr;
			auto pud_idx = PUD_MASK & vaddr;
			auto pmd_idx = PMD_MASK & vaddr;
			auto pte_idx = PTE_MASK & vaddr; // only needed for normal pages

			// Perform Page Walk
			auto p4d = pageTableWalk(&multiLevelPTable.pgd, pgd_idx);
			assert(std::holds_alternative<PTableLevel*>(p4d));
			auto pud = pageTableWalk(std::get<PTableLevel*>(p4d), p4d_idx);
			assert(std::holds_alternative<PTableLevel*>(pud));
			if (flags & HugePage1GiB) {
				// stop here for `1GiB` huge pages
				auto it = std::get<PTableLevel*>(pud)->entries.find(pud_idx);
				if(it !=  std::get<PTableLevel*>(pud)->entries.end())
					DPRINTF(HugePage, "WARNING: Huge Page already mapped in Multi-Level PTable pud: for vaddr=%#x \n", vaddr);

				std::get<PTableLevel*>(pud)->entries[pud_idx] = new Entry(paddr, flags);  // map '1GiB' huge page
				DPRINTF(HugePage, "Mapped Huge Page in Multi-Level PTable pud: %#x-%#x to paddr=0x%x\n", vaddr, vaddr + size, paddr);
			} else {
				auto pmd = pageTableWalk(std::get<PTableLevel*>(pud), pud_idx);
				assert(std::holds_alternative<PTableLevel*>(pmd));
				auto pte = pageTableWalk(std::get<PTableLevel*>(pmd), pmd_idx); // this is only needed for 4KiB pages
				if (flags & HugePage2MiB) {
					// stop here for `2MiB` huge pages
					auto it = std::get<PTableLevel*>(pud)->entries.find(pmd_idx);
					if(it !=  std::get<PTableLevel*>(pmd)->entries.end())
					DPRINTF(HugePage, "WARNING: Huge Page already mapped in Multi-Level PTable pmd: for vaddr=%#x \n", vaddr);

					std::get<PTableLevel*>(pmd)->entries[pmd_idx] = new Entry(paddr, flags); // map '2MiB' huge page
					DPRINTF(HugePage, "Mapped Huge Page in Multi-Level PTable pmd: %#x-%#x to paddr=0x%x\n", vaddr, vaddr + size, paddr);
				} else {

					auto it = std::get<PTableLevel*>(pte)->entries.find(pte_idx);
					if(it !=  std::get<PTableLevel*>(pte)->entries.end())
					DPRINTF(HugePage, "WARNING: Huge Page already mapped in Multi-Level PTable pte: for vaddr=%#x \n", vaddr);

					std::get<PTableLevel*>(pte)->entries[pte_idx] = new Entry(paddr, flags); // map '4KiB' huge page
					DPRINTF(HugePage, "In PTE although huge page?: Mapped Huge Page in Multi-Level PTable pte: %#x-%#x to paddr=0x%x\n", vaddr, vaddr + size, paddr);
				}
			}
        // }

        size -= pageSize;
        vaddr += pageSize;
        paddr += pageSize;
    }
}

std::variant<EmulationPageTable::Entry*, EmulationPageTable::PTableLevel*>
EmulationPageTable::pageTableWalk(PTableLevel* pxd, Addr pxd_vaddr) {
	auto it = pxd->entries.find(pxd_vaddr);
    if (it != pxd->entries.end()) {
        // Entry exists, return the stored variant
        return it->second;
    } else {
		// allocate new one for next level
		auto pd_next = new PTableLevel;
		pxd->entries[pxd_vaddr] = pd_next;
		return pd_next;
	}
}

void
EmulationPageTable::remap(Addr vaddr, int64_t size, Addr new_vaddr)
{
    assert(pageOffset(vaddr) == 0);
    assert(pageOffset(new_vaddr) == 0);

    DPRINTF(MMU, "moving pages from vaddr %08p to %08p, size = %d\n", vaddr,
            new_vaddr, size);

    while (size > 0) {
        [[maybe_unused]] auto new_it = pTable.find(new_vaddr);
        auto old_it = pTable.find(vaddr);
        assert(old_it != pTable.end() && new_it == pTable.end());

        pTable.emplace(new_vaddr, old_it->second);
        pTable.erase(old_it);
        size -= _pageSize;
        vaddr += _pageSize;
        new_vaddr += _pageSize;
    }
}

void
EmulationPageTable::getMappings(std::vector<std::pair<Addr, Addr>> *addr_maps)
{
    for (auto &iter : pTable)
        addr_maps->push_back(std::make_pair(iter.first, iter.second.paddr));

	// TODO: return MutliLevel PageTable mappings
}

void
EmulationPageTable::unmap(Addr vaddr, int64_t size)
{
    assert(pageOffset(vaddr) == 0);

    DPRINTF(MMU, "Unmapping page: %#x-%#x\n", vaddr, vaddr + size);

    while (size > 0) {
        auto it = pTable.find(vaddr);
        assert(it != pTable.end());
        pTable.erase(it);
        size -= _pageSize;
        vaddr += _pageSize;
    }
}

bool
EmulationPageTable::isUnmapped(Addr vaddr, int64_t size)
{
    // starting address must be page aligned
    assert(pageOffset(vaddr) == 0);

    for (int64_t offset = 0; offset < size; offset += _pageSize)
        if (pTable.find(vaddr + offset) != pTable.end())
            return false;

    return true;
}

const EmulationPageTable::Entry *
EmulationPageTable::lookup(Addr vaddr)
{
	// TODO: if in huge page pool: use `multiLevelPTable` to find it
	if (hugePagePoolRange.contains(vaddr)) {

		// DPRINTF(HugePage, "EmulationPageTable::lookup into huge page pool at vaddr=0x%X", vaddr);
		Entry* entry;

		auto pgd_idx = PGD_MASK & vaddr;
		auto p4d_idx= P4D_MASK & vaddr;
		auto pud_idx = PUD_MASK & vaddr;
		auto pmd_idx = PMD_MASK & vaddr;
		auto pte_idx = PTE_MASK & vaddr; // only needed for normal pages

		// Perform Page Walk
		auto p4d = pageTableWalk(&multiLevelPTable.pgd, pgd_idx);
		assert(std::holds_alternative<PTableLevel*>(p4d));
		auto pud = pageTableWalk(std::get<PTableLevel*>(p4d), p4d_idx);
		assert(std::holds_alternative<PTableLevel*>(pud));
		if (std::holds_alternative<Entry*>(pud)) {
			// stop here for `1GiB` huge pages
			entry = std::get<Entry*>(
					std::get<PTableLevel*>(pud)->entries[pud_idx]
			);
		}

		auto pmd = pageTableWalk(std::get<PTableLevel*>(pud), pud_idx);
		assert(std::holds_alternative<PTableLevel*>(pmd));
		auto pte = pageTableWalk(std::get<PTableLevel*>(pmd), pmd_idx); // this is only needed for 4KiB pages
		if (std::holds_alternative<Entry*>(
					std::get<PTableLevel*>(pmd)->entries[pmd_idx]
		)) {
			// stop here for `2MiB` huge pages
			entry = std::get<Entry*>(
				std::get<PTableLevel*>(pmd)->entries[pmd_idx]
			);
		} else if (std::holds_alternative<Entry*>(
		std::get<PTableLevel*>(pmd)->entries[pmd_idx]
		)) {
			DPRINTF(HugePage, "Returning non-huge page although vaddr=0x%x is inside huge page pool??", vaddr);
			entry = std::get<Entry*>(
				std::get<PTableLevel*>(pte)->entries[pte_idx]
			);
		} else {
			return nullptr;
		}

		// QUICKFIX (bc we are not touching `trie` for huge pages I guess?
		// entry->paddr = entry->paddr | (vaddr & hugePageAddrMask);
		DPRINTF(HugePage, "Lookup for huge page at vaddr=0x%X served with paddr=0x%x\n", vaddr, entry->paddr);
		return entry;
	}

    Addr page_addr = pageAlign(vaddr);
    PTableItr iter = pTable.find(page_addr);
    if (iter == pTable.end())
        return nullptr;
    return &(iter->second);
}

bool
EmulationPageTable::translate(Addr vaddr, Addr &paddr)
{
    const Entry *entry = lookup(vaddr);
    if (!entry) {
        DPRINTF(MMU, "Couldn't Translate: %#x\n", vaddr);
        return false;
    }

	if (hugePagePoolRange.contains(vaddr)) {
		paddr = hugePageOffset(vaddr) + entry->paddr;
		DPRINTF(HugePage, "Translating in HugePagePool: %#x->%#x\n", vaddr, paddr);
	}
	else
		paddr = pageOffset(vaddr) + entry->paddr;

    DPRINTF(MMU, "Translating: %#x->%#x\n", vaddr, paddr);
    return true;
}

Fault
EmulationPageTable::translate(const RequestPtr &req)
{
    Addr paddr;
    assert(pageAlign(req->getVaddr() + req->getSize() - 1) ==
           pageAlign(req->getVaddr()));
    if (!translate(req->getVaddr(), paddr))
        return Fault(new GenericPageTableFault(req->getVaddr()));
    req->setPaddr(paddr);
    if ((paddr & (_pageSize - 1)) + req->getSize() > _pageSize) {
        panic("Request spans page boundaries!\n");
        return NoFault;
    }
    return NoFault;
}

void
EmulationPageTable::PageTableTranslationGen::translate(Range &range) const
{
    const Addr page_size = pt->pageSize();

    Addr next = roundUp(range.vaddr, page_size);
    if (next == range.vaddr)
        next += page_size;
    range.size = std::min(range.size, next - range.vaddr);

    if (!pt->translate(range.vaddr, range.paddr))
        range.fault = Fault(new GenericPageTableFault(range.vaddr));
}

void
EmulationPageTable::serialize(CheckpointOut &cp) const
{
    ScopedCheckpointSection sec(cp, "ptable");
    paramOut(cp, "size", pTable.size());

    PTable::size_type count = 0;
    for (auto &pte : pTable) {
        ScopedCheckpointSection sec(cp, csprintf("Entry%d", count++));

        paramOut(cp, "vaddr", pte.first);
        paramOut(cp, "paddr", pte.second.paddr);
        paramOut(cp, "flags", pte.second.flags);
    }
    assert(count == pTable.size());
}

void
EmulationPageTable::unserialize(CheckpointIn &cp)
{
    int count;
    ScopedCheckpointSection sec(cp, "ptable");
    paramIn(cp, "size", count);

    for (int i = 0; i < count; ++i) {
        ScopedCheckpointSection sec(cp, csprintf("Entry%d", i));

        Addr vaddr;
        UNSERIALIZE_SCALAR(vaddr);
        Addr paddr;
        uint64_t flags;
        UNSERIALIZE_SCALAR(paddr);
        UNSERIALIZE_SCALAR(flags);

        pTable.emplace(vaddr, Entry(paddr, flags));
    }
}

const std::string
EmulationPageTable::externalize() const
{
    std::stringstream ss;
    for (PTable::const_iterator it=pTable.begin(); it != pTable.end(); ++it) {
        ss << std::hex << it->first << ":" << it->second.paddr << ";";
    }
    return ss.str();
}

} // namespace gem5
