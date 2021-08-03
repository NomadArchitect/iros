#include <app/box_layout.h>
#include <app/context_menu.h>
#include <app/text_label.h>
#include <app/window.h>
#include <clipboard/connection.h>
#include <edit/document.h>
#include <edit/line_renderer.h>
#include <edit/position.h>
#include <eventloop/event.h>
#include <graphics/bitmap.h>
#include <graphics/renderer.h>

#include "app_display.h"

SearchWidget::SearchWidget() {}

SearchWidget::~SearchWidget() {}

void SearchWidget::render() {
    Widget::render();

    auto renderer = get_renderer();
    renderer.draw_rect(sized_rect(), ColorValue::White);
}

AppDisplay& SearchWidget::display() {
    if (!m_display) {
        auto& layout = set_layout<App::HorizontalBoxLayout>();
        auto& label = layout.add<App::TextLabel>("Find:");
        label.set_preferred_size({ 46, App::Size::Auto });

        m_display = layout.add<AppDisplay>(false).shared_from_this();
    }
    return *m_display;
}

AppDisplay::AppDisplay(bool main_display) : m_main_display(main_display) {}

void AppDisplay::initialize() {
    auto w = window()->shared_from_this();
    auto context_menu = App::ContextMenu::create(w, w);
    context_menu->add_menu_item("Copy", [this] {
        if (auto* doc = document()) {
            doc->copy(*this, cursors());
        }
    });
    context_menu->add_menu_item("Cut", [this] {
        if (auto* doc = document()) {
            doc->cut(*this, cursors());
        }
    });
    context_menu->add_menu_item("Paste", [this] {
        if (auto* doc = document()) {
            doc->paste(*this, cursors());
        }
    });
    set_context_menu(context_menu);
}

AppDisplay::~AppDisplay() {}

AppDisplay& AppDisplay::ensure_search_display() {
    assert(m_main_display);
    if (!m_search_widget) {
        m_search_widget = SearchWidget::create(shared_from_this());
        m_search_widget->set_hidden(true);
    }
    return m_search_widget->display();
}

Edit::RenderedLine AppDisplay::compose_line(const Edit::Line& line) const {
    auto renderer = Edit::LineRenderer { cols(), document()->word_wrap_enabled() };
    for (int index_into_line = 0; index_into_line <= line.length(); index_into_line++) {
        if (cursors().should_show_auto_complete_text_at(*document(), line, index_into_line)) {
            auto maybe_suggestion_text = cursors().preview_auto_complete_text(*this);
            if (maybe_suggestion_text) {
                renderer.begin_segment(index_into_line, Edit::CharacterMetadata::Flags::AutoCompletePreview,
                                       Edit::PositionRangeType::InlineAfterCursor);
                renderer.add_to_segment(maybe_suggestion_text->view(), maybe_suggestion_text->size());
                renderer.end_segment();
            }
        }

        if (index_into_line == line.length()) {
            break;
        }

        renderer.begin_segment(index_into_line, 0, Edit::PositionRangeType::Normal);
        char c = line.char_at(index_into_line);
        if (c == '\t') {
            auto spaces = String::repeat(' ', Edit::tab_width - (renderer.absolute_col_position() % Edit::tab_width));
            renderer.add_to_segment(spaces.view(), spaces.size());
        } else {
            renderer.add_to_segment(StringView { &c, 1 }, 1);
        }
        renderer.end_segment();
    }
    return renderer.finish(line);
}

void AppDisplay::output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata) {
    auto renderer = get_renderer();

    for (size_t i = col_offset; i < static_cast<size_t>(col_offset + cols()) && i < text.size(); i++) {
        render_cell(renderer, (i - col_offset) * col_width(), row * row_height(), text[i], metadata[i]);
    }
}

void AppDisplay::schedule_update() {
    invalidate();
}

int AppDisplay::enter() {
    window()->set_focused_widget(this);
    return 0;
}

void AppDisplay::quit() {
    if (on_quit) {
        on_quit();
    }
}

void AppDisplay::send_status_message(String) {}

Maybe<String> AppDisplay::prompt(const String&) {
    return {};
}

void AppDisplay::enter_search(String starting_text) {
    if (!m_main_display) {
        return;
    }

    ensure_search_display().set_document(Edit::Document::create_single_line(move(starting_text)));
    ensure_search_display().document()->on_change = [this] {
        auto contents = ensure_search_display().document()->content_string();
        document()->set_search_text(move(contents));
    };
    ensure_search_display().document()->on_submit = [this] {
        cursors().remove_secondary_cursors();
        document()->move_cursor_to_next_search_match(*this, cursors().main_cursor());
    };
    ensure_search_display().document()->on_escape_press = [this] {
        document()->set_search_text("");
        if (document()->on_escape_press) {
            document()->on_escape_press();
        }
        window()->set_focused_widget(this);
    };
    ensure_search_display().on_quit = [this] {
        document()->set_search_text("");
        if (document()->on_escape_press) {
            document()->on_escape_press();
        }
        window()->set_focused_widget(this);
    };

    m_search_widget->set_hidden(false);
    window()->set_focused_widget(&ensure_search_display());
}

void AppDisplay::set_clipboard_contents(String text, bool is_whole_line) {
    m_prev_clipboard_contents = move(text);
    m_prev_clipboard_contents_were_whole_line = is_whole_line;
    Clipboard::Connection::the().set_clipboard_contents_to_text(m_prev_clipboard_contents);
}

String AppDisplay::clipboard_contents(bool& is_whole_line) const {
    auto contents = Clipboard::Connection::the().get_clipboard_contents_as_text();
    if (!contents.has_value()) {
        is_whole_line = m_prev_clipboard_contents_were_whole_line;
        return m_prev_clipboard_contents;
    }

    auto& ret = contents.value();
    if (ret == m_prev_clipboard_contents) {
        is_whole_line = m_prev_clipboard_contents_were_whole_line;
    } else {
        m_prev_clipboard_contents = "";
        is_whole_line = m_prev_clipboard_contents_were_whole_line = false;
    }
    return move(ret);
}

void AppDisplay::do_open_prompt() {}

void AppDisplay::render_cursor(Renderer& renderer) {
    if (this != window()->focused_widget().get()) {
        return;
    }

    auto cursor_pos = document()->cursor_position_on_display(*this, cursors().main_cursor());

    int cursor_x = cursor_pos.col * col_width();
    int cursor_y = cursor_pos.row * row_height();
    for (int y = cursor_y; y < cursor_y + row_height(); y++) {
        renderer.pixels().put_pixel(cursor_x, y, ColorValue::White);
    }

    m_last_drawn_cursor_col = cursor_pos.col;
    m_last_drawn_cursor_row = cursor_pos.row;
}

void AppDisplay::render_cell(Renderer& renderer, int x, int y, char c, Edit::CharacterMetadata metadata) {
    RenderingInfo info = rendering_info_for_metadata(metadata);

    Color fg = info.fg.has_value() ? Color(info.fg.value()) : Color(VGA_COLOR_LIGHT_GREY);
    Color bg = info.bg.has_value() ? Color(info.bg.value()) : ColorValue::Black;

    auto cell_rect = Rect { x, y, col_width(), row_height() };
    renderer.fill_rect(cell_rect, bg);
    renderer.render_text(String(c), cell_rect, fg, TextAlign::Center, info.bold ? Font::bold_font() : Font::default_font());
}

void AppDisplay::render() {
    auto renderer = get_renderer();

    auto total_width = cols() * col_width();
    auto total_height = rows() * row_height();
    Rect bottom_extra_rect = { 0, total_height, total_width, sized_rect().height() - total_height };
    Rect right_extra_rect = { total_width, 0, sized_rect().width() - total_width, sized_rect().height() };
    renderer.fill_rect(bottom_extra_rect, ColorValue::Black);
    renderer.fill_rect(right_extra_rect, ColorValue::Black);

    renderer.fill_rect(sized_rect(), ColorValue::Black);
    document()->display(*this);

    render_cursor(renderer);
    Widget::render();
}

void AppDisplay::on_mouse_event(const App::MouseEvent& event) {
    if (!document()) {
        return;
    }

    auto event_copy =
        App::MouseEvent(event.mouse_event_type(), event.buttons_down(), event.x(), event.y(), event.z(), event.button(), event.modifiers());
    auto text_index = document()->text_index_at_scrolled_position(*this, { event.y() / row_height(), event.x() / col_width() });
    event_copy.set_x(text_index.index_into_line());
    event_copy.set_y(text_index.line_index());

    if (document()->notify_mouse_event(*this, event_copy)) {
        return;
    }

    Widget::on_mouse_event(event);
}

void AppDisplay::on_key_event(const App::KeyEvent& event) {
    if (!document() || !event.key_down()) {
        return;
    }

    document()->notify_key_pressed(*this, event);
}

void AppDisplay::document_did_change() {
    if (document()) {
        notify_line_count_changed();

        if (m_main_display) {
            document()->on_escape_press = [this] {
                if (m_search_widget) {
                    m_search_widget->set_hidden(true);
                }
            };
        }
        schedule_update();
    }
}

void AppDisplay::on_resize() {
    m_rows = positioned_rect().height() / row_height();
    m_cols = positioned_rect().width() / col_width();
    if (document()) {
        document()->notify_display_size_changed();
    }

    if (m_main_display) {
        ensure_search_display();
        constexpr int display_height = 28;
        m_search_widget->set_positioned_rect({ positioned_rect().x(), positioned_rect().y() + positioned_rect().height() - display_height,
                                               positioned_rect().width(), display_height });
    }
}

void AppDisplay::on_focused() {
    invalidate();
}