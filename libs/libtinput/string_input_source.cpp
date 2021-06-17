#include <tinput/repl.h>
#include <tinput/string_input_source.h>

namespace TInput {

StringInputSource::StringInputSource(Repl& repl, String string) : InputSource(repl), m_string(move(string)) {
    m_line_vector = m_string.split_view('\n');
}

StringInputSource::~StringInputSource() {}

InputResult StringInputSource::get_input() {
    clear_input();

    while (m_line_index < m_line_vector.size()) {
        auto input = input_text();
        if (!input.is_empty()) {
            input += String("\n");
        }
        input += String(m_line_vector[m_line_index++]);
        set_input(input);

        auto status = repl().get_input_status(input_text());
        if (status == InputStatus::Finished) {
            return InputResult::Success;
        }
    }

    if (!input_text().is_empty()) {
        return InputResult::Success;
    }
    return InputResult::Eof;
}
}