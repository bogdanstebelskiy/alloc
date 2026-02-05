#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define ALIGNMENT alignof(void *)
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define BLK_HEADER_SIZE (ALIGN(sizeof(struct blk_header)))

void visualize_heap();

static void *heap_start = NULL;

struct blk_header {
  size_t size;
  struct blk_header *next;
  struct blk_header *prev;
  int free;
};

void split_block(struct blk_header *block, size_t size) {
  size_t remaining = block->size - size;

  if (remaining > BLK_HEADER_SIZE + ALIGNMENT) {
    struct blk_header *new_block =
        (struct blk_header *)((char *)block + BLK_HEADER_SIZE + size);

    new_block->size = remaining - BLK_HEADER_SIZE;
    new_block->free = 1;
    new_block->next = block->next;
    new_block->prev = block;

    if (block->next) {
      block->next->prev = new_block;
    }

    block->size = size;
    block->next = new_block;
  }
}

void *find_fit(size_t size) {
  struct blk_header *header = (struct blk_header *)heap_start;
  while (header != NULL) {
    if (header->free && header->size >= size) {
      if (header->size > size + BLK_HEADER_SIZE + ALIGNMENT) {
        split_block(header, size);
      }
      return header;
    }

    header = header->next;
  }

  return NULL;
}

void *my_malloc(size_t size) {
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
    header->prev = NULL;

    if (!heap_start) {
      heap_start = header;
    } else {
      struct blk_header *last = (struct blk_header *)heap_start;

      while (last->next) {
        last = last->next;
      }

      last->next = header;
      header->prev = last;
    }
  }

  header->free = 0;

  return (void *)((char *)header + BLK_HEADER_SIZE);
}

void my_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  struct blk_header *header =
      (struct blk_header *)((char *)ptr - BLK_HEADER_SIZE);
  header->free = 1;
}

int main() {
  printf("BLK_HEADER_SIZE = %zu bytes\n", BLK_HEADER_SIZE);
  printf("ALIGNMENT = %zu bytes\n\n", ALIGNMENT);

  // Step 1: Allocate a large block
  printf("1. Allocate 200 bytes:\n");
  char *a = my_malloc(200);
  visualize_heap();
  printf("\n");

  // Step 2: Free it
  printf("2. Free the 200-byte block:\n");
  my_free(a);
  visualize_heap();
  printf("\n");

  // Step 3: Allocate small block (should split!)
  printf("3. Allocate 50 bytes (should split the 200-byte block):\n");
  char *b = my_malloc(50);
  visualize_heap();
  printf("\n");

  // Step 4: Allocate another small block (should reuse free block)
  printf("4. Allocate 30 bytes (should use the remaining free block):\n");
  char *c = my_malloc(30);
  visualize_heap();
  printf("\n");

  // Step 5: Allocate exact fit
  printf("5. Allocate 20 bytes:\n");
  char *d = my_malloc(20);
  visualize_heap();
  printf("\n");

  // Step 6: Free middle block
  printf("6. Free the 50-byte block (middle one):\n");
  my_free(b);
  visualize_heap();
  printf("\n");

  // Step 7: Try to allocate exact size (no split)
  printf("7. Allocate 50 bytes (exact fit, no split):\n");
  char *e = my_malloc(50);
  visualize_heap();
  printf("\n");

  // Step 8: Allocate slightly smaller (no split due to small remainder)
  printf("8. Free 50-byte block and allocate 45 bytes:\n");
  my_free(e);
  visualize_heap();
  char *f = my_malloc(45);
  visualize_heap();
  printf("\n");

  // Step 9: Show final state
  printf("9. Final heap state:\n");
  visualize_heap();

  return 0;
}
void visualize_heap() {
  printf("\nHeap: ");
  struct blk_header *h = heap_start;
  while (h) {
    printf("[%s:%zu] ", h->free ? "F" : "A", h->size);
    if (h->next)
      printf("-> ");
    h = h->next;
  }
  printf("\n");
}
