#!/usr/bin/env python3
"""Teach C-Dogs path helpers about AmigaDOS volume and PROGDIR syntax.

The upstream Unix helper considers only a leading slash absolute.  On AmigaOS,
`PROGDIR:data/...` is absolute too.  Treating it as relative produced paths such
as `Work:CDogs-FLIGHT/PROGDIR:data/...`, which were then resolved a second time
and became `Work:CDogs-FLIGHT/Work:CDogs-FLIGHT/PROGDIR:data/...`.
"""

from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_amiga_paths.py CDOGS_SOURCE_ROOT")

root = Path(sys.argv[1]).resolve()
source = root / "src" / "cdogs" / "utils.c"
if not source.is_file():
    raise RuntimeError(f"C-Dogs utils.c missing: {source}")

text = source.read_text(encoding="utf-8")

old_data = r'''void GetDataFilePath(char *buf, const char *path)
{
	if (IsAbsolutePath(path))
	{
		strcpy(buf, path);
		return;
	}
	char relbuf[CDOGS_PATH_MAX];
	// Don't bother prepending CWD if data dir already an absolute path
	if (IsAbsolutePath(CDOGS_DATA_DIR))
	{
		sprintf(relbuf, "%s%s", CDOGS_DATA_DIR, path);
	}
	else
	{
		char cwd[CDOGS_PATH_MAX];
		if (CDogsGetCWD(cwd) == NULL)
		{
			fprintf(stderr, "Error getting CWD; %s\n", strerror(errno));
			strcpy(cwd, "");
		}
		sprintf(relbuf, "%s/%s%s", cwd, CDOGS_DATA_DIR, path);
	}
	RealPath(relbuf, buf);
}
'''

new_data = r'''void GetDataFilePath(char *buf, const char *path)
{
	if (IsAbsolutePath(path))
	{
		strcpy(buf, path);
		return;
	}
	char relbuf[CDOGS_PATH_MAX];
	// Don't bother prepending CWD if data dir already an absolute path
	if (IsAbsolutePath(CDOGS_DATA_DIR))
	{
		snprintf(relbuf, sizeof relbuf, "%s%s", CDOGS_DATA_DIR, path);
#ifdef __amigaos__
		/* AmigaDOS assigns and volumes are already fully resolved namespaces.
		 * Passing PROGDIR: through Unix realpath() prepends the current volume
		 * and can duplicate the working directory. */
		FixPathSeparator(buf, relbuf);
		return;
#endif
	}
	else
	{
		char cwd[CDOGS_PATH_MAX];
		if (CDogsGetCWD(cwd) == NULL)
		{
			fprintf(stderr, "Error getting CWD; %s\n", strerror(errno));
			strcpy(cwd, "");
		}
		snprintf(relbuf, sizeof relbuf, "%s/%s%s", cwd, CDOGS_DATA_DIR, path);
	}
	RealPath(relbuf, buf);
}
'''

old_abs = r'''static bool IsAbsolutePath(const char *path)
{
#ifdef _WIN32
	return strlen(path) > 1 && path[1] == ':';
#else
	return path[0] == '/';
#endif
}
'''

new_abs = r'''static bool IsAbsolutePath(const char *path)
{
#ifdef _WIN32
	return strlen(path) > 1 && path[1] == ':';
#elif defined(__amigaos__)
	if (path == NULL || path[0] == '\0')
	{
		return false;
	}
	/* DEVICE:path and ASSIGN:path are absolute AmigaDOS names.  A colon must
	 * belong to the first path component, before any slash. */
	const char *colon = strchr(path, ':');
	const char *slash = strchr(path, '/');
	const char *backslash = strchr(path, '\\');
	return colon != NULL &&
		(slash == NULL || colon < slash) &&
		(backslash == NULL || colon < backslash);
#else
	return path[0] == '/';
#endif
}
'''

if new_data not in text:
    if text.count(old_data) != 1:
        raise RuntimeError("GetDataFilePath anchor missing or ambiguous")
    text = text.replace(old_data, new_data, 1)

if new_abs not in text:
    if text.count(old_abs) != 1:
        raise RuntimeError("IsAbsolutePath anchor missing or ambiguous")
    text = text.replace(old_abs, new_abs, 1)

source.write_text(text, encoding="utf-8")
print(f"patched AmigaDOS absolute data paths: {source}")
