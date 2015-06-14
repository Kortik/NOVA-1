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
Slab_cache Tc::cache (sizeof (Tc), 32);
INIT_PRIORITY (PRIO_LOCAL)
Sc::Rq Sc::rq;

static unsigned counter = 0;

Sc *        Sc::current;
unsigned    Sc::ctr_link;
unsigned    Sc::ctr_loop;
Mys *       Sc::current_mys=0;
Sc *Sc::list[priorities];

unsigned Sc::prio_top;

Sc::Sc (Pd *own, mword sel, Ec *e) : Kobject (SC, static_cast<Space_obj *>(own), sel, 0x1), ec (e), cpu (static_cast<unsigned>(sel)), prio (0), budget (Tc::getInstance()->GetTimeSc(0)), left (0)
{
    trace (0, "SC:%p created (PD:%p Kernel), budget=%lu", this, own,budget);
}

Sc::Sc (Pd *own, mword sel, Ec *e, unsigned c, unsigned p, unsigned q) : Kobject (SC, static_cast<Space_obj *>(own), sel, 0x1), ec (e), cpu (c), prio (p), budget (Tc::getInstance()->GetTimeSc(1)), left (0)
{
    trace (0, "SC:%p created (EC:%p CPU:%#x P:%u Q:%#x) budget =%lu", this, e, c, p, q, budget);
}

Mys *Sc::getMys(Sc *sc){
  Mys *tmp = current_mys->root_mys;
  do {
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
    cur->pd_budget = Tc::getInstance()->GetTimePd(cur->current_pd->getMyNum());
    cur->prio_top = 0;
    current_mys=cur;
	//trace(0,"------------------------------------Create Mys: %d budget: %lld",cur->current_pd->getMyNum(), cur->pd_budget);
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
      cur1->next_mys=cur1->root_mys=current_mys->root_mys;
     // cur1->root_mys=current_mys->root_mys;
      cur1->prev_mys=current_mys->root_mys->prev_mys;
      current_mys->root_mys->prev_mys = cur1;
      cur1->prev_mys->next_mys=cur1;
      cur1->list[priorities];
      cur1->pd_budget = Tc::getInstance()->GetTimePd(cur1->current_pd->getMyNum());
      cur1->prio_top = 0;
	//trace(0,"------------------------------------Create Mys: %p budget: %lld",cur1->current_pd->getMyNum(), cur1->pd_budget);
  }
  
    }
  Mys *mys_cur = Sc::getMys(this);
    if(prio > mys_cur->prio_top)
      mys_cur->prio_top = prio;

    if(!mys_cur->list[prio]){
      mys_cur->list[prio] = prev = next = this; // trace(0,"####################### Create Mys: %p budget: %lld sc %p",mys_cur->current_pd->getMyNum(), mys_cur->pd_budget, mys_cur->list[prio]);
    }
    else{
      Sc *tmp = mys_cur->list[prio];

      if(tmp){
        next = mys_cur->list[prio];
        prev = mys_cur->list[prio]->prev;
        next->prev = prev->next = this;
        if (left)
            mys_cur->list[prio] = this;
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
  //trace(0, "ready_dequeue start %p", this);
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
//trace(0,"PD %p, EC %p, SC %p, pd_budget %lld budget_sc %lld",current_mys->current_pd->getMyNum(), &ec, this ,current_mys->pd_budget, (long long)budget);
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
    current_mys->pd_budget+=current->left;
    Cpu::hazard &= ~HZD_SCHED;

    if (EXPECT_TRUE (!suspend))
        current->ready_enqueue (t);

    if (!current_mys->pd_budget){
      current_mys->pd_budget=Tc::getInstance()->GetTimePd(current_mys->current_pd->getMyNum());
      current_mys = current_mys->next_mys;
    }

    Sc *sc = current_mys->list[current_mys->prio_top];
    while(!sc){
      current_mys = current_mys->next_mys;
      sc = current_mys->list[current_mys->prio_top];
    }
      assert (sc);
  
      if (current_mys->pd_budget < sc->left) sc->left=(unsigned long)current_mys->pd_budget;
      Lapic::set_timer (static_cast<uint32>(sc->left));  
      current_mys->pd_budget-=sc->left;
      ctr_loop = 0;
      current = sc;
      sc->ready_dequeue (t);
      sc->ec->activate();
}

void Sc::remote_enqueue()
{
        ready_enqueue (rdtsc());
}

Tc *Tc::instance = 0;
Tc *Tc::getInstance() {
        
	if (!Tc::instance) Tc::instance = new Tc;
        return Tc::instance;
    }

unsigned long Tc::GetTimePd(int num) { return pd[num]; }
unsigned long Tc::GetTimeSc(int num) { if (num>=2) return sc[2]; else return sc[num]; }//*/


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
