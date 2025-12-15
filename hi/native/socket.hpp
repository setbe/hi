#pragma once


#include "types.hpp"
#include "out.hpp"
#include "slot_alloc.hpp"

// ------------------- OS-dependent Includes -------------------------
#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "Ws2_32.lib")
#   undef MessageBox
#   undef CreateWindow
#elif defined (__linux__)
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <unistd.h>
#   include <fcntl.h>
#   include <netdb.h>
#   include <errno.h>
#else
#   error "OS isn't specified"
#endif // OS


namespace io {
    // ---------------- Cross-platform byte order ----------------
    IO_CONSTEXPR u16 htons(u16 x) noexcept {
        return (x << 8) | (x >> 8);
    }

    IO_CONSTEXPR u32 htonl(u32 x) noexcept {
        return ((x & 0xFF) << 24) |
            ((x & 0xFF00) << 8) |
            ((x & 0xFF0000) >> 8) |
            ((x >> 24) & 0xFF);
    }

    // Convenience functions for network->host
    IO_CONSTEXPR u16 ntohs(u16 x) noexcept { return htons(x); }
    IO_CONSTEXPR u32 ntohl(u32 x) noexcept { return htonl(x); }

    struct IP {
        u32 addr_be;

        explicit IO_CONSTEXPR IP(u32 be) noexcept : addr_be(be) {}

        // Convert "X.Y.Z.W" -> u32 (network order), returns 0 on failure
        static IO_CONSTEXPR u32 parse_component(const char* s, usize& i) noexcept {
            u32 v = 0;
            int digits = 0;
            while (s[i] >= '0' && s[i] <= '9') {
                v = v * 10 + (s[i] - '0');
                ++i;
                ++digits;
                if (digits > 3) return 256; // invalid marker (>255)
            }
            return v;
        }

        static IO_CONSTEXPR u32 from_string(const char* s) noexcept {
            if (!s || !s[0]) return 0;
            usize i = 0;
            u32 o0 = parse_component(s, i); if (o0 > 255) return 0;
            if (s[i] != '.') return 0; ++i;
            u32 o1 = parse_component(s, i); if (o1 > 255) return 0;
            if (s[i] != '.') return 0; ++i;
            u32 o2 = parse_component(s, i); if (o2 > 255) return 0;
            if (s[i] != '.') return 0; ++i;
            u32 o3 = parse_component(s, i); if (o3 > 255) return 0;

            u32 v = ((o0 & 0xFFu) << 24) |
                ((o1 & 0xFFu) << 16) |
                ((o2 & 0xFFu) << 8) |
                ((o3 & 0xFFu));
            return ::io::htonl(v);
        }
    };

    // -------------------------------------------------------------
    // Out support
    // -------------------------------------------------------------
    inline native::Out& operator<<(native::Out& t, const IP& ip) noexcept {
#ifndef IO_TERMINAL
        return t;
#else
        u32 h = ::io::ntohl(ip.addr_be);
        u32 o0 = (h >> 24) & 0xFFu;
        u32 o1 = (h >> 16) & 0xFFu;
        u32 o2 = (h >> 8) & 0xFFu;
        u32 o3 = h & 0xFFu;
        t << o0 << '.' << o1 << '.' << o2 << '.' << o3;
        return t;
#endif
    }


    enum class Protocol {
        TCP,
        UDP
    };

    enum class Error {
        None = 0,
        WouldBlock,
        Again,
        Closed,
        Generic,
    };

    struct Socket {
#if defined(_WIN32)
        using native_t = SOCKET;
        static IO_CONSTEXPR_VAR native_t INVALID_NATIVE_SOCKET = INVALID_SOCKET;
#elif defined(__linux__)
        using native_t = int;
        static IO_CONSTEXPR_VAR native_t INVALID_NATIVE_SOCKET = -1;
#else
        using native_t = int; // Placeholder for future platforms
        static IO_CONSTEXPR_VAR native_t INVALID_NATIVE_SOCKET = -1;
#endif

    private:
        native_t _s{ INVALID_NATIVE_SOCKET };
        Error _error{ Error::Closed };
        Protocol _proto{ Protocol::TCP };
        bool _opened{ false };

    public:
        Socket() noexcept { Startup(); }
        ~Socket() noexcept { close(); }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;
        Socket(Socket&& other) noexcept : _s(other._s), _error(other._error), _opened(other._opened), _proto(other._proto) {
            other._s = INVALID_NATIVE_SOCKET;
            other._opened = false;
        }
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                close();
                _s = other._s;
                _error = other._error;
                _opened = other._opened;
                _proto = other._proto;
                other._s = INVALID_NATIVE_SOCKET;
                other._opened = false;
                other._error = Error::Closed;
            }
            return *this;
        }

        void disable_nagle() const noexcept {
#if defined(_WIN32)
            BOOL flag = TRUE;
            setsockopt(_s, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
#elif defined(__linux__)
            int flag = 1;
            setsockopt(_s, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
#else
#   error "Not implemented"
#endif
        }

        IO_NODISCARD bool is_open() const noexcept { return _opened; }

        IO_NODISCARD bool open(Protocol proto = Protocol::TCP) noexcept {
            _proto = proto;
            int type = (proto == Protocol::TCP) ? SOCK_STREAM : SOCK_DGRAM;

#if defined(_WIN32)
            _s = ::socket(AF_INET, type, (proto == Protocol::TCP) ? IPPROTO_TCP : IPPROTO_UDP);
#elif defined(__linux__)
            _s = ::socket(AF_INET, type, (proto == Protocol::TCP) ? 0 : IPPROTO_UDP);
#endif
            if (_s != INVALID_NATIVE_SOCKET) {
                // if (!unblock()) { updateLastError(); /* still may continue but mark error */ }
                _opened = true;
                _error = Error::None;
                return true;
            }
            updateLastError();
            return false;
        }

        void close() noexcept {
            if (_s == INVALID_NATIVE_SOCKET) return;
#if defined(_WIN32)
            ::closesocket(_s);
#else
            ::close(_s);
#endif
            _s = INVALID_NATIVE_SOCKET;
            _opened = false;
            _error = Error::Closed;
    }

         // set blocking/non-blocking, returns success
         IO_NODISCARD bool set_blocking(bool blocking) noexcept {
            bool ok = true;

#if defined(_WIN32)
            unsigned long mode = blocking ? 0 : 1;
            if (::ioctlsocket(_s, FIONBIO, &mode) != 0)
                ok = false;

#elif defined(__linux__)
            int flags = ::fcntl(_s, F_GETFL, 0);
            if (flags < 0) return false;

            if (blocking)
                flags &= ~O_NONBLOCK;
            else
                flags |= O_NONBLOCK;

            if (::fcntl(_s, F_SETFL, flags) != 0)
                ok = false;
#else
#           error "Not implemented"
#endif
            if (!ok) updateLastError();
            return ok;
        }


        IO_NODISCARD bool bind(u32 addr_be, u16 port_be) noexcept {
            if (!_opened) { _error = Error::Generic; return false; }
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = addr_be;
            a.sin_port = port_be;
            int r = ::bind(_s, reinterpret_cast<sockaddr*>(&a), sizeof(a));
            if (r == 0) { _error = Error::None; return true; }
            updateLastError();
            return false;
        }

        IO_NODISCARD bool listen(int backlog) noexcept {
            if (!_opened) { _error = Error::Generic; return false; }
            if (_proto != Protocol::TCP) { _error = Error::Generic; return false; }
            int r = ::listen(_s, backlog);
            if (r == 0) { _error = Error::None; return true; }
            updateLastError();
            return false;
        }

        IO_NODISCARD bool accept(Socket& out_client) noexcept {
            if (!_opened) { _error = Error::Generic; return false; }
            if (_proto != Protocol::TCP) { _error = Error::Generic; return false; }
            native_t new_s = INVALID_NATIVE_SOCKET;
#if defined(_WIN32)
            new_s = ::accept(_s, nullptr, nullptr);
            if (new_s == INVALID_NATIVE_SOCKET) { updateLastError(); return false; }
#else
            new_s = ::accept(_s, nullptr, nullptr);
            if (new_s < 0) { updateLastError(); return false; }
#endif
            out_client._s = new_s;
            out_client._opened = true;
            out_client._proto = Protocol::TCP;
            out_client._error = Error::None;
            return true;
        }

        IO_NODISCARD bool connect(u32 addr_be, u16 port_be) noexcept {
            if (!_opened) { _error = Error::Generic; return false; }
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = addr_be;
            a.sin_port = port_be;
            int r = ::connect(_s, reinterpret_cast<sockaddr*>(&a), sizeof(a));
            if (r == 0) { _error = Error::None; return true; }

            // Non-blocking in-progress detection
#if defined(_WIN32)
            int e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK || e == WSAEINPROGRESS) { _error = Error::WouldBlock; return false; }
#else
            int e = errno;
            if (e == EINPROGRESS || e == EWOULDBLOCK || e == EAGAIN) { _error = Error::WouldBlock; return false; }
#endif
            updateLastError();
            return false;
        }

        // send: returns number of bytes, or -1 on error (sets _error)
        IO_NODISCARD int send(const void* data, int size) noexcept {
            if (!_opened) { _error = Error::Generic; return -1; }
#if defined(_WIN32)
            int r = ::send(_s, reinterpret_cast<const char*>(data), size, 0);
#else
            int r = ::send(_s, data, size, 0);
#endif
            if (r >= 0) return r;
            updateLastError();
            return -1;
        }

        // recv: returns bytes (>0), 0 -> remote closed (we mark Closed and return -1), -1 on error
        IO_NODISCARD int recv(void* data, int size) noexcept {
            if (!_opened) { _error = Error::Generic; return -1; }
#if defined(_WIN32)
            int r = ::recv(_s, reinterpret_cast<char*>(data), size, 0);
#else
            int r = ::recv(_s, data, size, 0);
#endif
            if (r == 0) {
                _error = Error::Closed;
                return -1;
            }
            if (r > 0) return r;
            updateLastError();
            return -1;
        }

        // graceful shutdown (TCP)
        void shutdown() noexcept {
            if (!_opened) return;
#if defined(_WIN32)
            ::shutdown(_s, SD_BOTH);
#else
            ::shutdown(_s, SHUT_RDWR);
#endif
        }

        IO_NODISCARD native_t native() const noexcept { return _s; }
        IO_NODISCARD Error error() const noexcept { return _error; }


    private:
        IO_NODISCARD void updateLastError() noexcept {
#if defined(_WIN32)
            int e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK) { _error = Error::WouldBlock; return; }
            else if (e == WSAEINTR) { _error = Error::Again; return; }
            else if (e == WSAECONNRESET) { _error = Error::Closed; return; }
            else { _error = Error::Generic; return; }
#elif defined(__linux__)
            int e = errno;
            if (e == EWOULDBLOCK || e == EAGAIN) { _error = Error::WouldBlock; return; }
            else if (e == EINTR) { _error = Error::Again; return; }
            else if (e == ECONNRESET) { _error = Error::Closed; return; }
            else { _error = Error::Generic; return; }
#else
            _error = Error::Generic;
#endif
        }

        bool Startup() noexcept {
#if defined(_WIN32)
            static bool ready = false;
            if (!ready) {
                WSADATA w{};
                if (::WSAStartup(MAKEWORD(2, 2), &w) != 0) {
                    _error = Error::Generic;
                    return false;
                }
                ready = true;
            }
#endif
            _error = Error::None;
            return true;
        }
    };


    template<class F>
    struct FnWrap {
        F* f;
        static void trampoline(void* ud, int r, io::Error e) {
            F* func = reinterpret_cast<F*>(ud);
            (*func)(r, e);
        }
    };

    struct BaseAsync {
        int fd() const noexcept { return native_fd(); }   // for poll
        void poll_event(short revents) noexcept { handle_event(revents); }

    private:
        virtual int native_fd() const noexcept = 0;
        virtual void handle_event(short revents) noexcept = 0;
    };


    struct AsyncListener : BaseAsync {
        Socket* s;                 // not owns
        void* accept_ud;           // userdata for callback
        void (*accept_fn)(void*, Socket&); // callback after accept
        bool want_accept = false;

        void init(Socket& sock) noexcept {
            s = &sock;
            want_accept = false;
        }

        template<class F>
        void async_accept(F& func) noexcept {
            accept_ud = &func;
            accept_fn = [](void* ud, Socket& client) {
                F* f = reinterpret_cast<F*>(ud);
                (*f)(client);
            };
            want_accept = true;
        }

    private:
        int native_fd() const noexcept override {
            return static_cast<int>(s->native());
        }

        void handle_event(short revents) noexcept override {
            if (want_accept && (revents & POLLIN)) {
                Socket client;
                if (s->accept(client)) {
                    want_accept = false;
                    accept_fn(accept_ud, client);
                }
            }
        }
    };

    struct AsyncSocket : BaseAsync {
        Socket* s;

        void* send_ud;
        void* recv_ud;
        void* connect_ud;

        void (*send_fn)(void*, int, io::Error);
        void (*recv_fn)(void*, int, io::Error);
        void (*connect_fn)(void*, int, io::Error);

        const void* send_buf;
        void* recv_buf;

        int send_len;
        int recv_len;

        bool want_send;
        bool want_recv;
        bool want_connect;

        void init(Socket& sock) noexcept {
            s = &sock;
            want_send = false;
            want_recv = false;
            want_connect = false;
        }

        template<class F>
        void async_send(const void* data, int len, F& func) noexcept {
            send_buf = data;
            send_len = len;
            send_ud = &func;
            send_fn = &FnWrap<F>::trampoline;
            want_send = true;
        }

        template<class F>
        void async_recv(void* data, int len, F& func) noexcept {
            recv_buf = data;
            recv_len = len;
            recv_ud = &func;
            recv_fn = &FnWrap<F>::trampoline;
            want_recv = true;
        }

        template<class F>
        void async_connect(u32 addr, u16 port, F& func) noexcept {
            connect_ud = &func;
            connect_fn = &FnWrap<F>::trampoline;
            want_connect = true;

            int r = s->connect(addr, port);
            if (r == 0 || s->error() == io::Error::None) {
                want_connect = false;
                connect_fn(connect_ud, 0, io::Error::None);
            }
        }
    private:
        int native_fd() const noexcept override {
            return static_cast<int>(s->native());  // Socket::native() returns native fd
        }

        void handle_event(short revents) noexcept override {
            if (want_send && (revents & POLLOUT)) {
                int r = s->send(send_buf, send_len);
                want_send = false;
                send_fn(send_ud, r, s->error());
            }
            if (want_recv && (revents & POLLIN)) {
                int r = s->recv(recv_buf, recv_len);
                want_recv = false;
                recv_fn(recv_ud, r, s->error());
            }
            if (want_connect && (revents & POLLOUT)) {
                want_connect = false;
                connect_fn(connect_ud, 0, s->error());
            }
        }
    }; // struct AsyncSocket


    struct EventLoop {
        BaseAsync* items[32];
        int count = 0;
        bool running = true;
        void stop() noexcept { running = false; }

        void add(BaseAsync* a) noexcept {
            items[count++] = a;
        }

        void run() noexcept {
            while (running) {

#if defined(_WIN32)
                WSAPOLLFD fds[32];
#else
                pollfd fds[32];
#endif
                for (int i = 0; i < count; i++) {
                    auto* a = items[i];
                    fds[i].fd = items[i]->fd();
                    fds[i].events = 0;
                    fds[i].revents = 0;
                }
#if defined(_WIN32)
                WSAPoll(fds, count, -1);
#else
                poll(fds, count, -1);
#endif
                for (int i = 0; i < count; i++)
                    items[i]->poll_event(fds[i].revents);
            }
        }
    }; // struct EventLoop
} // namespace io