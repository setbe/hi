#pragma once
#include "types.hpp"
#include "ptr.hpp"   // unique_array

namespace io {

    template<typename T>
    struct vector {
        using value_type = T;

        IO_CONSTEXPR vector() noexcept = default;

        ~vector() noexcept {
            destroy_range(this->_ptr, this->_len);
            _raw.reset(nullptr); // delete[] bytes
            _ptr = nullptr;
            _len = 0;
            _cap = 0;
        }

        vector(const vector&) = delete;
        vector& operator=(const vector&) = delete;

        vector(vector&& o) noexcept {
            _raw = static_cast<unique_array<u8>&&>(o._raw); // move
            _ptr = o._ptr;
            _len = o._len;
            _cap = o._cap;
            o._ptr = nullptr;
            o._len = 0;
            o._cap = 0;
        }

        vector& operator=(vector&& o) noexcept {
            if (this == &o) return *this;
            this->~vector();
            new (this) vector(static_cast<vector&&>(o));
            return *this;
        }

        // -------- view conversion --------
        IO_NODISCARD view<T> as_view() noexcept { return view<T>(_ptr, _len); }
        IO_NODISCARD view<const T> as_view() const noexcept { return view<const T>(_ptr, _len); }

        // -------- capacity/size --------
        IO_NODISCARD usize size() const noexcept { return _len; }
        IO_NODISCARD usize capacity() const noexcept { return _cap; }
        IO_NODISCARD bool empty() const noexcept { return _len == 0; }

        // -------- iterators --------
        IO_NODISCARD T* begin() noexcept { return _ptr; }
        IO_NODISCARD T* end() noexcept { return _ptr + _len; }
        IO_NODISCARD const T* begin() const noexcept { return _ptr; }
        IO_NODISCARD const T* end() const noexcept { return _ptr + _len; }

        // -------- data --------
        IO_NODISCARD T* data() noexcept { return _ptr; }
        IO_NODISCARD const T* data() const noexcept { return _ptr; }

        // -------- element access --------
        IO_NODISCARD T& operator[](usize i) noexcept { return _ptr[i]; }
        IO_NODISCARD const T& operator[](usize i) const noexcept { return _ptr[i]; }

        IO_NODISCARD T& front() noexcept { return _ptr[0]; }
        IO_NODISCARD T& back() noexcept { return _ptr[_len - 1]; }
        IO_NODISCARD const T& front() const noexcept { return _ptr[0]; }
        IO_NODISCARD const T& back() const noexcept { return _ptr[_len - 1]; }

        // -------- reserve / resize --------
        IO_NODISCARD bool reserve(usize new_cap) noexcept {
            if (new_cap <= _cap) return true;

            usize target = _cap ? (_cap * 2) : 8;
            if (target < new_cap) target = new_cap;

            u8* new_raw = static_cast<u8*>(::operator new[](target * sizeof(T)));
            if (!new_raw) return false;

            T* new_ptr = reinterpret_cast<T*>(new_raw);

            // move-construct existing elements into new storage
            for (usize i = 0; i < _len; ++i) {
                new (new_ptr + i) T(io::move(_ptr[i]));
                _ptr[i].~T();
            }

            _raw.reset(new_raw);
            _ptr = new_ptr;
            _cap = target;
            return true;
        }

        // resize to n (value-initialize new elems)
        IO_NODISCARD bool resize(usize n) noexcept {
            if (n > _cap) {
                if (!reserve(n)) return false;
            }

            if (n > _len) {
                // construct new elements
                for (usize i = _len; i < n; ++i) {
                    new (_ptr + i) T{};
                }
            }
            else if (n < _len) {
                // destroy extra
                for (usize i = n; i < _len; ++i) {
                    _ptr[i].~T();
                }
            }

            _len = n;
            return true;
        }

        // -------- modifiers --------
        IO_NODISCARD bool push_back(const T& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_ptr + _len) T(v);
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_back(T&& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_ptr + _len) T(io::move(v));
            ++_len;
            return true;
        }

        void pop_back() noexcept {
            if (_len == 0) return;
            --_len;
            _ptr[_len].~T();
        }

        void clear() noexcept {
            destroy_range(_ptr, _len);
            _len = 0;
        }

        void set_size_unsafe(usize n) noexcept {
            _len = (n <= _cap) ? n : _len;
        }

    private:
        unique_array<u8> _raw{}; // owner of raw bytes
        T* _ptr{ nullptr };
        usize _len{ 0 };
        usize _cap{ 0 };

        static void destroy_range(T* p, usize n) noexcept {
            if (!p) return;
            for (usize i = 0; i < n; ++i) {
                p[i].~T();
            }
        }
    }; // struct vector





    template<class CharT>
    struct basic_string {
        io::vector<CharT> v;

        IO_CONSTEXPR basic_string() noexcept {
            v.push_back(CharT(0));
            v.set_size_unsafe(1);
        }

        basic_string(const basic_string&) = delete;
        basic_string& operator=(const basic_string&) = delete;

        basic_string(basic_string&& o) noexcept : v(static_cast<io::vector<CharT>&&>(o.v)) {}
        basic_string& operator=(basic_string&& o) noexcept { v = static_cast<io::vector<CharT>&&>(o.v); return *this; }

        // from view
        explicit basic_string(view<const CharT> s) noexcept {
            init_empty();
            append(s);
        }

        // from zero-terminated
        explicit basic_string(const CharT* s) noexcept {
            init_empty();
            if (!s) return;
            append_cstr(s);
        }

        IO_NODISCARD usize size() const noexcept {
            const usize n = v.size();
            return n ? (n - 1) : 0; // without '\0'
        }
        IO_NODISCARD bool empty() const noexcept { return size() == 0; }

        IO_NODISCARD const CharT* c_str() const noexcept { return v.data() ? v.data() : zlit(); }
        IO_NODISCARD CharT* data() noexcept { return v.data(); }
        IO_NODISCARD const CharT* data() const noexcept { return c_str(); }

        IO_NODISCARD CharT& operator[](usize i) noexcept { return v[i]; }
        IO_NODISCARD const CharT& operator[](usize i) const noexcept { return v[i]; }

        IO_NODISCARD view<const CharT> as_view() const noexcept { return view<const CharT>(v.data(), size()); }
        IO_NODISCARD view<CharT> as_view() noexcept { return view<CharT>(v.data(), size()); }

        IO_NODISCARD bool reserve(usize n) noexcept { return v.reserve(n + 1); }

        void clear() noexcept {
            v.clear();
            v.push_back(CharT(0));
            v.set_size_unsafe(1);
        }

        IO_NODISCARD bool resize(usize n, CharT fill = CharT(0)) noexcept {
            const usize old = size();
            if (!ensure_terminator()) return false;

            if (n < old) {
                if (!v.resize(n + 1)) return false;
                v[n] = CharT(0);
                return true;
            }

            if (n > old) {
                if (!v.resize(n + 1)) return false;
                for (usize i = old; i < n; ++i) v[i] = fill;
                v[n] = CharT(0);
            }
            return true;
        }

        IO_NODISCARD bool push_back(CharT ch) noexcept {
            const usize n = size();
            if (!ensure_terminator()) return false;
            if (!v.resize(n + 2)) return false;
            v[n] = ch;
            v[n + 1] = CharT(0);
            return true;
        }

        IO_NODISCARD bool append(view<const CharT> s) noexcept {
            if (s.size() == 0) return true;
            const usize n = size();
            if (!ensure_terminator()) return false;

            if (!v.resize(n + s.size() + 1)) return false;
            for (usize i = 0; i < s.size(); ++i) v[n + i] = s[i];
            v[n + s.size()] = CharT(0);
            return true;
        }

        IO_NODISCARD bool append(const CharT* s) noexcept {
            if (!s) return true;
            return append_cstr(s);
        }

        IO_NODISCARD bool operator+=(view<const CharT> s) noexcept { return append(s); }
        IO_NODISCARD bool operator+=(const CharT* s) noexcept { return append(s); }
        IO_NODISCARD bool operator+=(CharT ch) noexcept { return push_back(ch); }

        friend bool operator==(const basic_string& a, view<const CharT> b) noexcept {
            if (a.size() != b.size()) return false;
            for (usize i = 0; i < b.size(); ++i) if (a[i] != b[i]) return false;
            return true;
        }

    private:
        static const CharT* zlit() noexcept {
            static const CharT z[1] = { CharT(0) };
            return z;
        }

        void init_empty() noexcept {
            v.clear();
            v.push_back(CharT(0));
            v.set_size_unsafe(1);
        }

        IO_NODISCARD bool ensure_terminator() noexcept {
            if (v.size() == 0) return v.push_back(CharT(0));
            if (v[v.size() - 1] != CharT(0)) return v.push_back(CharT(0));
            return true;
        }

        IO_NODISCARD bool append_cstr(const CharT* s) noexcept {
            usize n = 0;
            while (s[n] != CharT(0)) ++n;
            return append(view<const CharT>(s, n));
        }
    }; // struct basic_string

    using string = basic_string<char>;
    using wstring = basic_string<wchar_t>;







    template<typename T>
    struct deque {
        using value_type = T;

        IO_CONSTEXPR deque() noexcept = default;

        ~deque() noexcept {
            clear();
            _raw.reset(nullptr);
            _buf = nullptr;
            _cap = 0;
            _len = 0;
            _front = 0;
        }

        deque(const deque&) = delete;
        deque& operator=(const deque&) = delete;

        deque(deque&& o) noexcept {
            _raw = static_cast<unique_array<u8>&&>(o._raw);
            _buf = o._buf;
            _cap = o._cap;
            _len = o._len;
            _front = o._front;

            o._buf = nullptr;
            o._cap = 0;
            o._len = 0;
            o._front = 0;
        }

        deque& operator=(deque&& o) noexcept {
            if (this == &o) return *this;
            this->~deque();
            new (this) deque(static_cast<deque&&>(o));
            return *this;
        }

        // ---- size/capacity ----
        IO_NODISCARD usize size() const noexcept { return _len; }
        IO_NODISCARD usize capacity() const noexcept { return _cap; }
        IO_NODISCARD bool empty() const noexcept { return _len == 0; }

        // ---- element access ----
        IO_NODISCARD T& operator[](usize i) noexcept { return _buf[idx(i)]; }
        IO_NODISCARD const T& operator[](usize i) const noexcept { return _buf[idx(i)]; }

        IO_NODISCARD T& front() noexcept { return (*this)[0]; }
        IO_NODISCARD const T& front() const noexcept { return (*this)[0]; }

        IO_NODISCARD T& back() noexcept { return (*this)[_len - 1]; }
        IO_NODISCARD const T& back() const noexcept { return (*this)[_len - 1]; }

        // ---- modifiers ----
        IO_NODISCARD bool reserve(usize new_cap) noexcept {
            if (new_cap <= _cap) return true;

            usize target = _cap ? (_cap * 2) : 8;
            if (target < new_cap) target = new_cap;

            u8* new_raw = static_cast<u8*>(::operator new[](target * sizeof(T)));
            if (!new_raw) return false;

            T* new_buf = reinterpret_cast<T*>(new_raw);

            // move existing items in logical order: 0.._len-1
            for (usize i = 0; i < _len; ++i) {
                new (new_buf + i) T(io::move((*this)[i]));
                (*this)[i].~T();
            }

            _raw.reset(new_raw);
            _buf = new_buf;
            _cap = target;
            _front = 0;
            return true;
        }

        IO_NODISCARD bool push_back(const T& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_buf + idx(_len)) T(v);
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_back(T&& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_buf + idx(_len)) T(io::move(v));
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_front(const T& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            dec_front();
            new (_buf + _front) T(v);
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_front(T&& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            dec_front();
            new (_buf + _front) T(io::move(v));
            ++_len;
            return true;
        }

        void pop_back() noexcept {
            if (_len == 0) return;
            usize bi = idx(_len - 1);
            _buf[bi].~T();
            --_len;
            if (_len == 0) _front = 0;
        }

        void pop_front() noexcept {
            if (_len == 0) return;
            _buf[_front].~T();
            _front = (_front + 1) % _cap;
            --_len;
            if (_len == 0) _front = 0;
        }

        void clear() noexcept {
            if (!_buf || _len == 0) { _len = 0; _front = 0; return; }
            for (usize i = 0; i < _len; ++i) {
                (*this)[i].~T();
            }
            _len = 0;
            _front = 0;
        }

    private:
        unique_array<u8> _raw{};
        T* _buf{ nullptr };
        usize _cap{ 0 };
        usize _len{ 0 };
        usize _front{ 0 };

        IO_NODISCARD usize idx(usize i) const noexcept {
            // logical i -> physical index in ring
            return (_front + i) % _cap;
        }

        void dec_front() noexcept {
            // called only when _cap > 0
            _front = (_front == 0) ? (_cap - 1) : (_front - 1);
        }
    }; // struct deque








    template<typename T>
    struct list {
        using value_type = T;

        struct node {
            node* prev;
            node* next;
            T value;

            node(const T& v) : prev(nullptr), next(nullptr), value(v) {}
            node(T&& v) : prev(nullptr), next(nullptr), value(io::move(v)) {}
        };

        struct iterator {
            node* p{ nullptr };

            T& operator*() const noexcept { return p->value; }
            T* operator->() const noexcept { return &p->value; }

            iterator& operator++() noexcept { p = p ? p->next : nullptr; return *this; }
            iterator operator++(int) noexcept { iterator t = *this; ++(*this); return t; }

            iterator& operator--() noexcept { p = p ? p->prev : nullptr; return *this; }

            friend bool operator==(iterator a, iterator b) noexcept { return a.p == b.p; }
            friend bool operator!=(iterator a, iterator b) noexcept { return a.p != b.p; }
        };

        struct const_iterator {
            const node* p{ nullptr };

            const T& operator*() const noexcept { return p->value; }
            const T* operator->() const noexcept { return &p->value; }

            const_iterator& operator++() noexcept { p = p ? p->next : nullptr; return *this; }
            const_iterator operator++(int) noexcept { const_iterator t = *this; ++(*this); return t; }

            friend bool operator==(const_iterator a, const_iterator b) noexcept { return a.p == b.p; }
            friend bool operator!=(const_iterator a, const_iterator b) noexcept { return a.p != b.p; }
        };

        IO_CONSTEXPR list() noexcept = default;

        ~list() noexcept {
            clear();
        }

        list(const list&) = delete;
        list& operator=(const list&) = delete;

        list(list&& o) noexcept {
            _head = o._head;
            _tail = o._tail;
            _len = o._len;
            o._head = o._tail = nullptr;
            o._len = 0;
        }

        list& operator=(list&& o) noexcept {
            if (this == &o) return *this;
            this->~list();
            new (this) list(static_cast<list&&>(o));
            return *this;
        }

        // ---- size ----
        IO_NODISCARD usize size() const noexcept { return _len; }
        IO_NODISCARD bool empty() const noexcept { return _len == 0; }

        // ---- iterators ----
        iterator begin() noexcept { return iterator{ _head }; }
        iterator end() noexcept { return iterator{ nullptr }; }
        const_iterator begin() const noexcept { return const_iterator{ _head }; }
        const_iterator end() const noexcept { return const_iterator{ nullptr }; }

        // ---- access ----
        T& front() noexcept { return _head->value; }
        T& back() noexcept { return _tail->value; }
        const T& front() const noexcept { return _head->value; }
        const T& back() const noexcept { return _tail->value; }

        // ---- modifiers ----
        IO_NODISCARD bool push_back(const T& v) noexcept { return link_back(make_node(v)); }
        IO_NODISCARD bool push_back(T&& v) noexcept { return link_back(make_node(io::move(v))); }

        IO_NODISCARD bool push_front(const T& v) noexcept { return link_front(make_node(v)); }
        IO_NODISCARD bool push_front(T&& v) noexcept { return link_front(make_node(io::move(v))); }

        void pop_back() noexcept {
            if (!_tail) return;
            node* n = _tail;
            _tail = n->prev;
            if (_tail) _tail->next = nullptr;
            else _head = nullptr;
            destroy_node(n);
            --_len;
        }

        void pop_front() noexcept {
            if (!_head) return;
            node* n = _head;
            _head = n->next;
            if (_head) _head->prev = nullptr;
            else _tail = nullptr;
            destroy_node(n);
            --_len;
        }

        iterator erase(iterator it) noexcept {
            node* n = it.p;
            if (!n) return end();

            node* nx = n->next;

            if (n->prev) n->prev->next = n->next;
            else _head = n->next;

            if (n->next) n->next->prev = n->prev;
            else _tail = n->prev;

            destroy_node(n);
            --_len;
            return iterator{ nx };
        }

        void clear() noexcept {
            node* cur = _head;
            while (cur) {
                node* nx = cur->next;
                destroy_node(cur);
                cur = nx;
            }
            _head = _tail = nullptr;
            _len = 0;
        }

    private:
        node* _head{ nullptr };
        node* _tail{ nullptr };
        usize _len{ 0 };

        template<class U>
        node* make_node(U&& v) noexcept {
            void* mem = ::operator new(sizeof(node), std::nothrow);
            if (!mem) return nullptr;
            return new (mem) node(static_cast<U&&>(v));
        }

        bool link_back(node* n) noexcept {
            if (!n) return false;
            n->prev = _tail;
            n->next = nullptr;
            if (_tail) _tail->next = n;
            else _head = n;
            _tail = n;
            ++_len;
            return true;
        }

        bool link_front(node* n) noexcept {
            if (!n) return false;
            n->prev = nullptr;
            n->next = _head;
            if (_head) _head->prev = n;
            else _tail = n;
            _head = n;
            ++_len;
            return true;
        }

        static void destroy_node(node* n) noexcept {
            if (!n) return;
            n->~node();
            ::operator delete(n);
        }
    }; // struct list
} // namespace io
