#pragma once

#include <di/container/action/sequence.h>
#include <di/container/string/fixed_string_to_utf8_string_view.h>
#include <di/container/string/string_impl.h>
#include <di/container/string/string_view.h>
#include <di/container/string/utf8_encoding.h>
#include <di/container/vector/static_vector.h>
#include <di/function/invoke.h>
#include <di/io/interface/writer.h>
#include <di/io/prelude.h>
#include <di/io/string_writer.h>
#include <di/io/write_exactly.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/reflect/enum_to_string.h>
#include <di/reflect/enumerator.h>
#include <di/reflect/field.h>
#include <di/reflect/reflect.h>
#include <di/serialization/serialize.h>
#include <di/serialization/serialize_string.h>
#include <di/util/declval.h>
#include <di/util/exchange.h>
#include <di/util/reference_wrapper.h>
#include <di/util/scope_value_change.h>
#include <di/vocab/error/result.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_for_each.h>
#include <di/vocab/tuple/tuple_sequence.h>

namespace di::serialization {
struct JsonFormat;

class JsonSerializerConfig {
public:
    JsonSerializerConfig() = default;

    constexpr JsonSerializerConfig pretty() const {
        auto config = *this;
        config.m_pretty = true;
        return config;
    }

    constexpr JsonSerializerConfig indent_width(int width) const {
        auto config = *this;
        config.m_indent_width = width;
        return config;
    }

    constexpr bool is_pretty() const { return m_pretty; }
    constexpr int indent_width() const { return m_indent_width; }

private:
    bool m_pretty { false };
    int m_indent_width { 4 };
};

template<concepts::Impl<Writer> Writer>
class JsonSerializer {
private:
    enum class State {
        First,
        Value,
        Normal,
    };

    class ObjectSerializerProxy {
    public:
        constexpr explicit ObjectSerializerProxy(JsonSerializer& serializer) : m_serializer(serializer) {}

        constexpr meta::WriterResult<void, Writer> serialize_bool(container::StringView key, bool value) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_bool(value);
        }

        constexpr meta::WriterResult<void, Writer> serialize_null(container::StringView key) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_null();
        }

        constexpr meta::WriterResult<void, Writer> serialize_string(container::StringView key,
                                                                    container::StringView view) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_string(view);
        }

        constexpr meta::WriterResult<void, Writer> serialize_number(container::StringView key,
                                                                    concepts::Integral auto number) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_number(number);
        }

        template<concepts::InvocableTo<meta::WriterResult<void, Writer>, JsonSerializer&> F>
        constexpr meta::WriterResult<void, Writer> serialize_array(container::StringView key, F&& function) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_array(util::forward<F>(function));
        }

        template<concepts::InvocableTo<meta::WriterResult<void, Writer>, ObjectSerializerProxy&> F>
        constexpr meta::WriterResult<void, Writer> serialize_object(container::StringView key, F&& function) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_object(util::forward<F>(function));
        }

        template<concepts::Serializable<JsonSerializer> T>
        constexpr meta::WriterResult<void, Writer> serialize(container::StringView key, T&& value) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return serialization::serialize(m_serializer.get(), value);
        }

    private:
        util::ReferenceWrapper<JsonSerializer> m_serializer;
    };

public:
    using SerializationFormat = JsonFormat;

    template<typename T>
    requires(concepts::ConstructibleFrom<Writer, T>)
    constexpr explicit JsonSerializer(T&& writer, JsonSerializerConfig config = {})
        : m_writer(util::forward<T>(writer)), m_config(config) {}

    constexpr meta::WriterResult<void, Writer> serialize_null() {
        DI_TRY(serialize_comma());

        DI_TRY(io::write_exactly(m_writer, "null"_sv));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_bool(bool value) {
        if (value) {
            return serialize_true();
        }
        return serialize_false();
    }

    constexpr meta::WriterResult<void, Writer> serialize_string(container::StringView view) {
        DI_TRY(serialize_comma());

        DI_TRY(io::write_exactly(m_writer, '"'));
        // FIXME: escape the string if needed.
        DI_TRY(io::write_exactly(m_writer, view));
        DI_TRY(io::write_exactly(m_writer, '"'));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_number(concepts::Integral auto number) {
        DI_TRY(serialize_comma());

        using Enc = container::string::Utf8Encoding;
        using TargetContext = format::BoundedFormatContext<Enc, meta::Constexpr<256zu>>;
        auto context = TargetContext {};
        DI_TRY(di::format::vpresent_encoded_context<Enc>(
            di::container::string::StringViewImpl<Enc>(encoding::assume_valid, u8"{}", 2),
            di::format::make_format_args<TargetContext>(number), context));

        DI_TRY(io::write_exactly(m_writer, context.output()));
        return {};
    }

    template<concepts::InvocableTo<meta::WriterResult<void, Writer>, JsonSerializer&> F>
    constexpr meta::WriterResult<void, Writer> serialize_array(F&& function) {
        DI_TRY(serialize_array_begin());
        auto guard = util::ScopeValueChange(m_state, State::First);
        DI_TRY(function::invoke(util::forward<F>(function), *this));
        return serialize_array_end();
    }

    template<concepts::InvocableTo<meta::WriterResult<void, Writer>, ObjectSerializerProxy&> F>
    constexpr meta::WriterResult<void, Writer> serialize_object(F&& function) {
        DI_TRY(serialize_object_begin());
        auto guard = util::ScopeValueChange(m_state, State::First);
        auto proxy = ObjectSerializerProxy(*this);
        DI_TRY(function::invoke(util::forward<F>(function), proxy));
        return serialize_object_end();
    }

    template<typename T, concepts::InstanceOf<reflection::Fields> M>
    constexpr meta::WriterResult<void, Writer> serialize(T&& value, M) {
        return serialize_object([&](auto& serializer) -> meta::WriterResult<void, Writer> {
            return vocab::tuple_sequence<meta::WriterResult<void, Writer>>(
                [&](auto field) -> meta::WriterResult<void, Writer> {
                    constexpr auto name = container::fixed_string_to_utf8_string_view<field.name>();

                    using Type = meta::Type<decltype(field)>;
                    if constexpr (concepts::Optional<Type>) {
                        if (!field.get(value).has_value()) {
                            return {};
                        }
                        return serializer.serialize(name, *field.get(value));
                    } else {
                        return serializer.serialize(name, field.get(value));
                    }
                },
                M {});
        });
    }

    template<typename T, concepts::InstanceOf<reflection::Enumerators> M>
    constexpr meta::WriterResult<void, Writer> serialize(T value, M) {
        return serialize_string(reflection::enum_to_string(value));
    }

    template<typename T, concepts::InstanceOf<reflection::Atom> M>
    requires(M::is_bool() || M::is_string() || M::is_integer())
    constexpr meta::WriterResult<void, Writer> serialize(T&& value, M) {
        if constexpr (M::is_bool()) {
            return serialize_bool(M::get(value));
        } else if constexpr (M::is_string()) {
            return serialize_string(M::get(value));
        } else if constexpr (M::is_integer()) {
            return serialize_number(M::get(value));
        }
    }

    template<typename T, concepts::InstanceOf<reflection::Atom> M>
    requires(M::is_list() && concepts::Serializable<meta::ContainerReference<T>, JsonSerializer>)
    constexpr meta::WriterResult<void, Writer> serialize(T&& value, M) {
        return serialize_array([&](auto& serializer) {
            return container::sequence(M::get(value), [&](auto&& element) {
                return serialization::serialize(serializer, element);
            });
        });
    }

    template<typename T, concepts::InstanceOf<reflection::Atom> M>
    requires(M::is_map() && concepts::detail::ConstantString<meta::TupleElement<meta::ContainerValue<T>, 0>> &&
             concepts::Serializable<meta::TupleValue<decltype(util::declval<meta::ContainerReference<T>>()), 1>,
                                    JsonSerializer>)
    constexpr meta::WriterResult<void, Writer> serialize(T&& value, M) {
        return serialize_object([&](auto& serializer) {
            return container::sequence(M::get(value), [&](concepts::TupleLike auto&& element) {
                return serializer.serialize(util::get<0>(element), util::get<1>(element));
            });
        });
    }

    constexpr Writer& writer() & { return m_writer; }
    constexpr Writer const& writer() const& { return m_writer; }
    constexpr Writer&& writer() && { return util::move(*this).m_writer; }

private:
    constexpr meta::WriterResult<void, Writer> serialize_true() {
        DI_TRY(serialize_comma());

        DI_TRY(io::write_exactly(m_writer, "true"_sv));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_false() {
        DI_TRY(serialize_comma());

        DI_TRY(io::write_exactly(m_writer, "false"_sv));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_comma() {
        if (m_state == State::Value) {
            return {};
        }
        if (util::exchange(m_state, State::Normal) == State::Normal) {
            DI_TRY(io::write_exactly(m_writer, ','));
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        } else if (m_indent > 0) {
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        }
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_object_begin() {
        DI_TRY(serialize_comma());
        DI_TRY(io::write_exactly(m_writer, '{'));
        ++m_indent;
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_object_end() {
        --m_indent;
        if (m_state != State::First) {
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        }
        DI_TRY(io::write_exactly(m_writer, '}'));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_array_begin() {
        DI_TRY(serialize_comma());
        DI_TRY(io::write_exactly(m_writer, '['));
        ++m_indent;
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_array_end() {
        --m_indent;
        if (m_state != State::First) {
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        }
        DI_TRY(io::write_exactly(m_writer, ']'));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_key(container::StringView key) {
        DI_TRY(serialize_string(key));
        DI_TRY(io::write_exactly(m_writer, ':'));
        if (pretty_print()) {
            DI_TRY(io::write_exactly(m_writer, ' '));
        }
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_newline() {
        if (!pretty_print()) {
            return {};
        }
        return io::write_exactly(m_writer, '\n');
    }

    constexpr meta::WriterResult<void, Writer> serialize_indent() {
        if (!pretty_print()) {
            return {};
        }
        return view::range(m_indent * m_config.indent_width()) | container::sequence([&](auto) {
                   return io::write_exactly(m_writer, ' ');
               });
    }

    constexpr bool pretty_print() const { return m_config.is_pretty(); }

    Writer m_writer;
    JsonSerializerConfig m_config {};
    usize m_indent { 0 };
    State m_state { State::First };
};

template<typename T>
JsonSerializer(T&&) -> JsonSerializer<T>;

template<typename T>
JsonSerializer(T&&, JsonSerializerConfig) -> JsonSerializer<T>;

template<concepts::Impl<io::Reader> Reader>
class JsonDeserializer;

struct JsonFormat {
    template<concepts::Impl<io::Writer> Writer, typename... Args>
    requires(concepts::ConstructibleFrom<JsonSerializer<meta::RemoveCVRef<Writer>>, Writer, Args...>)
    constexpr static auto serializer(Writer&& writer, Args&&... args) {
        return JsonSerializer<meta::RemoveCVRef<Writer>>(util::forward<Writer>(writer), util::forward<Args>(args)...);
    }

    template<concepts::Impl<io::Reader> Reader, typename... Args>
    requires(concepts::ConstructibleFrom<JsonDeserializer<meta::RemoveCVRef<Reader>>, Reader, Args...>)
    constexpr static auto deserializer(Reader&& reader, Args&&... args) {
        return JsonDeserializer<meta::RemoveCVRef<Reader>>(util::forward<Reader>(reader), util::forward<Args>(args)...);
    }
};

constexpr inline auto json_format = JsonFormat {};

namespace detail {
    struct ToJsonStringFunction {
        template<concepts::Serializable<JsonSerializer<io::StringWriter<>>> T, typename... Args>
        requires(concepts::ConstructibleFrom<JsonSerializer<io::StringWriter<>>, io::StringWriter<>, Args...>)
        constexpr auto operator()(T&& value, Args&&... args) const {
            return serialization::serialize_string(json_format, value, util::forward<Args>(args)...);
        }
    };
}

constexpr inline auto to_json_string = detail::ToJsonStringFunction {};

namespace detail {
    struct SerializeJsonFunction {
        template<concepts::Impl<io::Writer> Writer, concepts::Serializable<JsonSerializer<Writer>> T, typename... Args>
        requires(concepts::ConstructibleFrom<JsonSerializer<util::ReferenceWrapper<meta::RemoveReference<Writer>>>,
                                             util::ReferenceWrapper<meta::RemoveReference<Writer>>, Args...>)
        constexpr auto operator()(Writer&& writer, T&& value, Args&&... args) const {
            return serialize(json_format, util::ref(writer), value, util::forward<Args>(args)...);
        }
    };
}

constexpr inline auto serialize_json = detail::SerializeJsonFunction {};
}

namespace di {
using serialization::JsonFormat;
using serialization::JsonSerializer;
using serialization::JsonSerializerConfig;

using serialization::json_format;
using serialization::serialize_json;
using serialization::to_json_string;
}
