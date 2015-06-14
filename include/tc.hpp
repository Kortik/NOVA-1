/*#pragma once
#include "compiler.hpp"

// static Tc *Tc::instance;
class Tc
{

private: 
	static Slab_cache cache;
    Tc() {};
    
    static Tc * instance;
    unsigned long pd[10] = {50000,20000,10000,20000,10,10,10,10,10,10};
    unsigned long sc[2] = {100000,10000}; 

public:
  
   ALWAYS_INLINE
    static inline void *operator new (size_t) { return cache.alloc(); }
   ALWAYS_INLINE
    static inline void operator delete (void *ptr) { cache.free (ptr); }
    static Tc *getInstance();/* {
        
	if (!instance) instance = new Tc;
        return instance;
    }//
    unsigned long GetTimePd(int num);// { return pd[num]; }
    unsigned long GetTimeSc(int num);// { if (num>=2) return pd[2]; else return pd[num]; }

  
}; 
/*
INIT_PRIORITY (PRIO_SLAB)
Slab_cache Tc::cache (sizeof (Tc), 32);
INIT_PRIORITY (PRIO_LOCAL)*/
