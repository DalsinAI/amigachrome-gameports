/* SPDX-License-Identifier: MIT
 * Inspect genuine guest display modes for an independent FS-UAE lab.
 * No modes are invented and no display preferences are changed.
 */
#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <graphics/displayinfo.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <string.h>
struct GfxBase *GfxBase;
int main(void) {
    ULONG id = INVALID_ID;
    GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 39);
    if (!GfxBase) return 20;
    while ((id = NextDisplayInfo(id)) != (ULONG)INVALID_ID) {
        struct NameInfo name;
        struct DimensionInfo dims;
        memset(&name, 0, sizeof name);
        memset(&dims, 0, sizeof dims);
        GetDisplayInfoData(NULL, (UBYTE *)&name, sizeof name, DTAG_NAME, id);
        if (GetDisplayInfoData(NULL, (UBYTE *)&dims, sizeof dims, DTAG_DIMS, id))
            printf("%08lx %ldx%ld depth=%u %s\n", (unsigned long)id,
                (long)dims.Nominal.MaxX - dims.Nominal.MinX + 1,
                (long)dims.Nominal.MaxY - dims.Nominal.MinY + 1,
                (unsigned)dims.MaxDepth, name.Name);
    }
    CloseLibrary((struct Library *)GfxBase);
    return 0;
}
