#ifndef SRC_HPP
#define SRC_HPP

#include <vector>

int* getNewBlock(int n);
void freeBlock(const int* block, int n);

class Allocator {
private:
    struct Block {
        int* ptr;
        int n;
        int total_ints;
        int allocated_ints;
        int active_allocations;
        bool is_freed;
    };

    struct Allocation {
        int* ptr;
        int size;
        int block_index;
    };

    std::vector<Block> blocks;
    std::vector<Allocation> allocs;
    int current_block_index;

public:
    Allocator() {
        current_block_index = -1;
    }

    ~Allocator() {
        for (auto& blk : blocks) {
            if (!blk.is_freed) {
                freeBlock(blk.ptr, blk.n);
                blk.is_freed = true;
            }
        }
    }

    int* allocate(int n) {
        if (current_block_index != -1) {
            Block& blk = blocks[current_block_index];
            if (blk.total_ints - blk.allocated_ints >= n) {
                int* ptr = blk.ptr + blk.allocated_ints;
                blk.allocated_ints += n;
                blk.active_allocations++;
                allocs.push_back({ptr, n, current_block_index});
                return ptr;
            }
        }

        if (current_block_index != -1) {
            Block& old_blk = blocks[current_block_index];
            if (old_blk.active_allocations == 0 && !old_blk.is_freed) {
                freeBlock(old_blk.ptr, old_blk.n);
                old_blk.is_freed = true;
            }
        }

        int num_units = (n + 1023) / 1024;
        int* ptr = getNewBlock(num_units);
        blocks.push_back({ptr, num_units, num_units * 1024, n, 1, false});
        current_block_index = blocks.size() - 1;
        allocs.push_back({ptr, n, current_block_index});
        return ptr;
    }

    void deallocate(int* pointer, int n) {
        int blk_idx = -1;
        for (int i = allocs.size() - 1; i >= 0; --i) {
            if (allocs[i].ptr == pointer) {
                blk_idx = allocs[i].block_index;
                allocs[i].ptr = nullptr;
                break;
            }
        }

        if (blk_idx != -1) {
            blocks[blk_idx].active_allocations--;

            int max_end = 0;
            for (const auto& alloc : allocs) {
                if (alloc.ptr != nullptr && alloc.block_index == blk_idx) {
                    int end = (alloc.ptr - blocks[blk_idx].ptr) + alloc.size;
                    if (end > max_end) {
                        max_end = end;
                    }
                }
            }
            blocks[blk_idx].allocated_ints = max_end;

            if (blocks[blk_idx].active_allocations == 0 && blk_idx != current_block_index) {
                if (!blocks[blk_idx].is_freed) {
                    freeBlock(blocks[blk_idx].ptr, blocks[blk_idx].n);
                    blocks[blk_idx].is_freed = true;
                }
            }
        }
    }
};

#endif // SRC_HPP
