/* SPDX-License-Identifier: GPL-3.0-or-later. */
#ifndef ALPHA_DISCOVERY_NOTIFICATION_H
#define ALPHA_DISCOVERY_NOTIFICATION_H
#include "discovery_presentation_text.h"
#include "original_template.h"
namespace alpha {
struct DiscoveryScriptInput {DiscoveryRuntimeObservation runtime;int32_t language_mode;char file[256];const uint8_t* bytes;size_t size;};
struct DiscoveryNotificationLine {uint32_t offset,length;uint8_t original_style;};
enum class DiscoveryNotificationError {ok,bad_state,runtime_required,resource_missing,resource_held,template_held,overlong};
class DiscoveryNotification {
 public:
  bool ready()const{return ready_;}
  bool pending()const{return ready_;} // No UI completion/acknowledgement API exists.
  const char* caption()const{return ready_?storage_:nullptr;}
  size_t line_count()const{return count_;}
  const DiscoveryNotificationLine* line(size_t n)const{return ready_&&n<count_?&lines_[n]:nullptr;}
  const char* line_text(size_t n)const{return ready_&&n<count_?storage_+lines_[n].offset:nullptr;}
  uint32_t width()const{return width_;}
  int32_t faction()const{return faction_;}int32_t technology()const{return technology_;}
  DiscoveryContinuationBoundary source_boundary()const{return boundary_;}
  // Stock supported resources carry no audio directive. This never requests a
  // guessed technology quote or claims the engine UI has no other sound effects.
  bool requests_audio()const{return false;}
 private:
  friend DiscoveryNotificationError discovery_notification(const DiscoveryPresentationText&,const DiscoveryScriptInput&,DiscoveryNotification&);
  char storage_[16384]{};DiscoveryNotificationLine lines_[64]{};
  size_t used_=0,count_=0;uint32_t width_=0;int32_t faction_=0,technology_=0;
  DiscoveryContinuationBoundary boundary_=DiscoveryContinuationBoundary::first_secrets_tail;bool ready_=false;
};
/* Bounded resource consumption for prepared REACTORUPGRADE/THESECRET0/1 requests.
 * Requires actual current script filename plus resolved owner SCRIPT bytes.
 * Owns expanded caption/line bytes and source caret-style codes, not font metrics
 * or an exact original popup. Only xs/caption directives and a simple body ending
 * at a blank line are supported; unknown/audio/button directives stay held.
 * Exact section identity, duplicate/late directives, capacities and aliases are
 * checked transactionally. Does not load files, play audio, present or fabricate
 * an outcome, resume discovery, log, destroy original objects or mutate World.
 */
DiscoveryNotificationError discovery_notification(const DiscoveryPresentationText&,const DiscoveryScriptInput&,DiscoveryNotification&);
}
#endif
