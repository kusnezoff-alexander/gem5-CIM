set debuginfod enabled on
# # break src/cpu/simple/exec_context.hh:185
# break gem5::X86ISAInst::ROWAND::execute
# # break gem5::BaseMMU::translateTiming
# b addToWriteQueue
# b processRefreshEvent
# b processPowerEvent
# b processWriteDoneEvent
# b checkRefreshState
# b checkDrainDone
# # b doBurstAccess if mem_pkt->addr == 0xb0
# b doBurstAccess
#
# b aapBank
# b apBank
#
# # only interested in what's happening after RowOp
define run_rowand
	run
	disable breakpoints
	tbreak ROWNOT::execute
	continue
	enable breakpoints
end

define print_data
	print *reinterpret_cast<Request::RowOpPayload*>(data)
end

define trans_data
	if !req->isRowOp()
		echo No RowOP!\n
	end
	set $data = ((gem5::DataTranslation<gem5::TimingSimpleCPU*>*)translation)->state->data
	print *reinterpret_cast<Request::RowOpPayload*>($data)
end

define state_data
	# if &req && !req->isRowOp()
	# 	echo No RowOP!\n
	# end
	print *reinterpret_cast<Request::RowOpPayload*>(state->data)
end

# b processRefreshEvent
#
#
# break Decoder::decode
# commands
#   silent
#   printf "modRM: %d\n", mach_inst.modRM
#   continue
# end

# CURRENTLY
# b CmdHandlers.cc:102

# CURRENTLY 2
# b timing.cc:588
# this is where the assert `mem_pkt->rank == mem_pkt1->rank` fails
# b mem_ctrl.cc:329
#b DRAMInterface::decodePacket if pkt->isRowOp()
#b addToWriteQueue if pkt->isRowOp()

# check if dst,src1,src2 arrive correctly

# b TimingSimpleCPU::writeMem

# break on all subsequent function calls

# b doBurstAccess
# b MemCtrl::addToWriteQueue
# b DRAMInterface::decodePacket
# b MemCtrl::recvTimingReq
# b MemoryPort::recvTimingReq
# b TimingRequestProtocol::sendReq
# b RequestPort::sendTimingReq
# b CoherentXBar::recvTimingReq
# b CoherentXBar::CoherentXBarResponsePort::recvTimingReq
# b TimingRequestProtocol::sendReq
# b RequestPort::sendTimingReq
# b TimingSimpleCPU::handleWritePacket
b TimingSimpleCPU::sendData
commands
	bt 5
	print_data
	continue
end
b TimingSimpleCPU::finishTranslation
commands
	bt 5
	state_data
	finish
	state_data
	continue
end
b DataTranslation::finish
commands
	bt 5
	state_data
	finish
	state_data
	continue
end
b WholeTranslationState::finish
commands
	bt 5
	print_data
	echo Faults
	print faults
	echo Given fault
	print fault->name()
	if sreqDest->hasPaddr()
		printf "Dest: %d, ", sreqDest->getPaddr()
	end
	if sreqSrc1->hasPaddr()
		printf "Src1: %d, ", sreqSrc1->getPaddr()
	end
	if sreqSrc2->hasPaddr()
		printf "Src2: %d", sreqSrc2->getPaddr()
	end
	continue
end
b WholeTranslationState::getFault
commands
	bt 5
	print_data
	continue
end
b X86ISA::TLB::translateTiming
commands
	bt 5
	trans_data
	if req->isRowOp()
		continue
	end
end
b BaseMMU::translateTiming
commands
	bt 5
	trans_data
	if req->isRowOp()
		continue
	end
end
b TimingSimpleCPU::writeMem
commands
	print *reinterpret_cast<Request::RowOpPayload*>(data)
	continue
end

# Error observed here:
# set $data_error = *reinterpret_cast<Request::RowOpPayload*>(state->data)
# b TimingSimpleCPU::finishTranslation if $data_error.dest == 22672
b TimingSimpleCPU::finishTranslation if (*reinterpret_cast<Request::RowOpPayload*>(state->data)).dest == 22672
# state is deleted here!
b timing.cc:697

run_rowand
# watch *(Request::RowOpPayload*)0x50f34a0 if *(Request::RowOpPayload*)0x50f0430 == 22672
# watch *(Request::RowOpPayload*)0x50f34a0 if *(Request::RowOpPayload*)0x50f0430 != 22672

# watch *(Request::RowOpPayload*)0x50f34a0
# commands
#   printf "Value changed to %d at 0x50f34a0 \n", *(Request::RowOpPayload*)0x50f34a0
#   continue
# end

# `finish()` calls `finishTranslation()`:
# break translation.hh:336
# commands
#   state_data
#   continue
# end

b TLB::translate if req->hasVaddr() && req->getVaddr() == 0x9678901

# make sure pages are allocated correctly after `mmap()`-calls
# b Process::allocateMem

# debug why 3rd operand isn't translated for 2nd ROW-Op in a row
break timing.cc:564
commands
	# print the variable
    print split_addr
	print flags._flags & (int)Request::ROWOP
	# resume execution automatically
    continue
end
