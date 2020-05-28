#include <app/app.h>
#include <app/box_layout.h>
#include <app/object.h>
#include <app/widget.h>
#include <app/window.h>
#include <assert.h>
#include <unistd.h>

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 300, 300, 250, 250, "About");

    auto widget = App::Widget::create(window);
    widget->set_rect(Rect(25, 25, 200, 200));

    auto& layout = widget->set_layout<App::BoxLayout>();
    layout.add(App::Widget::create(nullptr));
    layout.add(App::Widget::create(nullptr));
    layout.add(App::Widget::create(nullptr));

    window->draw();

    app.enter();
    return 0;
}
