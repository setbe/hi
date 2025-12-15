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


    // ------------------------- unique_array ----------------------------------
    template <typename T>
    class unique_array {
    public:
        unique_array() noexcept : ptr_(nullptr) {}
        explicit unique_array(T* p) noexcept : ptr_(p) {}

        unique_array(unique_array&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
        unique_array& operator=(unique_array&& other) noexcept {
            if (this != &other) { reset(); ptr_ = other.ptr_; other.ptr_ = nullptr; }
            return *this;
        }

        ~unique_array() { reset(); }

        T* get() const noexcept { return ptr_; }
        T& operator[](usize i) const noexcept { return ptr_[i]; }

        void reset(T* p = nullptr) noexcept {
            if (ptr_) { delete[] ptr_; }
            ptr_ = p;
        }

        unique_array(const unique_array&) = delete;
        unique_array& operator=(const unique_array&) = delete;

    private:
        T* ptr_;
    };

} // namespace io