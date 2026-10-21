// Copyright (c) 2026 AmigaChrome Team. MIT licence.
//
// AstroMenace's malloc on AmigaOS 3.x: size classes with free lists.
//
// libnix's malloc takes a semaphore, then walks a list of 64 KB-ish
// MemHeaders calling exec's Allocate on each (first fit), and free walks
// the same list and calls Deallocate (address-ordered insert). The game's
// C++ news and deletes small objects every frame, and on Instance-32
// (21 Oct 2026) about 30% of the 68040's time went into exec's Allocate,
// Deallocate, AllocMem, the semaphore and Disable/Enable.
//
// Here a request up to 4 KB takes the head of its class's free list, or
// a fresh slice of a 64 KB arena; free puts it back on the list. Larger
// requests are AllocMem blocks of their own. Everything sits under
// Forbid/Permit (SDL's audio and the game's pthreads also allocate), and
// nothing in the list paths calls exec. Arenas and large blocks are given
// back at exit, in libnix's malloc exit slot.

#include <exec/memory.h>
#include <proto/exec.h>
#include <stabs.h>
#include <stddef.h>
#include <string.h>

namespace {

const unsigned kClasses = 18;
const unsigned long kClassSize[kClasses] = {
    8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096};
const unsigned long kSmallMax = 4096;
const unsigned long kArenaSize = 65536;
const unsigned long kLarge = 0xffffffffUL;

// Every block has its tag in the longword before it: the class index + 1,
// or kLarge. A small block's header is 8 bytes (keeps 8-byte alignment);
// a large block's is 16: its list links and its AllocMem size.
struct Small { unsigned long pad, tag; };
struct Large { Large *next, *prev; unsigned long bytes, tag; };
struct Free { Free *next; };
struct Arena { Arena *next; unsigned long pad; };

Free *free_list[kClasses];
Arena *arenas;
char *bump, *bump_end;
Large *larges;

unsigned class_of(unsigned long n)
{
    unsigned c = 0;
    while (kClassSize[c] < n)
        ++c;
    return c;
}

void *alloc_small(unsigned c)
{
    if (Free *f = free_list[c]) {
        free_list[c] = f->next;
        return f;
    }
    unsigned long need = sizeof(Small) + kClassSize[c];
    if ((unsigned long)(bump_end - bump) < need) {
        Arena *a = (Arena *)AllocMem(kArenaSize, MEMF_ANY);
        if (!a)
            return NULL;
        a->next = arenas;
        arenas = a;
        bump = (char *)(a + 1);
        bump_end = (char *)a + kArenaSize;
    }
    Small *s = (Small *)bump;
    bump += need;
    s->tag = c + 1;
    return s + 1;
}

void *alloc_large(unsigned long n)
{
    unsigned long bytes = sizeof(Large) + n;
    if (bytes < n)
        return NULL;
    Large *l = (Large *)AllocMem(bytes, MEMF_ANY);
    if (!l)
        return NULL;
    l->bytes = bytes;
    l->tag = kLarge;
    l->prev = NULL;
    l->next = larges;
    if (larges)
        larges->prev = l;
    larges = l;
    return l + 1;
}

unsigned long capacity(void *p)
{
    unsigned long tag = ((unsigned long *)p)[-1];
    return tag == kLarge ? ((Large *)p - 1)->bytes - sizeof(Large) : kClassSize[tag - 1];
}

void malloc_exit()
{
    while (larges) {
        Large *l = larges;
        larges = l->next;
        FreeMem(l, l->bytes);
    }
    while (arenas) {
        Arena *a = arenas;
        arenas = a->next;
        FreeMem(a, kArenaSize);
    }
    for (unsigned c = 0; c < kClasses; ++c)
        free_list[c] = NULL;
    bump = bump_end = NULL;
}

} // namespace

extern "C" {

void *malloc(size_t n)
{
    if (!n)
        n = 1;
    Forbid();
    void *p = n <= kSmallMax ? alloc_small(class_of(n)) : alloc_large(n);
    Permit();
    return p;
}

void free(void *p)
{
    if (!p)
        return;
    unsigned long tag = ((unsigned long *)p)[-1];
    Forbid();
    if (tag == kLarge) {
        Large *l = (Large *)p - 1;
        if (l->prev)
            l->prev->next = l->next;
        else
            larges = l->next;
        if (l->next)
            l->next->prev = l->prev;
        Permit();
        FreeMem(l, l->bytes);
        return;
    }
    Free *f = (Free *)p;
    f->next = free_list[tag - 1];
    free_list[tag - 1] = f;
    Permit();
}

void *realloc(void *p, size_t n)
{
    if (!p)
        return malloc(n);
    if (!n) {
        free(p);
        return NULL;
    }
    unsigned long cap = capacity(p);
    if (n <= cap && (cap <= kSmallMax || n > cap / 2))
        return p;
    void *q = malloc(n);
    if (q) {
        memcpy(q, p, n < cap ? n : cap);
        free(p);
    }
    return q;
}

void *calloc(size_t count, size_t size)
{
    size_t n = count * size;
    if (size && n / size != count)
        return NULL;
    void *p = malloc(n);
    if (p)
        memset(p, 0, n);
    return p;
}

void malloc_amigachrome_exit(void) { malloc_exit(); }   // named in the exit list below

} // extern "C"

// libnix gives its own malloc's memory back from this slot (exit list,
// priority -50), after the program's other exit work; ours goes in the same.
ADD2EXIT(malloc_amigachrome_exit, -50);
