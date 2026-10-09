#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve()

json_h = root / "deps" / "json.hpp"
text = json_h.read_text(encoding="utf-8")

if "#include <cerrno> // AmigaChrome" not in text:
    marker = "#include <cstddef> // nullptr_t, ptrdiff_t, size_t\n"
    if marker not in text:
        raise RuntimeError("json.hpp include marker missing")
    text = text.replace(
        marker,
        marker + "#include <cerrno> // AmigaChrome: errno/ERANGE for libnix numeric parsing\n"
                 "#include <cstdlib> // AmigaChrome: global C numeric conversion functions\n",
        1,
    )

repls = {
    "f = std::strtof(str, endptr);":
        "f = static_cast<float>(::strtod(str, endptr));",
    "f = std::strtod(str, endptr);":
        "f = ::strtod(str, endptr);",
    "f = std::strtold(str, endptr);":
        "f = static_cast<long double>(::strtod(str, endptr));",
    "std::strtoull(token_buffer.data(), &endptr, 10)":
        "::strtoull(token_buffer.data(), &endptr, 10)",
    "std::strtoll(token_buffer.data(), &endptr, 10)":
        "::strtoll(token_buffer.data(), &endptr, 10)",
}
for before, after in repls.items():
    text = text.replace(before, after)

old = '''        std::size_t processed_chars = 0;
        unsigned long long res = 0;
        JSON_TRY
        {
            res = std::stoull(s, &processed_chars);
        }
        JSON_CATCH(std::out_of_range&)
        {
            JSON_THROW(detail::out_of_range::create(404, "unresolved reference token '" + s + "'"));
        }
'''
new = '''        std::size_t processed_chars = 0;
        unsigned long long res = 0;
#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
        char* amiga_endptr = nullptr;
        errno = 0;
        res = ::strtoull(s.c_str(), &amiga_endptr, 10);
        processed_chars = amiga_endptr != nullptr
            ? static_cast<std::size_t>(amiga_endptr - s.c_str())
            : 0;
        if (errno == ERANGE)
        {
            JSON_THROW(detail::out_of_range::create(404, "unresolved reference token '" + s + "'"));
        }
#else
        JSON_TRY
        {
            res = std::stoull(s, &processed_chars);
        }
        JSON_CATCH(std::out_of_range&)
        {
            JSON_THROW(detail::out_of_range::create(404, "unresolved reference token '" + s + "'"));
        }
#endif
'''
if old not in text and "amiga_endptr" not in text:
    raise RuntimeError("json.hpp stoull block marker missing")
text = text.replace(old, new, 1)
json_h.write_text(text, encoding="utf-8")

spd = root / "deps" / "spdlog" / "details" / "os-inl.h"
text = spd.read_text(encoding="utf-8")
old = "#elif defined(__VITA__) || defined(__SWITCH__)\n"
new = "#elif defined(__VITA__) || defined(__SWITCH__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)\n"
if old not in text and new not in text:
    raise RuntimeError("spdlog timezone fallback marker missing")
text = text.replace(old, new, 1)
spd.write_text(text, encoding="utf-8")

cmake = root / "CMakeLists.txt"
text = cmake.read_text(encoding="utf-8")
old_link = "target_link_libraries(nx ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${PNG_LIBRARY} ${JPEG_LIBRARY})"
new_link = "target_link_libraries(nx ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${PNG_LIBRARY} ${JPEG_LIBRARY} ${ZLIB_LIBRARY})"
if old_link in text:
    text = text.replace(old_link, new_link, 1)
elif new_link not in text:
    raise RuntimeError("NXEngine target link marker missing")
cmake.write_text(text, encoding="utf-8")

print("patched NXEngine-evo bundled json/spdlog and static PNG/zlib link for AmigaOS 3")
