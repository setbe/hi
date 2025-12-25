#pragma once
#include "slot_alloc.hpp"

namespace io {

    // ------------------------- unique ----------------------------------
    template <typename T>
    class unique {
    public:
        unique() noexcept : ptr_(nullptr) {}
        explicit unique(T* p) noexcept : ptr_(p) {}
        unique(unique&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
        unique& operator=(unique&& other) noexcept {
            if (this != &other) { reset(); ptr_ = other.ptr_; other.ptr_ = nullptr; }
            return *this;
        }

        ~unique() { reset(); }

        T* get() const noexcept { return ptr_; }
        T& operator*() const noexcept { return *ptr_; }
        T* operator->() const noexcept { return ptr_; }

        void reset(T* p = nullptr) noexcept {
            if (ptr_) { delete ptr_; }
            ptr_ = p;
        }

        unique(const unique&) = delete;
        unique& operator=(const unique&) = delete;

    private:
        T* ptr_;
    };

    // ------------------------- shared ----------------------------------
    template <typename T>
    class shared {
    public:
        shared() noexcept : ptr_(nullptr), count_(nullptr) {}
        explicit shared(T* p) : ptr_(p), count_(p ? new usize(1) : nullptr) {}

        shared(const shared& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
            if (count_) ++*count_;
        }
        shared& operator=(const shared& other) noexcept {
            if (this != &other) {
                release();
                ptr_ = other.ptr_;
                count_ = other.count_;
                if (count_) ++*count_;
            }
            return *this;
        }

        shared(shared&& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
            other.ptr_ = nullptr; other.count_ = nullptr;
        }
        shared& operator=(shared&& other) noexcept {
            if (this != &other) {
                release();
                ptr_ = other.ptr_;
                count_ = other.count_;
                other.ptr_ = nullptr; other.count_ = nullptr;
            }
            return *this;
        }

        ~shared() { release(); }

        T* get() const noexcept { return ptr_; }
        T& operator*() const noexcept { return *ptr_; }
        T* operator->() const noexcept { return ptr_; }

        usize use_count() const noexcept { return count_ ? *count_ : 0; }

        void reset(T* p = nullptr) noexcept {
            release();
            if (p) count_ = new usize(1);
            ptr_ = p;
        }

    private:
        void release() noexcept {
            if (!count_) return;
            if (--*count_ == 0) {
                delete ptr_;
                delete count_;
            }
            ptr_ = nullptr;
            count_ = nullptr;
        }

        T* ptr_;
        usize* count_;
    };


    // ------------------------- unique_bytes ----------------------------------

    struct unique_bytes {
        u8* p{ nullptr };

        unique_bytes() noexcept = default;
        explicit unique_bytes(u8* x) noexcept : p(x) {}
        ~unique_bytes() { reset(nullptr); }

        unique_bytes(const unique_bytes&) = delete;
        unique_bytes& operator=(const unique_bytes&) = delete;

        unique_bytes(unique_bytes&& o) noexcept : p(o.p) { o.p = nullptr; }
        unique_bytes& operator=(unique_bytes&& o) noexcept {
            if (this != &o) {
                reset(nullptr);
                p = o.p;
                o.p = nullptr;
            }
            return *this;
        }

        IO_NODISCARD u8* get() const noexcept { return p; }

        void reset(u8* x) noexcept {
            if (p) ::operator delete[](p);
            p = x;
        }
    };



} // namespace io