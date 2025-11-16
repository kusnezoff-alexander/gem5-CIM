#include "cim/cim_driver.hh"

#include <memory>

#include "arch/x86/page_size.hh"
#include "base/compiler.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "cpu/thread_context.hh"
#include "debug/CIMDriver.hh"
#include "dev/hsa/hsa_packet_processor.hh"
#include "dev/hsa/kfd_event_defines.h"
#include "dev/hsa/kfd_ioctl.h"
#include "gpu-compute/gpu_command_processor.hh"
#include "gpu-compute/shader.hh"
#include "mem/port_proxy.hh"
#include "mem/se_translating_port_proxy.hh"
#include "mem/translating_port_proxy.hh"
#include "params/CIMDriver.hh"
#include "sim/full_system.hh"
#include "sim/process.hh"
#include "sim/se_workload.hh"
#include "sim/syscall_emul_buf.hh"

namespace gem5
{

CIMDriver::CIMDriver(const Params &p)
    : EmulatedDriver(p), device(p.device), queueId(0),
      eventPage(0), eventSlotIndex(0)
{
    device->attachDriver(this);
    DPRINTF(CIMDriver, "Constructing KFD: device\n");

    // Convert the 3 bit mtype specified in Shader.py to the proper type
    // used for requests.
    std::bitset<MtypeFlags::NUM_MTYPE_BITS> mtype(p.m_type);
    if (mtype.test(MtypeFlags::SHARED)) {
        defaultMtype.set(Request::SHARED);
    }

    if (mtype.test(MtypeFlags::READ_WRITE)) {
        defaultMtype.set(Request::READ_WRITE);
    }

    if (mtype.test(MtypeFlags::CACHED)) {
        defaultMtype.set(Request::CACHED);
    }
}

const char*
CIMDriver::DriverWakeupEvent::description() const
{
    return "DriverWakeupEvent";
}

/**
 * Create an FD entry for the KFD inside of the owning process.
 */
int
CIMDriver::open(ThreadContext *tc, int mode, int flags)
{
    DPRINTF(CIMDriver, "Opened %s\n", filename);
    auto process = tc->getProcessPtr();
    auto device_fd_entry = std::make_shared<DeviceFDEntry>(this, filename);
    int tgt_fd = process->fds->allocFD(device_fd_entry);
    return tgt_fd;
}

/**
 * Currently, mmap() will simply setup a mapping for the associated
 * device's packet processor's doorbells and creates the event page.
 */
Addr
CIMDriver::mmap(ThreadContext *tc, Addr start, uint64_t length,
                       int prot, int tgt_flags, int tgt_fd, off_t offset)
{
    auto process = tc->getProcessPtr();
    auto mem_state = process->memState;

    Addr pg_off = offset >> PAGE_SHIFT;
    Addr mmap_type = pg_off & KFD_MMAP_TYPE_MASK;
    DPRINTF(CIMDriver, "amdkfd mmap (start: %p, length: 0x%x,"
            "offset: 0x%x)\n", start, length, offset);

    switch(mmap_type) {
        case KFD_MMAP_TYPE_DOORBELL:
            DPRINTF(CIMDriver, "amdkfd mmap type DOORBELL offset\n");
            start = mem_state->extendMmap(length);
            process->pTable->map(start, device->hsaPacketProc().pioAddr,
                    length, false);
            break;
        case KFD_MMAP_TYPE_EVENTS:
            DPRINTF(CIMDriver, "amdkfd mmap type EVENTS offset\n");
            panic_if(start != 0,
                     "Start address should be provided by KFD\n");
            panic_if(length != 8 * KFD_SIGNAL_EVENT_LIMIT,
                     "Requested length %d, expected length %d; length "
                     "mismatch\n", length, 8* KFD_SIGNAL_EVENT_LIMIT);
            /**
             * We don't actually access these pages.  We just need to reserve
             * some VA space.  See commit id 5ce8abce for details on how
             * events are currently implemented.
             */
            if (!eventPage) {
                eventPage = mem_state->extendMmap(length);
                start = eventPage;
            }
            break;
        default:
            warn_once("Unrecognized kfd mmap type %llx\n", mmap_type);
            break;
    }

    return start;
}

int
CIMDriver::ioctl(ThreadContext *tc, unsigned req, Addr ioc_buf)
{
	// TODO
    return 0;
}

void
CIMDriver::sleepCPU(ThreadContext *tc, uint32_t milliSecTimeout)
{
    // Convert millisecs to ticks
    Tick wakeup_delay((uint64_t)milliSecTimeout * 1000000000);
    assert(TCEvents.count(tc) == 1);
    TCEvents[tc].timerEvent.scheduleWakeup(wakeup_delay);
    tc->suspend();
    DPRINTF(CIMDriver,
            "CPU %d is put to sleep\n", tc->cpuId());
}

} // namespace gem5
