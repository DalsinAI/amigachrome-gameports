#ifdef NDEBUG
#undef NDEBUG
#endif
#include <Ui/Widget.h>
#include <cassert>
#include <cstdio>
using namespace OpenLoco;
int main()
{
    Ui::Widget w{Ui::WidgetId::none, {0,0}, {70,12}, Ui::WidgetType::button,
        static_cast<Ui::WindowColour>(0), uint32_t{StringIds::label_button_ok}};
    assert(w.text == StringIds::label_button_ok);
    w.content = 0x12345678;
    assert(w.text == 0x5678);
    w.content = Ui::Widget::kContentNull;
    assert(w.text == StringIds::null);
    w.text = StringIds::label_button_ok;
    assert(w.content == StringIds::label_button_ok);
    w.content = StringIds::empty;
    assert(w.text == StringIds::empty);
    std::puts("PASS widget text, image-content low bits and null string marker");
}
