#pragma once

#include "types.hpp"

// Minimal freestanding-friendly atomic wrapper for C++20
// - Prefer std::atomic if available
// - Fall back to GCC/Clang __atomic builtins if available
// - Fall back to older __sync builtins if available
// - Final fallback: volatile-backed (NO ATOMIC GUARANTEES!)

#if defined(__has_include)
#  if __has_include(<atomic>)
#    define IO_HAS_STD_ATOMIC 1
#  else
#    define IO_HAS_STD_ATOMIC 0
#  endif
#else
#  define IO_HAS_STD_ATOMIC 0
#endif

// Allow user to force disable std::atomic usage
#if defined(IO_NO_STD_ATOMIC)
#  undef IO_HAS_STD_ATOMIC
#  define IO_HAS_STD_ATOMIC 0
#endif

#if IO_HAS_STD_ATOMIC
#  include <atomic>
#endif

// Compiler builtins detection
#if !IO_HAS_STD_ATOMIC
#  if defined(__GNUC__) || defined(__clang__)
#    define IO_HAS___ATOMIC_BUILTINS 1
#  else
#    define IO_HAS___ATOMIC_BUILTINS 0
#  endif
#endif

#if !IO_HAS_STD_ATOMIC && !defined(IO_HAS___ATOMIC_BUILTINS)
#  define IO_HAS___ATOMIC_BUILTINS 0
#endif

#if !IO_HAS_STD_ATOMIC && !IO_HAS___ATOMIC_BUILTINS
#  if defined(__GNUC__) || defined(__clang__)
#    define IO_HAS___SYNC_BUILTINS 1
#  else
#    define IO_HAS___SYNC_BUILTINS 0
#  endif
#else
#  define IO_HAS___SYNC_BUILTINS 0
#endif

namespace io {

    // Simple memory_order enum for API parity. If std::atomic is present we'll use std::memory_order.
#if IO_HAS_STD_ATOMIC
    using memory_order = std::memory_order;
    static IO_CONSTEXPR_VAR memory_order memory_order_relaxed = std::memory_order_relaxed;
    static IO_CONSTEXPR_VAR memory_order memory_order_consume = std::memory_order_consume;
    static IO_CONSTEXPR_VAR memory_order memory_order_acquire = std::memory_order_acquire;
    static IO_CONSTEXPR_VAR memory_order memory_order_release = std::memory_order_release;
    static IO_CONSTEXPR_VAR memory_order memory_order_acq_rel = std::memory_order_acq_rel;
    static IO_CONSTEXPR_VAR memory_order memory_order_seq_cst = std::memory_order_seq_cst;
#else
// lightweight stand-in (only names; semantics depend on backend)
    enum memory_order {
        memory_order_relaxed = 0,
        memory_order_consume,
        memory_order_acquire,
        memory_order_release,
        memory_order_acq_rel,
        memory_order_seq_cst
    };
#endif

    // Primary template
    template<typename T>
    class atomic {
    public:
#if IO_HAS_STD_ATOMIC
        // Use std::atomic<T> directly
        atomic() noexcept = default;
        IO_CONSTEXPR atomic(T v) noexcept : _a(v) {}
        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) = delete;

        IO_NODISCARD inline T load(memory_order mo = memory_order_seq_cst) const noexcept {
            return _a.load(mo);
        }

        inline void store(T v, memory_order mo = memory_order_seq_cst) noexcept {
            _a.store(v, mo);
        }

        IO_NODISCARD inline T exchange(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.exchange(v, mo);
        }

        IO_NODISCARD inline bool compare_exchange_strong(T& expected, T desired,
            memory_order success = memory_order_seq_cst,
            memory_order failure = memory_order_seq_cst) noexcept {
            return _a.compare_exchange_strong(expected, desired, success, failure);
        }

        IO_NODISCARD inline T fetch_add(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_add(v, mo);
        }

        IO_NODISCARD inline T fetch_sub(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_sub(v, mo);
        }

        IO_NODISCARD inline T fetch_and(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_and(v, mo);
        }

        IO_NODISCARD inline T fetch_or(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_or(v, mo);
        }

        // convenience
        operator T() const noexcept { return load(); }
        T operator=(T v) noexcept { store(v); return v; }

    private:
        std::atomic<T> _a;
#else
        // Non-std fallback: try to use __atomic builtins, then __sync, else volatile fallback.
        inline atomic() noexcept : _v() {}
        IO_CONSTEXPR atomic(T v) noexcept : _v(v) {}

        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) = delete;

        IO_NODISCARD inline T load(memory_order /*mo*/ = memory_order_seq_cst) const noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_load_n(&_v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // __sync builtins do not have load; use read of volatile (not strictly atomic)
            return _v;
#else
            // WARNING: not atomic
            return _v;
#endif
        }

        inline void store(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            __atomic_store_n(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // no direct store builtin; use assignment (may not be atomic)
            _v = v;
#else
            _v = v; // not atomic
#endif
        }

        IO_NODISCARD inline T exchange(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_exchange_n(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // emulate with loop
            T old;
            do {
                old = _v;
            } while (!__sync_bool_compare_and_swap(&_v, old, v));
            return old;
#else
            T old = _v;
            _v = v;
            return old;
#endif
        }

        IO_NODISCARD inline bool compare_exchange_strong(T& expected, T desired,
            memory_order /*success*/ = memory_order_seq_cst,
            memory_order /*failure*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_compare_exchange_n(&_v, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // __sync_bool_compare_and_swap returns bool
            return __sync_bool_compare_and_swap(&_v, expected, desired)
                ? (true)
                : ((expected = _v), false);
#else
            // Not atomic: only succeed if exact match
            if (_v == expected) {
                _v = desired;
                return true;
            }
            else {
                expected = _v;
                return false;
            }
#endif
        }

        IO_NODISCARD inline T fetch_add(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_add(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_add(&_v, v);
#else
            T old = _v;
            _v = old + v;
            return old;
#endif
        }

        IO_NODISCARD inline T fetch_sub(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_sub(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_sub(&_v, v);
#else
            T old = _v;
            _v = old - v;
            return old;
#endif
        }

        IO_NODISCARD inline T fetch_and(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_and(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_and(&_v, v); // rarely available, keep for completeness
#else
            T old = _v;
            _v = old & v;
            return old;
#endif
        }

        IO_NODISCARD inline T fetch_or(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_or(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_or(&_v, v);
#else
            T old = _v;
            _v = old | v;
            return old;
#endif
        }

        operator T() const noexcept { return load(); }
        T operator=(T v) noexcept { store(v); return v; }

    private:
        // Use plain (volatile) storage for fallback (may not be atomic)
        volatile T _v;
#endif // IO_HAS_STD_ATOMIC
    }; // class atomic

} // namespace io
