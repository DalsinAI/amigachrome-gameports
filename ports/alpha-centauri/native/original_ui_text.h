/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_ORIGINAL_UI_TEXT_H
#define ALPHA_ORIGINAL_UI_TEXT_H
struct RastPort;struct TTF_Font;
namespace alpha {
class OriginalUiText {
public:
 OriginalUiText();~OriginalUiText();bool open();void close();
 bool draw(RastPort*,int x,int baseline,const char*,int max_width=900,bool heading=false);
private:
 OriginalUiText(const OriginalUiText&)=delete;OriginalUiText& operator=(const OriginalUiText&)=delete;
 TTF_Font* body_;TTF_Font* heading_;bool initialized_;
};
}
#endif
