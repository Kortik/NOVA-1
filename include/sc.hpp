/*
 * Scheduling Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#include "compiler.hpp"

class Ec;
struct Mys;
class Sc : public Kobject
{
    friend class Queue<Sc>;

    public:
        Refptr<Ec> const ec;
        unsigned const cpu;
        unsigned const prio;
        unsigned const budget;
        uint64 time;

    private:
        unsigned left;
        Sc *prev, *next;
        uint64 tsc;

        

        static Slab_cache cache;

        static struct Rq {
            Spinlock    lock;
            Sc *        queue;
        } rq CPULOCAL;
	
        static unsigned prio_top CPULOCAL;

        void ready_enqueue (uint64);
        void ready_dequeue (uint64);

	static Mys *getMys(Sc *);

    public:
        static Sc *     current     CPULOCAL_HOT;
        static unsigned ctr_link    CPULOCAL;
        static unsigned ctr_loop    CPULOCAL;
	static Mys * 	current_mys;
        static unsigned const default_prio = 1;
        static unsigned const default_quantum = 10000;
	static unsigned const priorities = 128;
	static Sc *list[priorities] CPULOCAL;
        Sc (Pd *, mword, Ec *);
        Sc (Pd *, mword, Ec *, unsigned, unsigned, unsigned);

        ALWAYS_INLINE
        static inline Rq *remote (unsigned long c)
        {
            return reinterpret_cast<typeof rq *>(reinterpret_cast<mword>(&rq) - CPU_LOCAL_DATA + HV_GLOBAL_CPUS + c * PAGE_SIZE);
        }

        void remote_enqueue();

        static void rrq_handler();
        static void rke_handler();

        NORETURN
        static void schedule (bool = false);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};

class Mys {
	 public:
	
	    Pd		*current_pd;
	  //  Sc  	*prev, *next;
	    Mys  	*next_mys, *root_mys, *prev_mys;
	    //Refptr<Pd>  current_pd;
	    Sc *list[Sc::priorities];
	    unsigned prio_top=0;
	    long long pd_budget;
	    Mys(){};
	static Slab_cache cache;
	 ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
	};
