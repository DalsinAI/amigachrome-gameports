/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_UNIT_SKIP_H
#define ALPHA_LOCAL_UNIT_SKIP_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
class LocalProductionSession;
struct LocalSkipPreview {size_t vehicle;uint32_t speed;uint8_t spent_before,spent_after;};
class LocalSkipCommand {
 public:const LocalSkipPreview& preview()const{return preview_;}
 private:friend class LocalProductionSession;
 uint64_t owner_identity_=0,revision_=0;LocalSkipPreview preview_{};
};
struct LocalSkipReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;LocalSkipPreview applied;};
/* Original veh_skip assigns low byte of speed(vehicle,0) to moves_spent ONLY.
 * This command supports the existing checked0..255 speed domain; original
 * wrapping speeds>255 remain unsupported/overflow. May lower an already larger
 * spent byte: do not replace original assignment with max(old,speed). No flags,
 * order, state, RNG, position, readiness/turn transition or rendering is changed.
 * Same-session lifetime/revision, local ownership and suspension gates apply.
 * Requires privately owned unit tables from action/accounting import.
 * No player input/frontend or native qualification is supplied by this API.
 */
}
#endif
