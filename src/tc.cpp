/*
#include "lapic.hpp"
#include "stdio.hpp"
#include "vectors.hpp"
INIT_PRIORITY (PRIO_SLAB)
Slab_cache Tc::cache (sizeof (Tc), 32);

Tc *Tc::instance = 0;
Tc *Tc::getInstance() {
        
	if (!Tc::instance) Tc::instance = new Tc;
        return Tc::instance;
    }

static unsigned long Tc::GetTimePd(int num) { return pd[num]; }
static unsigned long Tc::GetTimeSc(int num) { if (num>=2) return pd[2]; else return pd[num]; }//*/
