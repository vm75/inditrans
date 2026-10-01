// glibc-only LD_PRELOAD allocator event and requested-live-byte probe for memory.cpp.
#define _GNU_SOURCE

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

extern void* __libc_malloc(size_t);
extern void* __libc_calloc(size_t, size_t);
extern void* __libc_realloc(void*, size_t);
extern void __libc_free(void*);

// The benchmark is single-threaded; avoid atomics in this diagnostic interposer.
static volatile uint64_t active;
static volatile uint64_t malloc_calls;
static volatile uint64_t calloc_calls;
static volatile uint64_t realloc_calls;
static volatile uint64_t free_calls;
static volatile uint64_t malloc_bytes;
static volatile uint64_t calloc_bytes;
static volatile uint64_t realloc_bytes;
static uint64_t tracking_overflow;
static uint64_t live_bytes;
static uint64_t peak_live_bytes;

enum { TRACKED_ALLOCATION_CAPACITY = 32768 };
struct TrackedAllocation {
  void* pointer;
  size_t size;
  uint8_t state;
};
static struct TrackedAllocation tracked_allocations[TRACKED_ALLOCATION_CAPACITY];

static size_t pointer_hash(const void* pointer) {
  uintptr_t value = (uintptr_t)pointer >> 4;
  value ^= value >> 17;
  value *= UINT64_C(0xed5ad4bb);
  value ^= value >> 11;
  return (size_t)(value % TRACKED_ALLOCATION_CAPACITY);
}

static size_t find_tracked_slot(const void* pointer, int for_insert) {
  size_t first_deleted = TRACKED_ALLOCATION_CAPACITY;
  const size_t start = pointer_hash(pointer);
  for (size_t offset = 0; offset < TRACKED_ALLOCATION_CAPACITY; ++offset) {
    const size_t slot = (start + offset) % TRACKED_ALLOCATION_CAPACITY;
    const struct TrackedAllocation* entry = &tracked_allocations[slot];
    if (entry->state == 0) {
      return for_insert && first_deleted != TRACKED_ALLOCATION_CAPACITY ? first_deleted : slot;
    }
    if (entry->state == 1 && entry->pointer == pointer) return slot;
    if (for_insert && entry->state == 2 && first_deleted == TRACKED_ALLOCATION_CAPACITY) first_deleted = slot;
  }
  return first_deleted;
}

static void track_allocation(void* pointer, size_t size) {
  if (pointer == NULL) return;
  const size_t slot = find_tracked_slot(pointer, 1);
  if (slot == TRACKED_ALLOCATION_CAPACITY) {
    ++tracking_overflow;
    return;
  }
  struct TrackedAllocation* entry = &tracked_allocations[slot];
  if (entry->state == 1) live_bytes -= entry->size;
  entry->pointer = pointer;
  entry->size = size;
  entry->state = 1;
  live_bytes += size;
  if (live_bytes > peak_live_bytes) peak_live_bytes = live_bytes;
}

static void untrack_allocation(void* pointer) {
  if (pointer == NULL) return;
  const size_t slot = find_tracked_slot(pointer, 0);
  if (slot == TRACKED_ALLOCATION_CAPACITY) return;
  struct TrackedAllocation* entry = &tracked_allocations[slot];
  if (entry->state != 1) return;
  live_bytes -= entry->size;
  entry->state = 2;
}

void* malloc(size_t size) {
  void* pointer = __libc_malloc(size);
  if (active) {
    ++malloc_calls;
    malloc_bytes += size;
    track_allocation(pointer, size);
  }
  return pointer;
}

void* calloc(size_t count, size_t size) {
  void* pointer = __libc_calloc(count, size);
  if (active) {
    ++calloc_calls;
    calloc_bytes += count * size;
    track_allocation(pointer, count * size);
  }
  return pointer;
}

void* realloc(void* memory, size_t size) {
  void* pointer = __libc_realloc(memory, size);
  if (active) {
    ++realloc_calls;
    realloc_bytes += size;
    if (pointer != NULL || size == 0) untrack_allocation(memory);
    track_allocation(pointer, size);
  }
  return pointer;
}

void free(void* memory) {
  if (active) {
    ++free_calls;
    untrack_allocation(memory);
  }
  __libc_free(memory);
}

void alloc_probe_reset(void) {
  malloc_calls = 0;
  calloc_calls = 0;
  realloc_calls = 0;
  free_calls = 0;
  malloc_bytes = 0;
  calloc_bytes = 0;
  realloc_bytes = 0;
  tracking_overflow = 0;
  active = 1;
  live_bytes = 0;
  peak_live_bytes = 0;
  for (size_t i = 0; i < TRACKED_ALLOCATION_CAPACITY; ++i) {
    tracked_allocations[i].state = 0;
  }
}

void alloc_probe_report(void) {
  active = 0;
  char output[352];
  const int length = snprintf(output, sizeof(output),
      "malloc=%llu,%llu calloc=%llu,%llu realloc=%llu,%llu free=%llu live=%llu peak=%llu untracked=%llu\n",
      (unsigned long long)malloc_calls, (unsigned long long)malloc_bytes,
      (unsigned long long)calloc_calls, (unsigned long long)calloc_bytes,
      (unsigned long long)realloc_calls, (unsigned long long)realloc_bytes,
      (unsigned long long)free_calls, (unsigned long long)live_bytes, (unsigned long long)peak_live_bytes,
      (unsigned long long)tracking_overflow);
  if (length > 0) write(STDERR_FILENO, output, (size_t)length);
}
