/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_READY_SEARCH_H
#define ALPHA_READY_SEARCH_H
#include "world_unit_readiness.h"
namespace alpha {
struct ReadySearchFacts {int32_t reference,cursor_x,cursor_y,current;};
struct ReadySearchResult {int32_t selected,distance;uint32_t passes;uint8_t clear_deferred[256];};
/* Original offline Console_ready_search; online priority branch excluded.
 * Spatial reference/current/cursor facts are not phase or turn authority.
 * Failed passes clear actor state bit2, across ALL factions. First pass keeps
 * current's bit2 when reference is locally owned; second clears all. Original
 * wrapped range metric/tie preference retained. No spent movement is reset.
 * Immutable plan proposal. Explicit error on unsafe references/unsupported
 * checked readiness domain, with no partial clear mask on failure.
 */
LogicError world_ready_search(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 int32_t human,int32_t road,const ReadySearchFacts&,ReadySearchResult&);
class LocalProductionSession;
class ReadySearchPlan {
 public:const ReadySearchResult& preview()const{return result_;}
 private:friend class LocalProductionSession;
 uint64_t owner_identity_=0,revision_=0;ReadySearchFacts facts_{};ReadySearchResult result_{};
};
struct ReadySearchReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;ReadySearchResult applied;};
}
#endif
