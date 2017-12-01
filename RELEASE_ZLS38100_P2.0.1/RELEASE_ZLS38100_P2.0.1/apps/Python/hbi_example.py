from os.path import dirname, realpath
import sys
sys.path.append(dirname(realpath(__file__))+'/../../../libs')
from hbi import *

# Create device config struct
cfg = hbi_dev_cfg_t();

# Init driver and open device
HBI_init(None)
handle = HBI_open(cfg)
print handle

# Read some registers
res = HBI_read(handle, 0x22, 8)
print ["%x"%i for i in res]

# Now open second device
cfg.dev_addr = 1
handle2 = HBI_open(cfg)
print handle2
res = HBI_read(handle2, 0x22, 8)
print ["%x"%i for i in res]

res = HBI_read(handle2, 0x32, 2)
print "Reg 0x32=" 
print ["%x"%i for i in res]
res[1] += 1
HBI_write(handle2, 0x32, res)
res = HBI_read(handle2, 0x32, 2)
print "Reg 0x32=" 
print ["%x"%i for i in res]

HBI_close(handle)
HBI_close(handle2)
HBI_term()
