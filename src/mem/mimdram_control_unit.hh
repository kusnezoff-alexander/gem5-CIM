#pragma once

#include <cstdint>

#include "base/statistics.hh"
#include "mem/qos/mem_ctrl.hh"
#include "params/MemCtrl.hh"

namespace gem5
{

namespace memory
{
/** @brief Control Unit for issuing CIM-Operations (`bbop*`) as described
 * in https://arxiv.org/pdf/2402.19080 Chap4.2
 *
 * The Control Unit is part of the Memory Controller.
 */
class MIMDRAMControlUnit {

	// TODO: make these configurable parameters
	/** Number of processing engines for micro-Programs each of which
	 * can issue CIM-Ops to different mat ranges */
	uint8_t nrProcessingEngines = 4;
	/** Nr of mats managed by the Control Unit (=allocated for CIM) */
	uint16_t nrMats = 128;
	/** Max nr of entries that fit into `bbop_buffer` */
    uint32_t bbopBufferSize = 128;

	/** Stores `bbop*`s dispatched by the host CPU */
	std::vector<PacketPtr> bbopBuffer;
	/** Tracks current mat utilization (whether a mat is currently used by a CIM-Op */
	std::vector<bool> matsScoreboard;

  public:
	/** Adds incoming bbop instruction to the bbop buffer (coming from CPU) */
	void addToBbopBuffer(PacketPtr pkt, unsigned int pkt_count, MemInterface* mem_intr);

	/** Schedules next `bbop*` depending on `bbop`'s range and current mat utilization (see `mat_scoreboard`) */
	void scheduleNextBbop();

	bool bbopBufferFull(unsigned int neededEntries) const;

	MIMDRAMControlUnit(uint16_t nr_mats)
		: nrMats(nr_mats), matsScoreboard(nr_mats, false)
	{}

	MIMDRAMControlUnit(uint8_t nr_processing_engines, uint16_t nr_mats, uint32_t bbopBufferSize)
		: nrProcessingEngines(nr_processing_engines), nrMats(nr_mats), bbopBufferSize(bbopBufferSize),
		matsScoreboard(nr_mats, false)
	{}

	MIMDRAMControlUnit(const MIMDRAMControlUnit &) = default; // copy constructor
    MIMDRAMControlUnit(MIMDRAMControlUnit &&) = default;      // move constructor
};

}
}
