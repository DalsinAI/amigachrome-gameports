/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented from original instruction-observed queue behavior;
 * decoded record layouts from Thinker (MIT), see THINKER-LICENSE.txt.
 */
#ifndef ALPHA_PROTOTYPE_RETIREMENT_QUEUE_H
#define ALPHA_PROTOTYPE_RETIREMENT_QUEUE_H
#include "game_logic.h"
#include "original_game.h"
namespace alpha {
enum class RetirementQueueError {ok,bad_state,out_of_memory};
class RetirementQueueProposal {
 public:
  RetirementQueueProposal()=default;
  ~RetirementQueueProposal(){clear();}
  RetirementQueueProposal(const RetirementQueueProposal&)=delete;
  RetirementQueueProposal& operator=(const RetirementQueueProposal&)=delete;
  void clear();
  size_t count()const{return count_;}
  const OriginalBase* base(size_t i)const{return i<count_?bases_+i:nullptr;}
  uint16_t unit_flags()const{return unit_flags_;}
  bool changed()const{return changed_;}
 private:
  friend RetirementQueueError retirement_queue_stage(Variant,int32_t,uint16_t,const OriginalBase*,size_t,RetirementQueueProposal&);
  void swap(RetirementQueueProposal&);
  OriginalBase* bases_=nullptr;size_t count_=0;uint16_t unit_flags_=0;bool changed_=false;
};
/* Complete queue/deactivation suffix of retirement ONLY, after history/live-unit
 * retirement effects already resolved externally. Not a whole retirement or
 * discovery award. Scans ALL bases regardless owner. IDs0..63 unchanged;
 * custom64..511 replace current queue slot with original edition default
 * (base-38,SMAX-69), remove later matching entries, then clear active bit1.
 * Original increment-after-removal behavior and stale tail entries preserved.
 * queue_size is LAST active index (-1..9), not count. Checked bounded proposal
 * accepts at most512 bases (our explicit capacity, not an original game limit).
 * Owned output preserves all decoded fields; no wire/presentation/callback or
 * World commit. Failures/overlap with output object preserve existing output.
 * Reloading from a previous proposal's owned array is safe and transactional.
 */
RetirementQueueError retirement_queue_stage(Variant,int32_t prototype_id,uint16_t unit_flags,
 const OriginalBase*,size_t,RetirementQueueProposal&);
}
#endif
