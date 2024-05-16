#include <di/container/vector/vector.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/algorithm/use_resources.h>
#include <di/execution/algorithm/when_all.h>
#include <di/execution/coroutine/prelude.h>
#include <di/execution/interface/schedule.h>
#include <di/execution/io/async_net.h>
#include <di/execution/io/async_read_exactly.h>
#include <di/execution/io/async_write_exactly.h>
#include <di/execution/io/async_write_some.h>
#include <di/vocab/span/as_bytes.h>
#include <di/vocab/span/as_writable_bytes.h>
#include <dius/io_context.h>
#include <dius/net/address.h>
#include <dius/print.h>
#include <dius/test/prelude.h>

#ifdef __linux__
static void unix_scoket() {
    auto context = *di::create<dius::IoContext>();
    auto scheduler = context.get_scheduler();

    namespace ex = di::execution;

    auto address = dius::net::UnixAddress("\0dius_test_socket"_ts);

    auto message = "Hello, World!"_ts;

    auto executed = false;
    auto server = ex::use_resources(
        [&](auto& passive_socket) -> di::Lazy<> {
            co_await ex::async_bind(passive_socket, address.clone());
            co_await ex::async_listen(passive_socket, 1);

            co_await ex::use_resources(
                [&](auto& socket) -> di::Lazy<> {
                    auto buffer = di::Vector<char> {};
                    buffer.resize(message.size());

                    co_await ex::async_read_exactly(socket, di::as_writable_bytes(buffer.span()));

                    auto received = di::move(buffer) | di::to<di::TransparentString>();
                    ASSERT_EQ(received, message);

                    executed = true;
                    co_return {};
                },
                ex::async_accept(passive_socket));

            co_return {};
        },
        ex::async_make_socket(scheduler));

    auto client = ex::use_resources(
        [&](auto& socket) -> di::Lazy<> {
            co_await ex::async_connect(socket, address.clone());

            co_await ex::async_write_exactly(socket, di::as_bytes(message.span()));

            co_return {};
        },
        ex::async_make_socket(scheduler));

    auto result = di::sync_wait_on(context, ex::when_all(di::move(server), di::move(client)));
    ASSERT(result);
    ASSERT(executed);
}

TEST(socket, unix_scoket)
#endif
