import pathlib
import sys

p = pathlib.Path(sys.argv[1]).resolve() / 'host/wasi.c'
data = p.read_text()
marker = 'static char **argv_;'
replacement = '''static char **argv_;
#ifdef __amigaos__
static char* civ2_amiga_environment[16];
static void civ2_read_environment(void) {
    static const char* names[] = {"CIV2_DATA", "CIV2_SCRIPT", "CIV2_SHOW",
        "CIV2_SHOT_DIR", "CIV2_WINDIR", "CIV2_SCALE", "CIV2_FULLSCREEN",
        "CIV2_WINDOW", "CIV2_CLASSIC", "CIV2_NORMAL_DESKTOP", "CIV2_RAND_LOG"};
    unsigned count = 0;
    for (unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++i) {
        const char* value = getenv(names[i]);
        if (value && *value && count < 15) {
            size_t n = strlen(names[i]) + strlen(value) + 2;
            char* entry = malloc(n);
            if (!entry) { perror("module environment"); exit(1); }
            snprintf(entry, n, "%s=%s", names[i], value);
            civ2_amiga_environment[count++] = entry;
        }
    }
    civ2_amiga_environment[count] = NULL;
    fprintf(stderr, "civ2: imported %u native environment settings\\n", count);
}
#endif'''
assert data.count(marker) == 1
data = data.replace(marker, replacement, 1)
data = data.replace('    argv_ = argv;', '''    argv_ = argv;
#ifdef __amigaos__
    civ2_read_environment();
#endif''', 1)
data = data.replace('#ifndef _WIN32\nextern char **environ;\n#endif', '''#ifdef __amigaos__
#define environ civ2_amiga_environment
#elif !defined(_WIN32)
extern char **environ;
#endif''', 1)
p.write_text(data)
print('Prepared explicit Amiga environment import into WASI')
