import pathlib
import sys

root = pathlib.Path(sys.argv[1]).resolve() / 'host'
p = root / 'wasi.c'
data = p.read_text()
marker = '/* WASI error numbers */'
helper = '''/* The native C library has iovec types but no readv/writev entry points. */
static long civ2_iov_io(int fd, const struct iovec* buffers, int n, int writing) {
    long total = 0;
    for (int i = 0; i < n; ++i) {
        ssize_t count = writing ? write(fd, buffers[i].iov_base, buffers[i].iov_len)
                                : read(fd, buffers[i].iov_base, buffers[i].iov_len);
        if (count < 0) return total ? total : -1;
        total += count;
        if ((size_t)count < buffers[i].iov_len) break;
    }
    return total;
}

'''
assert marker in data and 'civ2_iov_io' not in data
data = data.replace(marker, helper + marker, 1)
data = data.replace('r = writing ? writev(f->host, v, (int)n) : readv(f->host, v, (int)n);',
                    'r = civ2_iov_io(f->host, v, (int)n, writing);')
marker = '    return s;\n}\n'
new = '''#ifdef __amigaos__
    /* Virtual /Volume/path is a native Volume:path. */
    if (s[0] == '/' && s[1]) {
        char* slash = strchr(s + 1, '/');
        char* colon = strchr(s + 1, ':');
        if (colon && (!slash || colon < slash)) {
            memmove(s, s + 1, strlen(s));
        } else if (slash) {
            memmove(s, s + 1, strlen(s));
            s[slash - s - 1] = ':';
        }
    }
#endif
    return s;
}
'''
assert data.count(marker) == 1
p.write_text(data.replace(marker, new, 1))

p = root / 'port_main.c'
data = p.read_text()
marker = '    snprintf(out, n, "%s", in);'
new = '''#ifdef __amigaos__
    const char* colon = strchr(in, ':');
    if (colon && n > 1) {
        snprintf(out, n, "/%s", in);
        if ((size_t)(colon - in + 1) < n) out[colon - in + 1] = '/';
        return;
    }
#endif
    snprintf(out, n, "%s", in);'''
assert data.count(marker) == 1
p.write_text(data.replace(marker, new, 1))
print('Prepared native vectored I/O fallback and Amiga volume mapping')
