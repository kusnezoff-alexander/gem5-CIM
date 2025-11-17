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
 * Declarations of a non-full system Page Table.
 */

#ifndef __MEM_PAGE_TABLE_HH__
#define __MEM_PAGE_TABLE_HH__

#include <string>
#include <unordered_map>
#include <variant>

#include "base/addr_range.hh"
#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "base/types.hh"
#include "mem/request.hh"
#include "mem/translation_gen.hh"
#include "sim/serialize.hh"

namespace gem5
{

class ThreadContext;

class EmulationPageTable : public Serializable
{
  public:
    struct Entry
    {
        Addr paddr;
        uint64_t flags;

        Entry(Addr paddr, uint64_t flags) : paddr(paddr), flags(flags) {}
        Entry() {}
    };

	// /** A single PTableLevel is responsible for */
	// struct _PTableLevel;
	// // maps addr to next PTableLevel (_PTableLevel==null for last level)
	// typedef std::unordered_map<Addr, _PTableLevel*> PTableLevel;
	struct PTableLevel {
		// map virtual addresses to child levels (nullptr at leaves)
		std::unordered_map<Addr, std::variant<Entry*, PTableLevel*>> entries;
	};

	/**
	 * see  [Linux Kernel Docs: Page Tables](https://docs.kernel.org/mm/page_tables.html)
	 *
	 * This table does NOT implement: Folding
	 */
	struct MultiLevelPTable
	{
		PTableLevel pgd;
	};

  protected:

    typedef std::unordered_map<Addr, Entry> PTable;
    typedef PTable::iterator PTableItr;
    PTable pTable;
	// WIP: this is going to replace `pTable`
	MultiLevelPTable multiLevelPTable;
	const uint64_t PGD_SHIFT = 39;
	const uint64_t PGD_MASK  = 0x1FFULL << PGD_SHIFT;   // bits 47:39
	const uint64_t P4D_SHIFT = 39;
	const uint64_t P4D_MASK  = 0x1FFULL << P4D_SHIFT;   // bits 47:39 (folded)
	const uint64_t PUD_SHIFT = 30;
	const uint64_t PUD_MASK  = 0x1FFULL << PUD_SHIFT;   // bits 38:30 (contains 2^30=1 GiB huge page level)
	const uint64_t PMD_SHIFT = 21;
	const uint64_t PMD_MASK  = 0x1FFULL << PMD_SHIFT;   // bits 29:21 (contains 2^21=2 MiB huge page level)
	const uint64_t PTE_SHIFT = 12;
	const uint64_t PTE_MASK  = 0x1FFULL << PTE_SHIFT;   // bits 20:12 (contains 2^12=4 KiB pages)
	const uint64_t PAGE_4K_OFFSET = 0xFFFULL;           // bits 11:0
	const uint64_t PAGE_2M_OFFSET = 0x1FFFFFULL;        // bits 20:0
    const Addr _pageSize;
    const Addr offsetMask;
    const Addr _hugePageSize;
    const Addr hugePageOffsetMask;
	const AddrRange hugePagePoolRange;

    const uint64_t _pid;
    const std::string _name;

  public:

    EmulationPageTable(
            const std::string &__name, uint64_t _pid, Addr _pageSize) :
            _pageSize(_pageSize), offsetMask(mask(floorLog2(_pageSize))),
			_hugePageSize(0), hugePageOffsetMask(0),
            _pid(_pid), _name(__name), shared(false)
    {
        assert(isPowerOf2(_pageSize));
    }
    EmulationPageTable(
            const std::string &__name, uint64_t _pid, Addr _pageSize, Addr _hugePageSize,
			AddrRange hugePagePoolRange) :
            _pageSize(_pageSize), offsetMask(mask(floorLog2(_pageSize))),
			_hugePageSize(_hugePageSize), hugePageOffsetMask(mask(floorLog2(_hugePageSize))),
			hugePagePoolRange(hugePagePoolRange),
            _pid(_pid), _name(__name), shared(false)
    {
        assert(isPowerOf2(_pageSize));
        assert(isPowerOf2(_hugePageSize));
    }

    uint64_t pid() const { return _pid; };

    virtual ~EmulationPageTable() {};

    /* generic page table mapping flags
     *              unset   | set
     * bit 0 - no-clobber   | clobber
     * bit 2 - cacheable    | uncacheable
     * bit 3 - read-write   | read-only
     * bit 4 - 4KiB or 1GiB | 2MiB Pages
	 * bit 5 - 4KiB or 2Mib | 1GiB Pages
     */
    enum MappingFlags : uint32_t
    {
        Clobber     	= 1,
        Uncacheable 	= 4,
        ReadOnly    	= 8,
        HugePage2MiB    = 16,
        HugePage1GiB	= 32,
    };

    // flag which marks the page table as shared among software threads
    bool shared;

    virtual void initState() {};

    // for DPRINTF compatibility
    const std::string name() const { return _name; }

    Addr pageAlign(Addr a)  { return (a & ~offsetMask); }
    Addr pageOffset(Addr a) { return (a &  offsetMask); }
    Addr hugePageAlign(Addr a)  { return (a & ~hugePageOffsetMask); }
    Addr hugePageOffset(Addr a) { return (a &  hugePageOffsetMask); }
    // Page size can technically vary based on the virtual address, but we'll
    // ignore that for now.
    Addr pageSize()   { return _pageSize; }
    Addr hugePageSize()   { return _hugePageSize; }

    /**
     * Maps a virtual memory region to a physical memory region.
     * @param vaddr The starting virtual address of the region.
     * @param paddr The starting physical address where the region is mapped.
     * @param size The length of the region.
     * @param flags Generic mapping flags that can be set by or-ing values
     *              from MappingFlags enum.
     */
    virtual void map(Addr vaddr, Addr paddr, int64_t size, uint64_t flags = 0);
    virtual void remap(Addr vaddr, int64_t size, Addr new_vaddr);
    virtual void unmap(Addr vaddr, int64_t size);

	/** Map huge pages into pTable (NOTE: this is a diry workaround to support huge pages */
	virtual void mapMultiLevel(Addr vaddr, Addr paddr, int64_t size, uint64_t flags);
	// perform pageTableWalk onto next level `pxd` with given (masked) address `pxd_addr`
	std::variant<Entry*, PTableLevel*> pageTableWalk(PTableLevel* pxd, Addr pxd_addr);

    /**
     * Check if any pages in a region are already allocated
     * @param vaddr The starting virtual address of the region.
     * @param size The length of the region.
     * @return True if no pages in the region are mapped.
     */
    virtual bool isUnmapped(Addr vaddr, int64_t size);

    /**
     * Lookup function
     * @param vaddr The virtual address.
     * @return The page table entry corresponding to vaddr.
     */
    const Entry *lookup(Addr vaddr);

    /**
     * Translate function
     * @param vaddr The virtual address.
     * @param paddr Physical address from translation.
     * @return True if translation exists
     */
    bool translate(Addr vaddr, Addr &paddr);

    /**
     * Simplified translate function (just check for translation)
     * @param vaddr The virtual address.
     * @return True if translation exists
     */
    bool
    translate(Addr vaddr)
    {
        Addr dummy;
        return translate(vaddr, dummy);
    }

    class PageTableTranslationGen : public TranslationGen
    {
      private:
        EmulationPageTable *pt;

        void translate(Range &range) const override;

      public:
        PageTableTranslationGen(EmulationPageTable *_pt, Addr vaddr,
                Addr size) : TranslationGen(vaddr, size), pt(_pt)
        {}
    };

    TranslationGenPtr
    translateRange(Addr vaddr, Addr size)
    {
        return TranslationGenPtr(
                new PageTableTranslationGen(this, vaddr, size));
    }

    /**
     * Perform a translation on the memory request, fills in paddr
     * field of req.
     * @param req The memory request.
     */
    Fault translate(const RequestPtr &req);

    /**
     * Dump all items in the pTable, to a concatenation of strings of the form
     *    Addr:Entry;
     */
    const std::string externalize() const;

    void getMappings(std::vector<std::pair<Addr, Addr>> *addr_mappings);

    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;
};

} // namespace gem5

#endif // __MEM_PAGE_TABLE_HH__
