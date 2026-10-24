import pathlib
import sys

root = pathlib.Path(sys.argv[1]).resolve() / 'host'
p = root/'wasi.c'
data = p.read_text()
data = data.replace('static char* civ2_amiga_environment[16];', '''static char* civ2_amiga_environment[16];
static unsigned civ2_environment_count;
void civ2_set_environment(const char* name, const char* value) {
    if (civ2_environment_count >= 15) { fputs("Too many module settings\\n", stderr); exit(1); }
    size_t n = strlen(name) + strlen(value) + 2;
    char* entry = malloc(n);
    if (!entry) { perror("module environment"); exit(1); }
    snprintf(entry, n, "%s=%s", name, value);
    civ2_amiga_environment[civ2_environment_count++] = entry;
    civ2_amiga_environment[civ2_environment_count] = NULL;
}''', 1)
data = data.replace('    unsigned count = 0;', '    unsigned count = civ2_environment_count;', 1)
p.write_text(data)

p = root/'port_main.c'
data = p.read_text()
marker = '    /* The module\'s working directory starts at "/": name the game'
new = '''#ifdef __amigaos__
    extern void civ2_set_environment(const char*, const char*);
    /* Amiga launches do not populate the POSIX environ array. */
    for (int i = 2; i < argc; ++i) {
        const char* key = NULL;
        if (!strcmp(argv[i], "--show")) {
            civ2_set_environment("CIV2_SHOW", "1");
            continue;
        }
        if (!strcmp(argv[i], "--data")) key = "CIV2_DATA";
        else if (!strcmp(argv[i], "--replay")) key = "CIV2_SCRIPT";
        else if (!strcmp(argv[i], "--shots")) key = "CIV2_SHOT_DIR";
        else { fprintf(stderr, "Unknown option: %s\\n", argv[i]); return 2; }
        if (++i >= argc) { fprintf(stderr, "Missing option value\\n"); return 2; }
        char path[4096];
        module_path(argv[i], path, sizeof path);
        civ2_set_environment(key, path);
    }
#endif
    /* The module's working directory starts at "/": name the game'''
assert data.count(marker) == 1
p.write_text(data.replace(marker, new, 1))

p = root/'host_sdl.c'
data = p.read_text().replace('#include <string.h>', '#include <string.h>\n#include <unistd.h>', 1)
marker = '    char *base = SDL_GetBasePath();'
new = '''#ifdef __amigaos__
    if (getcwd(buf, (size_t)size)) {
        size_t n = strlen(buf);
        if (n + 1 < (size_t)size) { buf[n] = '/'; buf[n+1] = 0; }
        return (int)strlen(buf);
    }
#endif
    char *base = SDL_GetBasePath();'''
assert data.count(marker) == 1
p.write_text(data.replace(marker, new, 1))
print('Added explicit native --data/--replay/--shots/--show options')
