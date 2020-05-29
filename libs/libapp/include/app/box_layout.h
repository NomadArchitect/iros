#pragma once

#include <app/layout.h>
#include <liim/vector.h>

namespace App {

class BoxLayout : public Layout {
public:
    enum class Orientation {
        Horizontal,
        Vertical,
    };

    BoxLayout(Widget& widget, Orientation orientation);
    virtual ~BoxLayout() override;

    int spacing() const { return m_spacing; }
    void set_spacing(int spacing) { m_spacing = spacing; }

    virtual void add(SharedPtr<Widget> widget) override;
    virtual void layout() override;

    void set_orientation(Orientation o) { m_orientation = o; }
    Orientation orientation() const { return m_orientation; }

private:
    int compute_available_space() const;
    int flexible_widget_count() const;

    Vector<Widget*> m_widgets;
    int m_spacing { 2 };
    Orientation m_orientation { Orientation::Horizontal };
};

}
