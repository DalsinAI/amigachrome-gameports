/* SPDX-License-Identifier: GPL-3.0-or-later. Independent portable text layout. */
#ifndef ALPHA_NOTIFICATION_LAYOUT_H
#define ALPHA_NOTIFICATION_LAYOUT_H
#include "discovery_notification.h"
namespace alpha {
/* Supplied rasterizer metrics, not invented Win32 GDI callbacks. Advance -1
 * means unsupported source byte. Units are 26.6; no implicit encoding/kerning.
 * Actual original Arial font inputs and port metrics must be qualified by the
 * caller. This data never grants session or UI completion authority. */
struct NotificationFontMetrics {int32_t ascent,line_height,advance[256];};
struct NotificationLayoutInput {DiscoveryRuntimeObservation runtime;NotificationFontMetrics body,caption;int32_t width,caption_top,body_top,max_height;};
struct NotificationGlyph {int32_t x26,baseline;uint8_t byte,caption;};
enum class NotificationLayoutError {ok,bad_state,runtime_required,metrics_held,glyph_held,capacity,geometry_held};
class NotificationLayout {
 public:
  bool ready()const{return ready_;}bool pending()const{return ready_;}
  size_t count()const{return count_;}const NotificationGlyph* glyph(size_t n)const{return ready_&&n<count_?&glyphs_[n]:nullptr;}
  int32_t height()const{return height_;}int32_t faction()const{return faction_;}int32_t technology()const{return technology_;}
  DiscoveryContinuationBoundary source_boundary()const{return boundary_;}
 private:
  friend NotificationLayoutError notification_layout(const DiscoveryNotification&,const NotificationLayoutInput&,NotificationLayout&);
  NotificationGlyph glyphs_[2048]{};size_t count_=0;int32_t height_=0,faction_=0,technology_=0;DiscoveryContinuationBoundary boundary_=DiscoveryContinuationBoundary::first_secrets_tail;bool ready_=false;
};
/* Portable ASCII-uppercase caption/paragraph layout only, not full original popup chrome or
 * Win32 font metrics. Ordinary physical lines continue with one separating
 * space; caret1 starts a paragraph, caret2 wraps centered, caret3 inserts a gap.
 * Non-ASCII caption casing stays held (original CharUpperA needs live codepage).
 * Source widths/default popup font/inline graphics/links need their own actual
 * binding. Long unbreakable words and missing glyphs fail closed. All aliases
 * and failures preserve previous result and every input. A rendered view or
 * supplied input event does not acknowledge discovery; no outcome API exists. */
NotificationLayoutError notification_layout(const DiscoveryNotification&,const NotificationLayoutInput&,NotificationLayout&);
}
#endif
