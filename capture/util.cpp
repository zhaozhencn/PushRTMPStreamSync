#include "util.h"

bool get_qpc_time_ms::inited_ = false;
LARGE_INTEGER get_qpc_time_ms::clock_freq = LARGE_INTEGER();

