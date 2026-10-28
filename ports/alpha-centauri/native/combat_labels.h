/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_COMBAT_LABELS_H
#define AC_ALPHA_COMBAT_LABELS_H
#include "world_battle_reporting.h"
namespace alpha {
enum class CombatLabelError {ok,bad_input,unsupported,missing_label,text_too_long};
struct OriginalLabelSpan {uint32_t offset,length;};
struct OriginalLabelsView {
 const uint8_t*bytes;size_t size;Variant edition;uint32_t count;
 OriginalLabelSpan labels[2048];
};
struct CombatTextLedger {uint32_t count;int32_t percentages[4];char names[4][80];};
struct CombatTextReporting {CombatTextLedger ledger;uint32_t resolved_lookups;};
/* Checked canonical #LABELS physical-row subset, LF or explicit CRLF transport.
 * Trims ASCII spaces only. Empty rows and literal commas/semicolons are labels.
 * Rejects NUL, bare CR, terminated rows >509 bytes (510 at EOF), unsupported header/count forms.
 * Source remains caller-owned, immutable and alive while the view is used.
 * Edition is an explicit caller profile, not inferred from file contents.
 * Full original file loader/CRT translation equivalence is NOT claimed. */
CombatLabelError import_original_labels(const uint8_t*,size_t,Variant,OriginalLabelsView&);
/* Resolves EVERY entered lookup, including zero/saturated ones, before output.
 * Source-order bounded strcpy semantics preserve unused slots and trailing
 * bytes after the copied NUL. Appended names >79 bytes are held, not truncated.
 * Pure owned lookup/copy completion only: no GUI/event acknowledgement, World
 * mutation, round execution, caller advancement or legal attack authority. */
CombatLabelError resolve_combat_reporting(const OriginalLabelsView&,Variant,
 const WorldBattleReportingInspection&,const CombatTextLedger&,CombatTextReporting&);
}
#endif
