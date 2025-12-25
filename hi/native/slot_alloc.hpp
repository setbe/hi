#pragma once
#include "types.hpp"
#include "syscalls.hpp"
#include "atomic.hpp"

#include <cstddef> // std::max_align_t


namespace io {
    namespace native {

        static constexpr usize DEFAULT_ALIGNMENT = alignof(std::max_align_t);

        struct BlockHeader {
            usize size;  // total bytes including header
        };
        static_assert(alignof(BlockHeader) >= DEFAULT_ALIGNMENT, "header align");
        static_assert(sizeof(BlockHeader) % DEFAULT_ALIGNMENT == 0, "payload alignment");

        struct FreeNode {
            FreeNode* next;
        };

        // ------------------------- Slot Pool ---------------------------------
        struct ISlotPool {
            virtual void* allocate() noexcept = 0;
            virtual void deallocate(void* ptr) noexcept = 0;
        };

        template <usize BlockSize>
        struct SlotPool : ISlotPool {
            atomic<FreeNode*> free_list = nullptr;

            explicit SlotPool() noexcept = default;

            void* allocate() noexcept override {
                FreeNode* node;
                do {
                    node = free_list.load(memory_order_acquire);
                    if (!node) return io::alloc(BlockSize);
                } while (!free_list.compare_exchange_strong(node, node->next, memory_order_acq_rel));
                return node;
            } // allocate

            void deallocate(void* ptr) noexcept override {
                FreeNode* old_head;
                FreeNode* node = static_cast<FreeNode*>(ptr);
                do {
                    old_head = free_list.load(memory_order_acquire);
                    node->next = old_head;
                } while (!free_list.compare_exchange_strong(old_head, node, memory_order_acq_rel));

            } // deallocate
        }; // struct SlotPool

        // ---------------------- Global Slot Pools ---------------------------
        static SlotPool<8>   pool8;
        static SlotPool<16>  pool16;
        static SlotPool<32>  pool32;
        static SlotPool<64>  pool64;
        static SlotPool<128> pool128;
        static SlotPool<256> pool256;

        // ---------------------- Helper --------------------------------------
        inline SlotPool<8>* select_pool8(usize size) { if (size <= 8) return &pool8; return nullptr; }
        inline SlotPool<16>* select_pool16(usize size) { if (size <= 16) return &pool16; return nullptr; }
        inline SlotPool<32>* select_pool32(usize size) { if (size <= 32) return &pool32; return nullptr; }
        inline SlotPool<64>* select_pool64(usize size) { if (size <= 64) return &pool64; return nullptr; }
        inline SlotPool<128>* select_pool128(usize size) { if (size <= 128) return &pool128; return nullptr; }
        inline SlotPool<256>* select_pool256(usize size) { if (size <= 256) return &pool256; return nullptr; }

        // ---------------------- Unified Select -----------------------------
        inline ISlotPool* pool_for_size(usize size) {
            if (size <= 8) return &pool8;
            if (size <= 16) return &pool16;
            if (size <= 32) return &pool32;
            if (size <= 64) return &pool64;
            if (size <= 128) return &pool128;
            if (size <= 256) return &pool256;
            return nullptr; // large object -> use io::alloc
        }

        static inline void* allocate_block(usize size) {
            const usize total = sizeof(BlockHeader) + size;

            if (auto pool = pool_for_size(total)) {
                void* raw = pool->allocate();
                if (!raw) return nullptr;

                auto* hdr = static_cast<BlockHeader*>(raw);
                hdr->size = total;
                return hdr + 1;
            }

            auto* hdr = static_cast<BlockHeader*>(io::alloc(total));
            if (!hdr) return nullptr;
            hdr->size = total;
            return hdr + 1;
        }

        static inline void deallocate_block(void* ptr) noexcept {
            if (!ptr) return;

            auto* hdr = static_cast<BlockHeader*>(ptr) - 1;
            const usize total = hdr->size;

            if (auto pool = pool_for_size(total))
                pool->deallocate(hdr);
            else
                io::free(hdr);
        }

    } // namespace native
} // namespace io