#pragma once

#include "catch.hpp"
#include "../../hi/socket.hpp"

using namespace io;

namespace test_socket {
    static constexpr u16 SERVER_PORT = 7777;

    static inline Endpoint server_ep() {
        Endpoint ep{};
        ep.addr_be = IP::from_string("127.0.0.1");
        ep.port_be = io::htons(SERVER_PORT);
        return ep;
    }

    static inline bool would_block(Error e) noexcept {
        return e == Error::WouldBlock || e == Error::Again;
    }

    static inline bool recv_one(Socket& s, Endpoint& from, u8* buf, int cap, int& out_n, u64 timeout_ms) {
        const u64 start = monotonic_ms();
        for (;;) {
            out_n = s.recv_from(from, buf, cap);
            if (out_n > 0) return true;
            if (!would_block(s.error())) return false;
            if (monotonic_ms() - start >= timeout_ms) return false;
            io::sleep_ms(1);
        }
    }

    static inline void fill_prng(u32& st, u8* dst, u32 n) {
        // xorshift32 deterministic
        for (u32 i = 0; i < n; ++i) {
            st ^= st << 13;
            st ^= st >> 17;
            st ^= st << 5;
            dst[i] = (u8)(st & 0xFFu);
        }
    }

    // Build raw datagram with arbitrary header fields (host order -> wire), and arbitrary payload copy length.
    // payload_len_in_header may intentionally mismatch payload_bytes_to_copy.
    static inline u32 build_udp_datagram(u8* out, u32 cap,
        u32 magic, u16 version, u32 seq, u32 ack, u64 ack_bits,
        u8 chan, u8 type,
        const u8* payload, u16 payload_len_in_header,
        u32 payload_bytes_to_copy)
    {
        const u32 need = (u32)sizeof(UdpHeader) + payload_bytes_to_copy;
        if (need > cap) return 0;

        UdpHeader h{};
        h.ack_bits = ack_bits;
        h.magic = magic;
        h.seq = seq;
        h.ack = ack;
        h.version = version;
        h.payload_len = payload_len_in_header;
        h.chan = chan;
        h.type = type;

        udp_header_host_to_wire(h);

        for (u32 i = 0; i < (u32)sizeof(UdpHeader); ++i)
            out[i] = ((u8*)&h)[i];

        if (payload_bytes_to_copy && payload) {
            for (u32 i = 0; i < payload_bytes_to_copy; ++i)
                out[(u32)sizeof(UdpHeader) + i] = payload[i];
        }
        return need;
    }

    static inline bool send_raw(Socket& s, Endpoint to, const u8* bytes, u32 len) {
        const int sent = s.send_to(to, bytes, (int)len);
        return sent == (int)len;
    }

    static inline Socket make_udp_socket() {
        Socket s{};
        REQUIRE(s.open(Protocol::UDP));
        Endpoint bind_ep{};
        bind_ep.addr_be = IP::from_string("0.0.0.0");
        bind_ep.port_be = io::htons(0);
        REQUIRE(s.bind(bind_ep));
        REQUIRE(s.set_blocking(false));
        return s;
    }

    // Parse only packets that are "well-framed" like the server checks:
    // n == sizeof(UdpHeader)+payload_len and header magic/version correct.
    // Returns true if parsed as a valid protocol datagram.
    struct Parsed {
        Endpoint from{};
        UdpHeader h{};
        const u8* payload = nullptr;
        u32 payload_n = 0;
    };

    static inline bool parse_if_valid_framed(const Endpoint& from, const u8* rx, int n, Parsed& out) {
        if (!rx || n < (int)sizeof(UdpHeader)) return false;

        UdpHeader h{};
        for (usize i = 0; i < sizeof(UdpHeader); ++i) ((u8*)&h)[i] = rx[i];
        udp_header_wire_to_host(h);

        const u32 payload_len = (u32)h.payload_len;
        if ((u32)n != (u32)sizeof(UdpHeader) + payload_len) return false;
        if (h.magic != UDP_MAGIC || h.version != UDP_VERSION) return false;

        out.from = from;
        out.h = h;
        out.payload = rx + sizeof(UdpHeader);
        out.payload_n = payload_len;
        return true;
    }

    // Drain for a short window and REQUIRE that we do NOT observe COOKIE/WELCOME.
    // (We ignore malformed frames/magic/version, because those are "noise".)
    static inline void require_no_cookie_or_welcome(Socket& s, Endpoint srv, u8* rx, int cap, u64 window_ms) {
        const u64 until = monotonic_ms() + window_ms;
        while (monotonic_ms() < until) {
            Endpoint from{};
            const int n = s.recv_from(from, rx, cap);
            if (n <= 0) {
                if (!would_block(s.error())) break;
                io::sleep_ms(1);
                continue;
            }
            if (!endpoint_eq(from, srv)) continue;

            Parsed p{};
            if (!parse_if_valid_framed(from, rx, n, p)) continue;

            if (p.h.type == MSG_COOKIE || p.h.type == MSG_WELCOME) {
                FAIL("Unexpected handshake reply (COOKIE/WELCOME) to traffic that should not elicit it");
            }
        }
    }

    static inline void build_hello(msg_hello& h, u16 mtu, u16 feats, u32 nonce) {
        h.mtu = io::htons(mtu);
        h.features = io::htons(feats);
        h.client_nonce = io::htonl(nonce);
    }
    static inline void build_hello2(msg_hello2& h2, u64 cookie, u32 nonce, u16 mtu, u16 feats) {
        h2.cookie = io::htonll(cookie);
        h2.client_nonce = io::htonl(nonce);
        h2.mtu = io::htons(mtu);
        h2.features = io::htons(feats);
    }

    // Honest handshake with retries.
    // Returns true if HELLO->COOKIE->HELLO2->WELCOME is completed.
    static inline bool honest_handshake(Socket& s, Endpoint srv,
        u32 nonce, u16 mtu, u16 feats,
        u64 per_step_timeout_ms,
        u32 hello_retries)
    {
        constexpr u32 BUF_CAP = 4096;
        u8 tx[BUF_CAP]{};
        u8 rx[BUF_CAP]{};

        // --- Step 1: send HELLO (retry if needed), wait COOKIE
        u64 cookie = 0;
        for (u32 attempt = 0; attempt < hello_retries && cookie == 0; ++attempt) {
            msg_hello h{};
            build_hello(h, mtu, feats, nonce);

            const u32 n0 = build_udp_datagram(
                tx, BUF_CAP,
                UDP_MAGIC, UDP_VERSION,
                /*seq=*/1u + attempt, /*ack=*/0, /*ack_bits=*/0,
                (u8)UdpChan::Unreliable, MSG_HELLO,
                (const u8*)&h, (u16)sizeof(h), (u32)sizeof(h)
            );
            if (!n0) return false;
            (void)send_raw(s, srv, tx, n0);

            Endpoint from{};
            int got = 0;
            if (!recv_one(s, from, rx, (int)BUF_CAP, got, per_step_timeout_ms))
                continue;
            if (!endpoint_eq(from, srv))
                continue;

            Parsed p{};
            if (!parse_if_valid_framed(from, rx, got, p)) continue;
            if (p.h.type != MSG_COOKIE) continue;
            if (p.payload_n != sizeof(msg_cookie)) continue;

            msg_cookie c{};
            for (usize i = 0; i < sizeof(c); ++i) ((u8*)&c)[i] = p.payload[i];

            const u32 nonce_echo = io::ntohl(c.nonce_echo);
            const u64 ck = io::ntohll(c.cookie);
            if (nonce_echo != nonce) continue;
            if (ck == 0) continue;

            cookie = ck;
        }
        if (cookie == 0) return false;

        // --- Step 2: send HELLO2, wait WELCOME (a couple retries)
        for (u32 attempt = 0; attempt < 3; ++attempt) {
            msg_hello2 h2{};
            build_hello2(h2, cookie, nonce, mtu, feats);

            const u32 n2 = build_udp_datagram(
                tx, BUF_CAP,
                UDP_MAGIC, UDP_VERSION,
                /*seq=*/100u + attempt, /*ack=*/0, /*ack_bits=*/0,
                (u8)UdpChan::Reliable, MSG_HELLO2,
                (const u8*)&h2, (u16)sizeof(h2), (u32)sizeof(h2)
            );
            if (!n2) return false;
            (void)send_raw(s, srv, tx, n2);

            Endpoint from{};
            int got = 0;
            if (!recv_one(s, from, rx, (int)BUF_CAP, got, per_step_timeout_ms))
                continue;
            if (!endpoint_eq(from, srv))
                continue;

            Parsed p{};
            if (!parse_if_valid_framed(from, rx, got, p)) continue;
            if (p.h.type != MSG_WELCOME) continue;
            if (p.payload_n != sizeof(msg_welcome)) continue;

            msg_welcome w{};
            for (usize i = 0; i < sizeof(w); ++i) ((u8*)&w)[i] = p.payload[i];

            const u32 sid = io::ntohl(w.session_id);
            const u16 agreed_mtu = io::ntohs(w.mtu);
            if (sid == 0) continue;
            if (agreed_mtu < MIN_MTU || agreed_mtu > MAX_MTU) continue;

            return true;
        }

        return false;
    }
} // namespace test