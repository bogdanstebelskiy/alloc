#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define ALIGNMENT alignof(void *)
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define BLK_HEADER_SIZE (ALIGN(sizeof(struct blk_header)))

static struct blk_header *heap_start = NULL;
static struct blk_header *heap_end = NULL;

struct blk_header {
  size_t size;
  struct blk_header *next;
  struct blk_header *prev;
  int free;
};

static void split_block(struct blk_header *block, size_t size) {
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
    } else {
      heap_end = new_block;
    }

    block->size = size;
    block->next = new_block;
  }
}

static void *find_fit(size_t size) {
  struct blk_header *header = heap_start;

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

static void *malloc(size_t size) {
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
      heap_end = header;
    } else {
      heap_end->next = header;
      header->prev = heap_end;
      heap_end = header;
    }
  }

  header->free = 0;

  return (void *)((char *)header + BLK_HEADER_SIZE);
}

static void coalesce(struct blk_header *block) {
  if (block->next && block->next->free) {
    block->size += BLK_HEADER_SIZE + block->next->size;

    if (block->next == heap_end) {
      heap_end = block;
    }

    block->next = block->next->next;

    if (block->next) {
      block->next->prev = block;
    }
  }

  if (block->prev && block->prev->free) {
    block->prev->size += BLK_HEADER_SIZE + block->size;

    if (block == heap_end) {
      heap_end = block->prev;
    }

    block->prev->next = block->next;
    if (block->next) {
      block->next->prev = block->prev;
    }
  }
}

static void free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  struct blk_header *header =
      (struct blk_header *)((char *)ptr - BLK_HEADER_SIZE);
  header->free = 1;

  coalesce(header);
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

int main() {
  printf("BLK_HEADER_SIZE = %zu bytes\n", BLK_HEADER_SIZE);
  printf("ALIGNMENT = %zu bytes\n\n", ALIGNMENT);

  printf("1. Allocate three blocks:\n");
  char *a = malloc(50);
  char *b = malloc(30);
  char *c = malloc(20);
  visualize_heap();
  printf("\n");

  printf("2. Free middle block:\n");
  free(b);
  visualize_heap();
  printf("\n");

  printf("3. Free first block (should coalesce with middle):\n");
  free(a);
  visualize_heap();
  printf("\n");

  printf("4. Free last block (should coalesce everything):\n");
  free(c);
  visualize_heap();
  printf("\n");

  printf("5. Allocate large block (should reuse coalesced space):\n");
  char *d = malloc(80);
  (void)d;
  visualize_heap();

  return 0;
}
