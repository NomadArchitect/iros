#pragma once

#include <app/layout.h>
#include <graphics/rect.h>
#include <liim/vector.h>

namespace App {

class BoxLayout : public Layout {
public:
    BoxLayout(Widget& widget);
    virtual ~BoxLayout() override;

    int spacing() const { return m_spacing; }
    void set_spacing(int spacing) { m_spacing = spacing; }

    virtual void add(SharedPtr<Widget> widget) override;
    virtual void layout() override;

private:
    const Rect& container_rect() const;
    int compute_available_space() const;
    int flexible_widget_count() const;

    Vector<Widget*> m_widgets;
    int m_spacing { 2 };
};

}
