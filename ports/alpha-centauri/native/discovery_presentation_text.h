/* SPDX-License-Identifier: GPL-3.0-or-later. Independent owned presentation
 * binding; inherited original data layouts retain their MIT attribution. */
#ifndef ALPHA_DISCOVERY_PRESENTATION_TEXT_H
#define ALPHA_DISCOVERY_PRESENTATION_TEXT_H
#include "discovery_continuation.h"
#include "original_parse_context.h"
namespace alpha {
struct DiscoveryTextRuntime {DiscoveryRuntimeObservation runtime;OriginalParseContext parser;char script_file[256];};
enum class DiscoveryTextError {ok,bad_state,runtime_required,pending_kind_required,text_held};
class DiscoveryPresentationText {
 public:
  bool ready()const{return ready_;}
  const OriginalParseContext& parser()const{return parser_;}
  const char* popup_key()const{return ready_?popup_key_:nullptr;}
  const char* script_file()const{return ready_?script_file_:nullptr;}
  DiscoveryContinuationBoundary source_boundary()const{return boundary_;}
  int32_t faction()const{return faction_;}int32_t technology()const{return technology_;}
 private:
  friend DiscoveryTextError discovery_presentation_text(const DiscoveryContinuationPlan&,const DiscoveryTextRuntime&,DiscoveryPresentationText&);
  OriginalParseContext parser_{};char popup_key_[16]{},script_file_[256]{};
  DiscoveryContinuationBoundary boundary_=DiscoveryContinuationBoundary::first_secrets_tail;
  int32_t faction_=0,technology_=0;bool ready_=false;
};
/* Complete bounded parse preparation for an actual pending reactor or Secrets
 * request only. Owns copied parser slots and original resource key/file. Uses
 * captured MapOwner/faction and actual decoded faction adjective; preserves
 * observed gender/plural defaults rather than guessing noun/leader sex.
 * Explicit current parser/file observations are mandatory data, never active
 * authority. No popup/template-morphology/render/audio/log/destructor outcome is
 * fabricated; the source plan stays pending. Generic/discovery-sharing/cleanup
 * contexts remain held. Prior result and every input stay unchanged on failure,
 * including all aliases into current decoded/opaque source storage.
 */
DiscoveryTextError discovery_presentation_text(const DiscoveryContinuationPlan&,const DiscoveryTextRuntime&,DiscoveryPresentationText&);
}
#endif
