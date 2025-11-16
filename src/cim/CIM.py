from m5.citations import add_citation
from m5.defines import buildEnv
from m5.objects.Bridge import Bridge
from m5.objects.ClockedObject import ClockedObject
from m5.objects.Device import DmaVirtDevice
from m5.objects.LdsState import LdsState
from m5.objects.Process import EmulatedDriver
from m5.objects.VegaGPUTLB import VegaPagetableWalker
from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject

class CIMDriver(EmulatedDriver):
    type = "CIMDriver"
    cxx_class = "gem5::CIMDriver"
    cxx_header = "cim/cim_driver.hh"
    device = Param.CIMMemoryDevice("Memory Device controlled by this driver")
