#pragma once

#include <di/bit/endian/little_endian.h>
#include <di/container/allocator/allocator.h>
#include <di/container/vector/vector.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_from.h>
#include <di/execution/algorithm/let.h>
#include <di/execution/algorithm/let_value_with.h>
#include <di/execution/algorithm/when_all.h>
#include <di/execution/algorithm/with_env.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/run.h>
#include <di/execution/io/async_read_exactly.h>
#include <di/execution/io/async_read_some.h>
#include <di/execution/io/async_write_exactly.h>
#include <di/execution/io/async_write_some.h>
#include <di/execution/io/ipc_protocol.h>
#include <di/execution/query/get_allocator.h>
#include <di/execution/query/get_sequence_cardinality.h>
#include <di/execution/query/get_stop_token.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/scope/scope.h>
#include <di/execution/sequence/ignore_all.h>
#include <di/execution/sequence/repeat.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/execution/types/empty_env.h>
#include <di/function/index_dispatch.h>
#include <di/function/invoke.h>
#include <di/function/make_deferred.h>
#include <di/function/monad/monad_try.h>
#include <di/function/tag_invoke.h>
#include <di/io/vector_reader.h>
#include <di/io/vector_writer.h>
#include <di/io/write_exactly.h>
#include <di/math/intcmp/representable_as.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/function.h>
#include <di/meta/language.h>
#include <di/meta/list.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/platform/compiler.h>
#include <di/platform/custom.h>
#include <di/serialization/binary_deserializer.h>
#include <di/serialization/binary_serializer.h>
#include <di/serialization/deserialize.h>
#include <di/serialization/serialize.h>
#include <di/sync/stop_token/forward_declaration.h>
#include <di/sync/stop_token/in_place_stop_source.h>
#include <di/sync/stop_token/in_place_stop_token.h>
#include <di/types/byte.h>
#include <di/util/addressof.h>
#include <di/util/bit_cast.h>
#include <di/util/defer_construct.h>
#include <di/util/noncopyable.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/array/array.h>
#include <di/vocab/error/error.h>
#include <di/vocab/error/result.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace di::execution {
namespace ipc_binary_ns {
    struct Client {
        using is_server = void;
    };

    struct Server {
        using is_client = void;
    };

    struct MessageHeader {
        LittleEndian<u32> message_type;
        LittleEndian<u32> message_number;
        LittleEndian<u32> message_size;
    };

    template<typename Proto, typename Read, typename Write, typename Alloc>
    struct ConnectionDataT {
        struct Type {
            [[no_unique_address]] Read read;
            [[no_unique_address]] Write write;
            [[no_unique_address]] Alloc allocator;
            Vector<byte, Alloc> send_buffer;
            Vector<byte, Alloc> receive_buffer;
            sync::InPlaceStopSource stop_source;
            u32 message_number { 0 };
        };
    };

    template<typename Proto, typename Read, typename Write, typename Alloc>
    using ConnectionData = meta::Type<ConnectionDataT<Proto, Read, Write, Alloc>>;

    template<typename Proto, typename ClientOrServer>
    struct MessageDecode {
        struct Sender : ClientOrServer {
            using is_sender = void;

            using Protocol = Proto;

            template<typename Rec>
            struct OperationStateT {
                struct Type : util::Immovable {
                    explicit Type(Rec receiver_, MessageHeader header_, Span<byte const> buffer_)
                        : receiver(di::move(receiver_)), header(header_), buffer(buffer_) {}

                    [[no_unique_address]] Rec receiver;
                    MessageHeader header;
                    Span<byte const> buffer;

                    friend void tag_invoke(Tag<start>, Type& self) {
                        using Messages = meta::MessageTypes<Sender>;
                        if (self.header.message_type >= meta::Size<Messages>) {
                            return set_error(di::move(self.receiver), Error { BasicError::InvalidArgument });
                        }

                        return function::index_dispatch<void, meta::Size<Messages>>(
                            self.header.message_type, [&]<usize index>(Constexpr<index>) {
                                using Message = meta::At<Messages, index>;

                                auto result = deserialize_binary<Message>(VectorReader(self.buffer));
                                if (!result) {
                                    return set_error(di::move(self.receiver), Error(di::move(result).error()));
                                }
                                return set_value(di::move(self.receiver), di::move(result).value());
                            });
                    }
                };
            };

            template<typename Rec>
            using OperationState = meta::Type<OperationStateT<Rec>>;

            using CompletionSignatures = meta::AsTemplate<
                types::CompletionSignatures,
                meta::PushBack<
                    meta::Transform<meta::MessageTypes<Sender>,
                                    meta::Chain<meta::Quote<meta::List>,
                                                meta::BindFront<meta::Quote<meta::AsLanguageFunction>, SetValue>>>,
                    SetError(Error)>>;

            explicit Sender(MessageHeader header_, Span<byte const> buffer_) : header(header_), buffer(buffer_) {}

            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            friend auto tag_invoke(Tag<connect>, Sender self, Rec receiver) {
                return OperationState<Rec> { di::move(receiver), self.header, self.buffer };
            }

            MessageHeader header;
            Span<byte const> buffer;
        };

        auto operator()(MessageHeader header, Span<byte const> buffer) const { return Sender(header, buffer); }
    };

    template<typename Proto, typename ClientOrServer>
    constexpr inline auto message_decode = MessageDecode<Proto, ClientOrServer> {};

    template<typename Proto, typename Read, typename Write, typename Alloc, typename ClientOrServer>
    struct MessageSequence {
        auto operator()(ConnectionData<Proto, Read, Write, Alloc>* data) const {
            return repeat(let_value_with(
                [data](Array<byte, sizeof(MessageHeader)>& buffer) {
                    return async_read_exactly(data->read, buffer.span()) | let_value([data, &buffer] {
                               auto header = util::bit_cast<MessageHeader>(buffer);
                               auto const total_size = u32(header.message_size);
                               return just_from([data, total_size] {
                                          return data->receive_buffer.reserve(total_size);
                                      }) |
                                      let_value([data, header] {
                                          return async_read_exactly(data->read, Span { data->receive_buffer.data(),
                                                                                       header.message_size.value() -
                                                                                           sizeof(MessageHeader) }) |
                                                 let_value([data, header] {
                                                     return message_decode<Proto, ClientOrServer>(
                                                         header, Span { data->receive_buffer.data(),
                                                                        header.message_size.value() });
                                                 });
                                      });
                           });
                },
                make_deferred<Array<byte, sizeof(MessageHeader)>>()));
        }
    };

    template<typename Proto, typename Read, typename Write, typename Alloc, typename ClientOrServer>
    constexpr inline auto message_sequence = MessageSequence<Proto, Read, Write, Alloc, ClientOrServer> {};

    template<typename Proto, typename Read, typename Write, typename Alloc, typename ClientOrServer>
    struct ConnectionTokenT {
        struct Type : ClientOrServer {
            using Serializer = BinarySerializer<VectorWriter<Vector<byte, Alloc>>>;
            using Protocol = Proto;

            explicit Type(ConnectionData<Proto, Read, Write, Alloc>* data_) : data(data_) {}

            friend auto tag_invoke(Tag<request_stop>, Type& self) { return self.data->stop_source.request_stop(); }

            template<concepts::Serializable<Serializer> T, typename U = meta::Decay<T>,
                     typename MessageList = meta::MessageTypes<Type>>
            requires(concepts::DecayConstructible<T> && meta::Contains<MessageList, U>)
            friend auto tag_invoke(Tag<send>, Type self, T&& message) {
                constexpr auto message_index = meta::Lookup<U, MessageList>;
                static_assert(math::representable_as<u32>(message_index),
                              "There can be at most 2^32 messages in a protocol.");

                // FIXME: acquire a async mutex first.
                return just_from([message = util::forward<T>(message), data = self.data] -> Result<> {
                           // Serialize the message. We re-use the send buffer to prevent extra allocations.
                           auto send_buffer = di::move(data->send_buffer);
                           send_buffer.clear();

                           auto total_size = sizeof(MessageHeader) + serialize_size(binary_format, message);
                           if constexpr (concepts::FallibleAllocator<Alloc>) {
                               DI_TRY(send_buffer.reserve(total_size));
                           } else {
                               send_buffer.reserve(total_size);
                           }

                           auto writer = VectorWriter(di::move(send_buffer));
                           auto message_header = MessageHeader {
                               .message_type = static_cast<u32>(message_index),
                               .message_number = data->message_number++,
                               .message_size = total_size,
                           };
                           auto as_bytes = di::bit_cast<Array<byte, sizeof(MessageHeader)>>(message_header);
                           DI_TRY(write_exactly(writer, as_bytes.span()));
                           DI_TRY(serialize_binary(writer, message));
                           data->send_buffer = di::move(writer).vector();
                           return {};
                       }) |
                       let_value([data = self.data] {
                           return async_write_exactly(data->write, data->send_buffer.span());
                       });
            }

            ConnectionData<Proto, Read, Write, Alloc>* data;
        };
    };

    template<typename Proto, typename Read, typename Write, typename Alloc, typename ClientOrServer>
    using ConnectionToken = meta::Type<ConnectionTokenT<Proto, Read, Write, Alloc, ClientOrServer>>;

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc,
             typename ClientOrServer>
    struct MakeJoinedSender {
        using Data = ConnectionData<Proto, Read, Write, Alloc>;
        using Token = ConnectionToken<Proto, Read, Write, Alloc, ClientOrServer>;

        using IncomingSequence = decltype(message_sequence<Proto, Read, Write, Alloc, ClientOrServer>(
            di::declval<ConnectionData<Proto, Read, Write, Alloc>*>()));

        static auto make_rx_sequence(Data* data, RxFun&& rx_function) {
            if constexpr (concepts::Invocable<RxFun, IncomingSequence, Token>) {
                return di::invoke(di::forward<RxFun>(rx_function),
                                  message_sequence<Proto, Read, Write, Alloc, ClientOrServer>(data), Token(data));
            } else {
                return di::invoke(di::forward<RxFun>(rx_function),
                                  message_sequence<Proto, Read, Write, Alloc, ClientOrServer>(data));
            }
        }

        static auto make_rx_sender(Data* data, RxFun&& rx_function) {
            using RxSequence = decltype(make_rx_sequence(data, di::forward<RxFun>(rx_function)));

            if constexpr (concepts::SequenceSender<RxSequence>) {
                return ignore_all(make_rx_sequence(data, di::forward<RxFun>(rx_function)));
            } else {
                return make_rx_sequence(data, di::forward<RxFun>(rx_function));
            }
        }

        template<typename Env>
        auto operator()(Data* data, TxFun&& tx_function, RxFun&& rx_function, Env&& env) const {
            return with_env(make_env(di::forward<Env>(env), with(get_allocator, auto(data->allocator)),
                                     with(get_stop_token, data->stop_source.get_stop_token())),
                            when_all(di::invoke(di::forward<TxFun>(tx_function), Token(data)),
                                     make_rx_sender(data, di::forward<RxFun>(rx_function))));
        }
    };

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc,
             typename ClientOrServer>
    constexpr inline auto make_joined_sender =
        MakeJoinedSender<Proto, Read, Write, TxFun, RxFun, Alloc, ClientOrServer> {};

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc,
             typename ClientOrServer, typename Rec>
    struct OperationStateT {
        struct Type {
        public:
            using ItemToken = ConnectionToken<Proto, Read, Write, Alloc, ClientOrServer>;
            using TxSender = meta::InvokeResult<TxFun, ItemToken>;

            using IncomingSequence = decltype(message_sequence<Proto, Read, Write, Alloc, ClientOrServer>(
                di::declval<ConnectionData<Proto, Read, Write, Alloc>*>()));
            using RxSender = meta::InvokeResult<RxFun, IncomingSequence>;

            using InnerSender = decltype(make_joined_sender<Proto, Read, Write, TxFun, RxFun, Alloc, ClientOrServer>(
                di::declval<ConnectionData<Proto, Read, Write, Alloc>*>(), di::declval<TxFun>(), di::declval<RxFun>(),
                get_env(di::declval<Rec>())));

            using InnerOp = meta::ConnectResult<InnerSender, Rec>;

            explicit Type(Read read, Write write, TxFun&& tx_function, RxFun&& rx_function, Alloc allocator,
                          Rec receiver)
                : m_data(di::move(read), di::move(write), di::move(allocator))
                , m_inner_op(connect(make_joined_sender<Proto, Read, Write, TxFun, RxFun, Alloc, ClientOrServer>(
                                         di::addressof(m_data), di::forward<TxFun>(tx_function),
                                         di::forward<RxFun>(rx_function), get_env(receiver)),
                                     di::move(receiver))) {}

        private:
            friend void tag_invoke(Tag<start>, Type& self) { start(self.m_inner_op); }

            ConnectionData<Proto, Read, Write, Alloc> m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS InnerOp m_inner_op;
        };
    };

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc,
             typename ClientOrServer, typename Rec>
    using OperationState = meta::Type<OperationStateT<Proto, Read, Write, TxFun, RxFun, Alloc, ClientOrServer, Rec>>;

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc,
             typename ClientOrServer>
    struct SenderT {
        struct Type : NonCopyable {
            using is_sender = void;

            explicit Type(Read read_, Write write_, TxFun tx_function_, RxFun rx_function_, Alloc allocator_)
                : read(di::move(read_))
                , write(di::move(write_))
                , tx_function(di::move(tx_function_))
                , rx_function(di::move(rx_function_))
                , allocator(di::move(allocator_)) {}

            template<typename Env>
            using Sigs = meta::CompletionSignaturesOf<
                decltype(make_joined_sender<Proto, Read, Write, TxFun, RxFun, Alloc, ClientOrServer>(
                    di::declval<ConnectionData<Proto, Read, Write, Alloc>*>(), di::declval<TxFun>(),
                    di::declval<RxFun>(), di::declval<Env>))>;

            template<typename Env>
            friend auto tag_invoke(Tag<get_completion_signatures>, Type&&, Env&&) -> Sigs<Env>;

            template<typename Rec>
            requires(concepts::ReceiverOf<Rec, Sigs<meta::EnvOf<Rec>>>)
            friend auto tag_invoke(Tag<connect>, Type&& self, Rec receiver) {
                return OperationState<Proto, Read, Write, TxFun, RxFun, Alloc, ClientOrServer, Rec>(
                    di::move(self.read), di::move(self.write), di::move(self.tx_function), di::move(self.rx_function),
                    di::move(self.allocator), di::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(empty_env, with(get_allocator, self.allocator));
            }

            [[no_unique_address]] Read read;
            [[no_unique_address]] Write write;
            [[no_unique_address]] TxFun tx_function;
            [[no_unique_address]] RxFun rx_function;
            [[no_unique_address]] Alloc allocator;
        };
    };

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc,
             typename ClientOrServer>
    using Sender = meta::Type<SenderT<Proto, Read, Write, TxFun, RxFun, Alloc, ClientOrServer>>;

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc>
    using ClientSender = Sender<Proto, meta::RemoveCVRef<Read>, meta::RemoveCVRef<Write>, meta::Decay<TxFun>,
                                meta::Decay<RxFun>, meta::Decay<Alloc>, Client>;

    template<typename Proto, typename Read, typename Write, typename TxFun, typename RxFun, typename Alloc>
    using ServerSender = Sender<Proto, meta::RemoveCVRef<Read>, meta::RemoveCVRef<Write>, meta::Decay<TxFun>,
                                meta::Decay<RxFun>, meta::Decay<Alloc>, Server>;

    template<concepts::InstanceOf<Protocol> Proto>
    struct ConnectToClientFunction {
        template<typename Comm, concepts::DecayConstructible TxFun, concepts::DecayConstructible RxFun,
                 concepts::Allocator Alloc = DefaultAllocator>
        requires(concepts::AsyncReadable<Comm> && concepts::AsyncWritable<Comm> && concepts::CopyConstructible<Comm>)
        auto operator()(Comm&& channel, TxFun&& tx_function, RxFun&& rx_function, Alloc&& allocator = {}) const {
            return (*this)(auto(channel), auto(channel), di::forward<TxFun>(tx_function),
                           di::forward<RxFun>(rx_function), di::forward<Alloc>(allocator));
        }

        template<concepts::AsyncReadable ReadComm, concepts::AsyncWritable WriteComm,
                 concepts::DecayConstructible TxFun, concepts::DecayConstructible RxFun,
                 concepts::Allocator Alloc = DefaultAllocator>
        auto operator()(ReadComm&& read, WriteComm&& write, TxFun&& tx_function, RxFun&& rx_function,
                        Alloc&& allocator = {}) const {
            return ServerSender<Proto, ReadComm, WriteComm, TxFun, RxFun, Alloc>(
                di::forward<ReadComm>(read), di::forward<WriteComm>(write), di::forward<TxFun>(tx_function),
                di::forward<RxFun>(rx_function), di::forward<Alloc>(allocator));
        }
    };

    template<concepts::InstanceOf<Protocol> Proto>
    struct ConnectToServerFunction {
        template<typename Comm, concepts::DecayConstructible TxFun, concepts::DecayConstructible RxFun,
                 concepts::Allocator Alloc = DefaultAllocator>
        requires(concepts::AsyncReadable<Comm> && concepts::AsyncWritable<Comm> && concepts::CopyConstructible<Comm>)
        auto operator()(Comm&& channel, TxFun&& tx_function, RxFun&& rx_function, Alloc&& allocator = {}) const {
            return (*this)(auto(channel), auto(channel), di::forward<TxFun>(tx_function),
                           di::forward<RxFun>(rx_function), di::forward<Alloc>(allocator));
        }

        template<concepts::AsyncReadable ReadComm, concepts::DecayConstructible TxFun,
                 concepts::DecayConstructible RxFun, concepts::AsyncWritable WriteComm,
                 concepts::Allocator Alloc = DefaultAllocator>
        auto operator()(ReadComm&& read, WriteComm&& write, TxFun&& tx_function, RxFun&& rx_function,
                        Alloc&& allocator = {}) const {
            return ClientSender<Proto, ReadComm, WriteComm, TxFun, RxFun, Alloc>(
                di::forward<ReadComm>(read), di::forward<WriteComm>(write), di::forward<TxFun>(tx_function),
                di::forward<RxFun>(rx_function), di::forward<Alloc>(allocator));
        }
    };
}

template<concepts::InstanceOf<Protocol> Proto>
constexpr inline auto ipc_binary_connect_to_client = ipc_binary_ns::ConnectToClientFunction<Proto> {};

template<concepts::InstanceOf<Protocol> Proto>
constexpr inline auto ipc_binary_connect_to_server = ipc_binary_ns::ConnectToServerFunction<Proto> {};
}

namespace di {
using execution::ipc_binary_connect_to_client;
using execution::ipc_binary_connect_to_server;
}
