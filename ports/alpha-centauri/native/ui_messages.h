/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_UI_MESSAGES_H
#define ALPHA_UI_MESSAGES_H
#include "ui_panel.h"
namespace alpha {
enum class MessageKind : uint8_t { info,action,warning,combat,base,research };
struct LogMessage { int32_t turn;MessageKind kind;uint16_t repeat;char text[120]; };
/* Bounded ring of the newest 64 messages. A message equal to the newest one
 * (same turn, kind and text) increments its repeat count instead of a new line.
 * Control characters become '?'; empty text is rejected. */
class MessageLog {
public:
 static constexpr size_t capacity=64;
 MessageLog();
 void clear();
 bool add(int32_t turn,MessageKind kind,const char* text);
 size_t count()const{return count_;}
 /* 0 is the newest message. */
 const LogMessage* newest(size_t age)const;
 /* Scroll offset in lines from the newest; clamped to the history. */
 void scroll(int lines);
 size_t offset()const{return offset_;}
 uint32_t total_added()const{return total_;}
private:
 LogMessage items_[capacity];size_t head_,count_,offset_;uint32_t total_;
};
/* Panel with a title and the visible lines (oldest at top, newest at bottom),
 * a kind stripe per line and a scroll indicator when history is hidden.
 * Returns the number of text items produced. */
size_t layout_message_panel(const MessageLog&,const UiFrame&,const UiRect&,int line_height,UiTextItem* items,size_t capacity);
}
#endif
