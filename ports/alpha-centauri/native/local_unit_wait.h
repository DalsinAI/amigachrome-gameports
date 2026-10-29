/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_UNIT_WAIT_H
#define ALPHA_LOCAL_UNIT_WAIT_H
#include "ready_search.h"
namespace alpha {
class LocalProductionSession;
class LocalWaitCommand {
 public:const ReadySearchResult& preview()const{return result_;}
 private:friend class LocalProductionSession;
 uint64_t owner_identity_=0,revision_=0;ReadySearchFacts facts_{};ReadySearchResult result_{};
};
struct LocalWaitReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;ReadySearchResult applied;};
/* Atomic owned-state projection: defer the locally owned current actor (bit2),
 * then resolve original offline search. May clear other factions' deferred bits.
 * No movement/RNG/order changes, no input/renderer/Console completion or turn
 * authority. Repaint is an obligation even if the net world state is unchanged.
 * Full Console_ready also has UI, rally, iteration/audio effects not supplied.
 */
}
#endif
