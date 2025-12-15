#include "../hi/io.hpp"

int main() {
    io::EventLoop loop;

    // --- Listener ---
    io::Socket listener_sock;
    listener_sock.open(io::Protocol::TCP);
    listener_sock.bind(io::IP::from_string("127.0.0.1"), io::htons(5000));
    listener_sock.listen(8);

    io::AsyncListener listener;
    listener.init(listener_sock);
    loop.add(&listener);

    // client array (stack-based)
    io::AsyncSocket clients[8] = {};
    char client_bufs[8][256] = {};
    int client_count = 0;

    auto on_accept = [&](io::Socket& client) {
        if (client_count >= 8) return; // max clients
        io::out << "Client connected!" << io::out.endl;

        io::AsyncSocket& c = clients[client_count];
        c.init(client);
        loop.add(&c);

        char* buf = client_bufs[client_count];
        int idx = client_count;
        client_count++;

        auto on_recv = [&](void*, int n, io::Error e) {
            if (e == io::Error::None) {
                buf[n] = 0;
                io::out << "Client: " << buf << io::out.endl;
                c.async_recv(buf, sizeof(client_bufs[idx]) - 1, on_recv);
            }
        };

        c.async_recv(buf, sizeof(client_bufs[idx]) - 1, on_recv);
    };

    listener.async_accept(on_accept);

    loop.run();
}
