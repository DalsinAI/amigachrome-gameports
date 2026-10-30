import pathlib
import sys

root = pathlib.Path(sys.argv[1]).resolve() / 'host'
p = root / 'port.h'
data = p.read_text()
start = data.index('static inline u32 GET32(')
end = data.index('\nvoid wasi_init', start)
data = data[:start] + '''static inline u32 GET32(u32 a) {
    const u8* p = MEM(a);
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}
static inline void host_put_le(void* target, u64 value, unsigned n) {
    u8* p = target;
    for (unsigned i = 0; i < n; ++i) p[i] = (u8)(value >> (8 * i));
}
static inline void PUT32(u32 a, u32 v) { host_put_le(MEM(a), v, 4); }
static inline void PUT64(u32 a, u64 v) { host_put_le(MEM(a), v, 8); }
''' + data[end:]
p.write_text(data)

p = root / 'port_main.c'
data = p.read_text()
old = '''    if (got)
        memcpy(MEM(out), e, sizeof *e);     /* five int32s, same layout */'''
new = '''    if (got) {
        PUT32(out, (u32)e->type);
        PUT32(out + 4, (u32)e->vk);
        PUT32(out + 8, (u32)e->x);
        PUT32(out + 12, (u32)e->y);
        PUT32(out + 16, (u32)e->button);
    }'''
assert data.count(old) == 1
p.write_text(data.replace(old, new))

p = root / 'host_sdl.c'
data = p.read_text().replace('SDL_PIXELFORMAT_ARGB8888', 'SDL_PIXELFORMAT_BGRA32')
p.write_text(data)

p = root / 'wasi.c'
data = p.read_text()
for old, new in [
    ('memcpy(hdr, &k, 8);', 'host_put_le(hdr, k, 8);'),
    ('memcpy(hdr + 8, &ino, 8);', 'host_put_le(hdr + 8, ino, 8);'),
    ('memcpy(hdr + 16, &n, 4);', 'host_put_le(hdr + 16, n, 4);')
]:
    assert data.count(old) == 1
    data = data.replace(old, new)
p.write_text(data)
print('Prepared scalar/event/directory LE boundary and byte-defined pixel formats')
