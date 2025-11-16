#include "mimdram_control_unit.hh"
#include "debug/RowOp.hh"

namespace gem5
{

namespace memory
{

// extracted MIMDRAM code from `MemCtrl::addToWriteQueue` here
void MIMDRAMControlUnit::addToBbopBuffer(PacketPtr pkt, unsigned int pkt_count, MemInterface* mem_intr)
{
	bbopBuffer.push_back(pkt);
}

bool
MIMDRAMControlUnit::bbopBufferFull(unsigned int neededEntries) const
{
    DPRINTF(RowOp, "Bbop Buffer limit %d, current size %d, entries needed %d\n",
            bbopBufferSize, bbopBuffer.size(), neededEntries);

    auto bbopbuffer_size_new = (bbopBuffer.size() + neededEntries);
    return  bbopbuffer_size_new > bbopBufferSize;
}

}

}
