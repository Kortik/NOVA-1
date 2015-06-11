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

#include "ec.hpp"
#include "lapic.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Sc::cache (sizeof (Sc), 32);
Slab_cache Mys::cache (sizeof (Mys), 32);
INIT_PRIORITY (PRIO_LOCAL)
Sc::Rq Sc::rq;

static unsigned counter = 0;

Sc *        Sc::current;
unsigned    Sc::ctr_link;
unsigned    Sc::ctr_loop;
Mys *       Sc::current_mys=0;
Sc *Sc::list[priorities];

unsigned Sc::prio_top;

Sc::Sc (Pd *own, mword sel, Ec *e) : Kobject (SC, static_cast<Space_obj *>(own), sel, 0x1), ec (e), cpu (static_cast<unsigned>(sel)), prio (0), budget (Lapic::freq_bus * 1000), left (0)
{
    trace (0, "SC:%p created (PD:%p Kernel)", this, own);
}

Sc::Sc (Pd *own, mword sel, Ec *e, unsigned c, unsigned p, unsigned q) : Kobject (SC, static_cast<Space_obj *>(own), sel, 0x1), ec (e), cpu (c), prio (p), budget (Lapic::freq_bus / 1000 * q), left (0)
{
    trace (0, "SC:%p created (EC:%p CPU:%#x P:%u Q:%#x)", this, e, c, p, q);
}

Mys *Sc::getMys(Sc *sc){
  Mys *tmp = current_mys->root_mys;
  do {
    //trace(0,"current_pd=%d get_pd()=%d",tmp->current_pd->getMyNum(), this->ec->get_pd()->getMyNum() );
    if(tmp->current_pd->getMyNum()==sc->ec->get_pd()->getMyNum()) {return tmp;}
    else tmp=tmp->next_mys;
  }
  while (tmp!=current_mys->root_mys);
}

void Sc::ready_enqueue (uint64 t)
{
    assert (prio < priorities);
    assert (cpu == Cpu::id);
  if (!current_mys) {
    Mys *cur = new Mys;
    cur->current_pd=this->ec->get_pd();
    cur->next_mys=cur->root_mys=cur=cur->prev_mys=cur;
    cur->list[priorities];
    //cur->next=cur->prev=this;
    cur->pd_budget = 100000;
    cur->prio_top = 0;
    current_mys=cur;
  }
  else {
    Mys *tmp = current_mys->root_mys;
    //int flag =0;
    do {
      if(tmp->current_pd->getMyNum()==this->ec->get_pd()->getMyNum()) {/*trace(0, "exit");*/tmp = 0; break;}
      else tmp=tmp->next_mys;
    }
    while (tmp!=current_mys->root_mys);
    if (tmp) { //Mys dlya dannogo PD ne sushestvuet
      Mys *cur1 = new Mys;
      cur1->current_pd=this->ec->get_pd();
      cur1->next_mys=current_mys->root_mys;
      cur1->root_mys=current_mys->root_mys;
      cur1->prev_mys=current_mys->root_mys->prev_mys;
      current_mys->root_mys->prev_mys = cur1;
      cur1->prev_mys->next_mys=cur1;
      cur1->list[priorities];
      cur1->pd_budget = 100000;
      cur1->prio_top = 0;
  }
  
    }
  Mys *mys_cur = Sc::getMys(this);
    if(prio > mys_cur->prio_top)
      mys_cur->prio_top = prio;

    if(!mys_cur->list[prio]){
      mys_cur->list[prio] = prev = next = this;
    }
    else{
      Sc *tmp = mys_cur->list[prio];
      do {
        if(tmp == this) {tmp = 0; break;};
        tmp = tmp->next;
      } while(tmp != mys_cur->list[prio]);
      if(tmp){
        next = mys_cur->list[prio];
        prev = mys_cur->list[prio]->prev;
        next->prev = prev->next = this;//??
        if (left)
            mys_cur->list[prio] = this;
        }
        else{
          mys_cur->list[prio] = prev = next = this;
        }
      }
 
    if (prio > current->prio || (this != current && prio == current->prio && left)){
        Cpu::hazard |= HZD_SCHED;
    }
  
    if (!left)
        left = budget;

    tsc = t;
}

void Sc::ready_dequeue (uint64 t)
{
  Mys *mys_cur = Sc::getMys(this);
    assert (prio < priorities);
    assert (cpu == Cpu::id);
  assert (prev && next);
  if (mys_cur->list[prio] == this)
          mys_cur->list[prio] = next == this ? nullptr : next;
  next->prev = prev;
  prev->next = next;
  prev = nullptr;
  while (!mys_cur->list[mys_cur->prio_top] && mys_cur->prio_top)
          mys_cur->prio_top--;
    ec->add_tsc_offset (tsc - t);

    tsc = t;

}

void Sc::schedule (bool suspend)
{
    Counter::print<1,16> (++Counter::schedule, Console_vga::COLOR_LIGHT_CYAN, SPN_SCH);
  
    assert (current);
    assert (suspend || !current->prev);
    uint64 t = rdtsc();
    current->time += t - current->tsc;
    current->left = Lapic::get_timer();
    //current_pd->budget_pd+=current->left;
    Cpu::hazard &= ~HZD_SCHED;

    if (EXPECT_TRUE (!suspend))
        current->ready_enqueue (t);
  
    //Sc *sc = list[prio_top];
  Sc *sc = current_mys->list[current_mys->prio_top];
 // trace(0,"prio=%u prio_top=%u ptr=%p pd=%d", sc->prio, current_mys->prio_top, sc, current_mys->current_pd->getMyNum());
 // trace(0, ">%d %p", sc->ec->get_pd()->getMyNum(), sc);
  
  counter++;
  if(counter%2 == 0){
    current_mys = current_mys->next_mys;
    Sc *tmp = current_mys->list[current_mys->prio_top];
    if(!tmp){
      current_mys = current_mys->next_mys;
  //    trace(0, "EMPTY LIST ^(");
    }
  }
 //   trace(0, "%u sc %p", counter, sc);
      assert (sc);
      //if (current_mys->budget_pd < sc->left) sc->left= current_mys->budget_pd
      Lapic::set_timer (static_cast<uint32>(sc->left));
      
     // current_mys->budget_pd-=sc->left;
      ctr_loop = 0;

      current = sc;
    
      sc->ready_dequeue (t);
 //  trace(0,"Fllag");
      sc->ec->activate();
}

void Sc::remote_enqueue()
{
    //if (Cpu::id == cpu)
        ready_enqueue (rdtsc());

    /*else {
        Sc::Rq *r = remote (cpu);

        Lock_guard <Spinlock> guard (r->lock);
  
        if (r->queue) {
            next = r->queue;
            prev = r->queue->prev;
            next->prev = prev->next = this;
        } else {
            r->queue = prev = next = this;
            Lapic::send_ipi (cpu, VEC_IPI_RRQ);
        }
    }*/
}

void Sc::rrq_handler()
{
  trace(0, ">>>>>>>>>>>>>");
    uint64 t = rdtsc();

    Lock_guard <Spinlock> guard (rq.lock);

    for (Sc *ptr = rq.queue; ptr; ) {

        ptr->next->prev = ptr->prev;
        ptr->prev->next = ptr->next;

        Sc *sc = ptr;

        ptr = ptr->next == ptr ? nullptr : ptr->next;

        sc->ready_enqueue (t);
    }

    rq.queue = nullptr;
}

void Sc::rke_handler()
{
    if (Pd::current->Space_mem::htlb.chk (Cpu::id))
        Cpu::hazard |= HZD_SCHED;
}
