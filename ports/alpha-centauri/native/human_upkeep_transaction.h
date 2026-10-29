/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_HUMAN_UPKEEP_TRANSACTION_H
#define ALPHA_HUMAN_UPKEEP_TRANSACTION_H
#include "world_population.h"
#include "world_worker_upkeep.h"
#include "human_upkeep_snapshot.h"
namespace alpha {
enum class HumanUpkeepError {ok,bad_state,bad_buffer,bad_rules,bad_world,no_memory,unsupported,overflow,already_prepared,bad_checkpoint};
enum class HumanUpkeepPhase {unprepared,base_removal_pending,specialists_pending,riots_pending,population_checked};
struct HumanUpkeepReceipt {
 size_t base;
 bool full_turn,food_checked,specialists_pending,riots_pending;
 HumanUpkeepPhase phase;
 WorldPopulationProposal population;
 WorkerUpkeepCandidate workers;
};
/* Owned internal transaction for the population/assignment prefix of human
 * base upkeep. Source records never mutate. Population/storage/cancellations
 * and worker proposals are staged in a separate owned world, then fresh food
 * is checked at the new population when a legal no-specialist mask permits.
 * Specialist optimisation and riots remain explicit blocking phases, including
 * when their provisional mask already permits a fresh numeric food query.
 * No public commit, phase override, year advance, EndTurn or mutable world API.
 * The future full upkeep transaction must resolve every outstanding phase and
 * run remaining support/energy/production stages before committing any world.
 */
class HumanUpkeepTransaction {
 public:
  HumanUpkeepTransaction()=default;
  ~HumanUpkeepTransaction(){close();}
  HumanUpkeepTransaction(const HumanUpkeepTransaction&)=delete;
  HumanUpkeepTransaction& operator=(const HumanUpkeepTransaction&)=delete;
  HumanUpkeepError start(const uint8_t* world,size_t,const uint8_t* rules,size_t,
   uint32_t runtime_seed,size_t memory_limit=64u*1024u*1024u);
  HumanUpkeepError prepare_population(size_t base,bool full_turn);
  HumanUpkeepError resume(const uint8_t*,size_t,const uint8_t* rules,size_t,
   size_t memory_limit=64u*1024u*1024u);
  void close();
  const OriginalWorld* source_world()const;
  const OriginalWorld* proposed_world()const;
  const HumanUpkeepReceipt* receipt()const;
  HumanUpkeepPhase phase()const;
  size_t checkpoint_size()const;
  LocalSnapshotError checkpoint(void*,size_t)const;
 private:
  struct Data;
  Data* data_=nullptr;
};
}
#endif
