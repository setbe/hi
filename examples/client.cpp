#include "../hi/io.hpp"

int main() {
    io::EventLoop loop;
    io::AsyncSocket as;
    io::Socket s;

    s.open(io::Protocol::TCP);
    s.set_blocking(false);

    as.init(s);
    loop.add(&as);

    static char recv_buf[256];

    auto on_connect = [&](int, io::Error e) {
        io::out << "connected!" << io::out.endl;
        char msg[] = "hello\n";
        
        auto on_send = [&](int, io::Error e2) {
            io::out << "sent!" << io::out.endl;
        };
        auto on_recv = [&](int n, io::Error e3) {
            recv_buf[n] = 0;
            io::out << "SERVER: " << recv_buf << io::out.endl;
        };

        as.async_send(msg, 6, on_send);
        as.async_recv(recv_buf, 255, on_recv);
    };

    as.async_connect(
        io::IP::from_string("127.0.0.1"),
        io::htons(5000),
        on_connect
    );

    loop.run();

}
