#pragma once
#include "types.hpp"
#include "syscalls.hpp"
#include "atomic.hpp"

namespace io {
    namespace native {

        struct BlockHeader {
            usize size;  // request real size
        };

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
            if (auto pool = pool_for_size(size)) {
                void* user_ptr = pool->allocate();
                BlockHeader* hdr = reinterpret_cast<BlockHeader*>(user_ptr);
                hdr->size = size;
                return reinterpret_cast<char*>(hdr + 1); // return to user after header
            }
            else {
                // Large data via io::alloc
                BlockHeader* hdr = static_cast<BlockHeader*>(io::alloc(sizeof(BlockHeader) + size));
                if (!hdr) return nullptr;
                hdr->size = size;
                return reinterpret_cast<char*>(hdr + 1);
            }
        }

        static inline void deallocate_block(void* ptr) noexcept {
            if (!ptr) return;
            BlockHeader* hdr = reinterpret_cast<BlockHeader*>(ptr) - 1;
            usize size = hdr->size;

            if (auto pool = pool_for_size(size))
                pool->deallocate(hdr);
            else
                io::free(hdr);
        }
    } // namespace native
} // namespace io