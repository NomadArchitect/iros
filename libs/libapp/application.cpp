#include <app/application.h>
#include <app/window.h>
#include <assert.h>
#include <eventloop/event.h>
#include <eventloop/selectable.h>
#include <liim/function.h>
#include <sys/mman.h>
#include <window_server/message.h>

namespace App {

static Application* s_app;

void WindowServerClient::initialize() {
    m_server = IPC::Endpoint::create(shared_from_this());
    m_server->set_dispatcher(shared_from_this());

    auto socket = UnixSocket::create_connection(shared_from_this(), "/tmp/.window_server.socket");
    m_server->set_socket(move(socket));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowClosedEventMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowEvent>(WindowEvent::Type::Close));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::MouseEventMessage& message) {
    auto events = s_app->m_mouse_tracker.notify_mouse_event(message.buttons, message.x, message.y, message.z);
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    for (auto& event : events) {
        EventLoop::queue_event(maybe_window.value(), move(event));
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::KeyEventMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<KeyEvent>(message.event.ascii, message.event.key, message.event.flags));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowDidResizeMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowEvent>(WindowEvent::Type::DidResize));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::WindowStateChangeMessage& message) {
    auto maybe_window = Window::find_by_wid(message.wid);
    assert(maybe_window.has_value());
    EventLoop::queue_event(maybe_window.value(), make_unique<WindowStateEvent>(message.active));
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ThemeChangeMessage&) {
    Window::windows().for_each([&](auto& window) {
        EventLoop::queue_event(window->weak_from_this(), make_unique<ThemeChangeEvent>());
    });
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidCreatedWindow& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_create_window(data);
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidChangeWindowTitle& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_change_window_title(data);
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidCloseWindow& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_close_window(data);
    }
}

void WindowServerClient::handle(IPC::Endpoint&, const WindowServer::Server::ServerDidMakeWindowActive& data) {
    if (m_window_server_listener) {
        m_window_server_listener->server_did_make_window_active(data);
    }
}

UniquePtr<Application> Application::create() {
    return UniquePtr<Application>(new Application);
}

Application::Application() {
    s_app = this;
    m_palette = Palette::create_from_shared_memory("/.shared_theme", PROT_READ);
    m_client = WindowServerClient::create(nullptr);
}

Application::~Application() {}

Application& Application::the() {
    return *s_app;
}

void Application::set_window_server_listener(WindowServerListener& listener) {
    if (!m_client->window_server_listener()) {
        m_client->server().send<WindowServer::Client::RegisterAsWindowServerListener>({});
    }
    m_client->set_window_server_listener(&listener);
}

void Application::remove_window_server_listener() {
    if (m_client->window_server_listener()) {
        m_client->server().send<WindowServer::Client::UnregisterAsWindowServerListener>({});
        m_client->set_window_server_listener(nullptr);
    }
}
}
