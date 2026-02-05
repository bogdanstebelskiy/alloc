#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define ALIGNMENT alignof(void *)
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define BLK_HEADER_SIZE (ALIGN(sizeof(struct blk_header)))

static void *heap_start = NULL;

struct blk_header {
  size_t size;
  struct blk_header *next;
  // also better to keep prev too
  int free;
};

void *find_fit(size_t size) {
  struct blk_header *header = (struct blk_header *)heap_start;
  while (header != NULL) {
    if (header->free && header->size >= size) {
      return header;
    }

    header = header->next;
  }

  return NULL;
}

void *malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  size_t blk_size = ALIGN(size + BLK_HEADER_SIZE);

  struct blk_header *header = find_fit(size);

  if (header == NULL) {
    void *res = sbrk(blk_size);

    if (res == (void *)-1) {
      perror("Error calling sbrk");
      return NULL;
    }

    header = (struct blk_header *)res;
    header->size = size; // only user usable memory, header is not included
    header->next = NULL;

    if (!heap_start) {
      heap_start = header;
    } else {
      struct blk_header *last = (struct blk_header *)heap_start;

      if (last != header) {
        while (last->next) {
          last = last->next;
        }

        last->next = header;
      }
    }
  }

  header->free = 0;

  return (void *)((char *)header + BLK_HEADER_SIZE);
}

void free(void *ptr) {
  struct blk_header *header =
      (struct blk_header *)((char *)ptr - BLK_HEADER_SIZE);
  header->free = 1;
}

int main() {
  int *a = malloc(10 * sizeof(int));
  char *b = malloc(20);
  double *c = malloc(5 * sizeof(double));

  printf("Allocated blocks:\n");
  printf("a = %p, align check: %zu\n", (void *)a,
         (uintptr_t)a % alignof(void *));
  printf("b = %p, align check: %zu\n", (void *)b,
         (uintptr_t)b % alignof(void *));
  printf("c = %p, align check: %zu\n", (void *)c,
         (uintptr_t)c % alignof(void *));

  // Free first block
  free(a);
  printf("Freed block a\n");

  // Allocate again (should reuse a)
  int *d = malloc(5 * sizeof(int));
  printf("d = %p, align check: %zu (should reuse a)\n", (void *)d,
         (uintptr_t)d % alignof(void *));

  // Show all headers
  struct blk_header *header = heap_start;
  int i = 0;
  printf("\nHeap headers:\n");
  while (header != NULL) {
    printf("Block %d: %p, size=%zu, free=%d\n", i, (void *)header, header->size,
           header->free);
    header = header->next;
    i++;
  }

  return 0;
}
