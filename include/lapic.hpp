/*
 * Local Advanced Programmable Interrupt Controller (Local APIC)
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
#include "config.hpp"
#include "memory.hpp"
#include "stdio.hpp"
#include "x86.hpp"

static int ik=0;
class Lapic
{
    private:
        enum Register
        {
            LAPIC_IDR       = 0x2,
            LAPIC_LVR       = 0x3,
            LAPIC_TPR       = 0x8,
            LAPIC_PPR       = 0xa,
            LAPIC_EOI       = 0xb,
            LAPIC_LDR       = 0xd,
            LAPIC_DFR       = 0xe,
            LAPIC_SVR       = 0xf,
            LAPIC_ISR       = 0x10,
            LAPIC_TMR       = 0x18,
            LAPIC_IRR       = 0x20,
            LAPIC_ESR       = 0x28,
            LAPIC_ICR_LO    = 0x30,
            LAPIC_ICR_HI    = 0x31,
            LAPIC_LVT_TIMER = 0x32,
            LAPIC_LVT_THERM = 0x33,
            LAPIC_LVT_PERFM = 0x34,
            LAPIC_LVT_LINT0 = 0x35,
            LAPIC_LVT_LINT1 = 0x36,
            LAPIC_LVT_ERROR = 0x37,
            LAPIC_TMR_ICR   = 0x38,
            LAPIC_TMR_CCR   = 0x39,
            LAPIC_TMR_DCR   = 0x3e,
            LAPIC_IPI_SELF  = 0x3f,
        };

        enum Delivery_mode
        {
            DLV_FIXED       = 0U << 8,
            DLV_NMI         = 4U << 8,
            DLV_INIT        = 5U << 8,
            DLV_SIPI        = 6U << 8,
            DLV_EXTINT      = 7U << 8,
        };

        enum Mask
        {
            UNMASKED        = 0U << 16,
            MASKED          = 1U << 16,
        };

        enum Shorthand
        {
            DSH_NONE        = 0U << 18,
            DSH_EXC_SELF    = 3U << 18,
        };
	
        ALWAYS_INLINE
        static inline uint32 read (Register reg)
        {
            return *reinterpret_cast<uint32 volatile *>(CPU_LOCAL_APIC + (reg << 4));
        }

        ALWAYS_INLINE
        static inline void write (Register reg, uint32 val)
        {
            *reinterpret_cast<uint32 volatile *>(CPU_LOCAL_APIC + (reg << 4)) = val;
        }

        ALWAYS_INLINE
        static inline void set_lvt (Register reg, unsigned vector, Delivery_mode dlv, Mask msk = UNMASKED)
        {
            write (reg, msk | dlv | vector);
        }

        ALWAYS_INLINE
        static inline void timer_handler();

        ALWAYS_INLINE
        static inline void error_handler();

        ALWAYS_INLINE
        static inline void perfm_handler();

        ALWAYS_INLINE
        static inline void therm_handler();

    public:
        static unsigned freq_tsc;
        static unsigned freq_bus;
        static uint8    apic_id[NUM_CPU];

        ALWAYS_INLINE
        static unsigned find_cpu (unsigned apic)
        {
            for (unsigned i = 0; i < NUM_CPU; i++)
                if (apic_id[i] == apic)
                    return i;
				trace (TRACE_CPU, "NUM_CPU=== , %u\n",NUM_CPU);
            return ~0U;
        }

        ALWAYS_INLINE
        static inline unsigned id()
        {
            return read (LAPIC_IDR) >> 24 & 0xff;
        }

        ALWAYS_INLINE
        static inline unsigned version()
        {
            return read (LAPIC_LVR) & 0xff;
        }

        ALWAYS_INLINE
        static inline unsigned lvt_max()
        {
            return read (LAPIC_LVR) >> 16 & 0xff;
        }

        ALWAYS_INLINE
        static inline void eoi()
        {
            write (LAPIC_EOI, 0);
        }

        ALWAYS_INLINE
        static inline void set_timer (unsigned val)
        {//trace (TRACE_CPU,"%" PRIu64 "\n", rdtsc());
	    ik++; 		//trace (TRACE_CPU, "<><><><><>set_timer , i =%d, val-rdtsc()=%lld((val = %u rdtsc=%lld )) \n",ik,val-rdtsc(), val, rdtsc());
            write (LAPIC_TMR_ICR, val);
        }

        ALWAYS_INLINE
        static inline unsigned get_timer()
        {
            return read (LAPIC_TMR_CCR);
        }

        static void init();

        static void send_ipi (unsigned, unsigned, Delivery_mode = DLV_FIXED, Shorthand = DSH_NONE);

        REGPARM (1)
        static void lvt_vector (unsigned) asm ("lvt_vector");

        REGPARM (1)
        static void ipi_vector (unsigned) asm ("ipi_vector");
};
