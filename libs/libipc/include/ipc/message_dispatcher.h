#pragma once

#include <eventloop/object.h>
#include <ipc/endpoint.h>

namespace IPC {

class Stream;

class MessageDispatcher : public App::Object {
public:
    virtual void handle_error(Endpoint&) = 0;
    virtual void handle_incoming_data(Endpoint&, Stream&) = 0;

    virtual void handle_send_error(Endpoint& endpoint) { handle_error(endpoint); }

    template<ConcreteMessage T>
    void send(Endpoint& endpoint, const T& message) {
        if (!endpoint.send<T>(message)) {
            handle_send_error(endpoint);
        }
    }
};

}
