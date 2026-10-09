#!/usr/bin/env python3
"""Patch pinned SeriousSamClassic TFE for the AmigaChrome/OpenUp runtime.

The port is intentionally a first-class m68k/Amiga target:
- 32-bit big-endian m68k, 68040+FPU;
- portable C/C++ paths only (no x86/ARM assembler personalities);
- one monolithic executable (Game/Entities/Shaders are object libraries);
- SDL2/OpenGL/audio/input come from the OpenGPU/OpenUp SDK;
- upstream host tools remain host tools;
- optional codec DLLs, editors and dedicated-server tooling are outside first light.

GPL-2.0, matching SeriousSamClassic.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        if new in text:
            return text
        raise SystemExit(f"patch marker missing: {label}")
    return text.replace(old, new, 1)


def entity_classes(src: Path) -> list[str]:
    names = {"CEntity", "CLiveEntity", "CRationalEntity"}
    for base in (src / "Engine" / "Classes", src / "Entities"):
        for path in sorted(base.glob("*.es")):
            data = path.read_text(encoding="latin-1")
            for m in re.finditer(r"\bclass\s+(C[A-Za-z_][A-Za-z0-9_]*)\s*:", data):
                names.add(m.group(1))
    return sorted(names)


def shader_names(src: Path) -> list[str]:
    names: set[str] = set()
    for path in sorted((src / "Shaders").glob("*.cpp")):
        data = path.read_text(encoding="latin-1")
        names.update(re.findall(r"SHADER_MAIN\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)", data))
    return sorted(names)


def make_local_network(src: Path) -> None:
    # First light deliberately keeps Serious Engine's local packet transport
    # and removes remote BSD sockets/GameAgent.  Multiplayer comes back as an
    # explicit OpenSocket integration after the playable gate.
    hdr = src / "Engine" / "Network" / "CommunicationInterface.h"
    text = hdr.read_text(encoding="latin-1")
    text = replace_once(
        text,
        """#ifdef PLATFORM_UNIX
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket close
typedef int SOCKET;
typedef struct hostent HOSTENT;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr    SOCKADDR;
#define WSAGetLastError() (INDEX) errno
#endif
""",
        """#if defined(PLATFORM_AMIGA)
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
typedef int SOCKET;
#elif defined(PLATFORM_UNIX)
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket close
typedef int SOCKET;
typedef struct hostent HOSTENT;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr    SOCKADDR;
#define WSAGetLastError() (INDEX) errno
#endif
""",
        "Amiga local-only network header",
    )
    hdr.write_text(text, encoding="latin-1")

    out = src / "Engine" / "Network" / "Amiga"
    out.mkdir(parents=True, exist_ok=True)
    (out / "AmigaCommunicationInterface.cpp").write_text(r'''/* Serious Engine local-only transport for AmigaChrome first light.
 * Remote networking is intentionally absent until the OpenSocket pass. */
#include <Engine/StdH.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Synchronization.h>
#include <Engine/Base/Translation.h>
#include <Engine/Network/ClientInterface.h>
#include <Engine/Network/CommunicationInterface.h>
#include <Engine/Network/Network.h>

#define SERVER_LOCAL_CLIENT 0

CPacketBufferStats _pbsSend;
CPacketBufferStats _pbsRecv;

static CClientInterface cm_aciClients[SERVER_CLIENTS];
static CClientInterface cm_ciLocalClient;
static CClientInterface cm_ciBroadcast;
static CTCriticalSection cm_csComm;
static BOOL cm_bNetworkInitialized = FALSE;
static CTString cm_strName = "AmigaChrome";
static CTString cm_strAddress = "local";

CCommunicationInterface _cmiComm;

CTString AddressToString(ULONG ulHost)
{
  (void)ulHost;
  return CTString("local");
}

ULONG StringToAddress(const CTString &strAddress)
{
  (void)strAddress;
  return 0;
}

CCommunicationInterface::CCommunicationInterface(void)
{
  cm_csComm.cs_iIndex = -1;
  cci_bSocketOpen = FALSE;
  cci_bBound = FALSE;
  cci_bInitialized = FALSE;
  cci_bWinSockOpen = FALSE;
  cci_bServerInitialized = FALSE;
  cci_bClientInitialized = FALSE;
  cci_hSocket = INVALID_SOCKET;
}

void CCommunicationInterface::Init(void)
{
  CTSingleLock lock(&cm_csComm, TRUE);
  _pbsSend.Clear();
  _pbsRecv.Clear();
  cci_pbMasterInput.Clear();
  cci_pbMasterOutput.Clear();
  cci_bInitialized = TRUE;
  cci_bSocketOpen = FALSE;
  cci_bBound = FALSE;
  cci_bWinSockOpen = FALSE;
  cci_bServerInitialized = FALSE;
  cci_bClientInitialized = FALSE;
  cm_bNetworkInitialized = FALSE;
}

void CCommunicationInterface::Close(void)
{
  CTSingleLock lock(&cm_csComm, TRUE);
  cm_ciLocalClient.Clear();
  cm_ciBroadcast.Clear();
  for (INDEX i=0; i<SERVER_CLIENTS; ++i) cm_aciClients[i].Clear();
  cci_pbMasterInput.Clear();
  cci_pbMasterOutput.Clear();
  cci_bInitialized = FALSE;
  cci_bServerInitialized = FALSE;
  cci_bClientInitialized = FALSE;
}

void CCommunicationInterface::InitWinsock(void) { cci_bWinSockOpen = FALSE; }
void CCommunicationInterface::EndWinsock(void)  { cci_bWinSockOpen = FALSE; }

void CCommunicationInterface::PrepareForUse(BOOL bUseNetwork, BOOL bClient)
{
  (void)bClient;
  cm_bNetworkInitialized = FALSE;
  if (bUseNetwork)
    CPrintF(TRANSV("OpenUp first-light build: remote networking disabled; local play only.\n"));
}

void CCommunicationInterface::Unprepare(void)
{
  cm_bNetworkInitialized = FALSE;
  cci_bWinSockOpen = FALSE;
  cci_bBound = FALSE;
}

BOOL CCommunicationInterface::IsNetworkEnabled(void) { return FALSE; }

void CCommunicationInterface::GetHostName(CTString &strName, CTString &strAddress)
{
  strName = cm_strName;
  strAddress = cm_strAddress;
}

void CCommunicationInterface::CreateSocket_t(void)
{ ThrowF_t(TRANS("Remote networking is not enabled in this OpenUp first-light build.")); }
void CCommunicationInterface::Bind_t(ULONG, ULONG)
{ ThrowF_t(TRANS("Remote networking is not enabled in this OpenUp first-light build.")); }
void CCommunicationInterface::SetNonBlocking_t(void) {}
CTString CCommunicationInterface::GetSocketError(INDEX)
{ return CTString("OpenSocket not enabled in first-light build"); }
void CCommunicationInterface::OpenSocket_t(ULONG, ULONG)
{ ThrowF_t(TRANS("Remote networking is not enabled in this OpenUp first-light build.")); }
void CCommunicationInterface::GetLocalAddress_t(ULONG &host, ULONG &port) { host=0; port=0; }
void CCommunicationInterface::GetRemoteAddress_t(ULONG &host, ULONG &port) { host=0; port=0; }
void CCommunicationInterface::Broadcast_Send(const void *, SLONG, CAddress &) {}
BOOL CCommunicationInterface::Broadcast_Receive(void *, SLONG &, CAddress &) { return FALSE; }
void CCommunicationInterface::Broadcast_Update_t(void) {}
void CCommunicationInterface::UpdateMasterBuffers(void) {}

void CCommunicationInterface::Server_Init_t(void)
{
  CTSingleLock lock(&cm_csComm, TRUE);
  for (INDEX i=0; i<SERVER_CLIENTS; ++i) {
    cm_aciClients[i].Clear();
    cm_aciClients[i].ci_pbOutputBuffer.pb_ppbsStats = &_pbsSend;
    cm_aciClients[i].ci_pbInputBuffer.pb_ppbsStats = &_pbsRecv;
  }
  cm_aciClients[SERVER_LOCAL_CLIENT].ci_bClientLocal = TRUE;
  cm_aciClients[SERVER_LOCAL_CLIENT].ci_bUsed = TRUE;
  cm_ciLocalClient.Clear();
  cm_ciLocalClient.ci_bUsed = TRUE;
  cm_ciLocalClient.ci_bClientLocal = TRUE;
  cm_ciLocalClient.ci_pbOutputBuffer.pb_ppbsStats = &_pbsSend;
  cm_ciLocalClient.ci_pbInputBuffer.pb_ppbsStats = &_pbsRecv;
  cci_bServerInitialized = TRUE;
}

void CCommunicationInterface::Server_Close(void)
{
  CTSingleLock lock(&cm_csComm, TRUE);
  for (INDEX i=0; i<SERVER_CLIENTS; ++i) cm_aciClients[i].Clear();
  cci_bServerInitialized = FALSE;
}

void CCommunicationInterface::Server_ClearClient(INDEX iClient)
{
  if (iClient>=0 && iClient<SERVER_CLIENTS) cm_aciClients[iClient].Clear();
}
BOOL CCommunicationInterface::Server_IsClientLocal(INDEX iClient) { return iClient==SERVER_LOCAL_CLIENT; }
BOOL CCommunicationInterface::Server_IsClientUsed(INDEX iClient)
{ return (iClient>=0 && iClient<SERVER_CLIENTS) ? cm_aciClients[iClient].ci_bUsed : FALSE; }
CTString CCommunicationInterface::Server_GetClientName(INDEX iClient)
{ return Server_IsClientLocal(iClient) ? CTString("Local machine") : CTString(""); }

void CCommunicationInterface::Server_Send_Reliable(INDEX i, const void *p, SLONG n)
{ cm_aciClients[i].Send(p,n,TRUE); }
BOOL CCommunicationInterface::Server_Receive_Reliable(INDEX i, void *p, SLONG &n)
{ return cm_aciClients[i].Receive(p,n,TRUE); }
void CCommunicationInterface::Server_Send_Unreliable(INDEX i, const void *p, SLONG n)
{ cm_aciClients[i].Send(p,n,FALSE); }
BOOL CCommunicationInterface::Server_Receive_Unreliable(INDEX i, void *p, SLONG &n)
{ return cm_aciClients[i].Receive(p,n,FALSE); }

BOOL CCommunicationInterface::Server_Update(void)
{
  if (cm_ciLocalClient.ci_bUsed && cm_ciLocalClient.ci_pciOther)
    cm_ciLocalClient.ExchangeBuffers();
  cm_aciClients[SERVER_LOCAL_CLIENT].UpdateOutputBuffers();
  cm_aciClients[SERVER_LOCAL_CLIENT].UpdateInputBuffers();
  cm_ciLocalClient.UpdateOutputBuffers();
  cm_ciLocalClient.UpdateInputBuffers();
  return TRUE;
}

void CCommunicationInterface::Client_Init_t(char *)
{
  if (!cci_bServerInitialized)
    ThrowF_t(TRANS("Remote networking is not enabled in this OpenUp first-light build."));
  Client_Init_t((ULONG)0);
}

void CCommunicationInterface::Client_Init_t(ULONG)
{
  CTSingleLock lock(&cm_csComm, TRUE);
  if (!cci_bServerInitialized)
    ThrowF_t(TRANS("Remote networking is not enabled in this OpenUp first-light build."));
  cm_ciLocalClient.Clear();
  cm_ciLocalClient.ci_pbOutputBuffer.pb_ppbsStats = &_pbsSend;
  cm_ciLocalClient.ci_pbInputBuffer.pb_ppbsStats = &_pbsRecv;
  cm_ciLocalClient.ci_bClientLocal = TRUE;
  Client_OpenLocal();
  cci_bClientInitialized = TRUE;
}

void CCommunicationInterface::Client_Close(void)
{
  cm_ciLocalClient.Clear();
  cm_ciLocalClient.ci_bClientLocal = FALSE;
  cci_bClientInitialized = FALSE;
}

void CCommunicationInterface::Client_OpenLocal(void)
{
  CClientInterface &client = cm_ciLocalClient;
  CClientInterface &server = cm_aciClients[SERVER_LOCAL_CLIENT];
  client.ci_bUsed = TRUE;
  client.SetLocal(&server);
  server.ci_bUsed = TRUE;
  server.SetLocal(&client);
}

void CCommunicationInterface::Client_OpenNet_t(ULONG)
{ ThrowF_t(TRANS("Remote networking is not enabled in this OpenUp first-light build.")); }

void CCommunicationInterface::Client_Clear(void) { cm_ciLocalClient.Clear(); }
BOOL CCommunicationInterface::Client_IsConnected(void) { return cm_ciLocalClient.ci_bUsed; }
void CCommunicationInterface::Client_Send_Reliable(const void *p, SLONG n)
{ cm_ciLocalClient.Send(p,n,TRUE); }
BOOL CCommunicationInterface::Client_Receive_Reliable(void *p, SLONG &n)
{ return cm_ciLocalClient.Receive(p,n,TRUE); }
BOOL CCommunicationInterface::Client_Receive_Reliable(CTStream &s)
{ return cm_ciLocalClient.Receive(s,TRUE); }
void CCommunicationInterface::Client_PeekSize_Reliable(SLONG &expected, SLONG &received)
{ expected=cm_ciLocalClient.GetExpectedReliableSize(); received=cm_ciLocalClient.GetCurrentReliableSize(); }
void CCommunicationInterface::Client_Send_Unreliable(const void *p, SLONG n)
{ cm_ciLocalClient.Send(p,n,FALSE); }
BOOL CCommunicationInterface::Client_Receive_Unreliable(void *p, SLONG &n)
{ return cm_ciLocalClient.Receive(p,n,FALSE); }

BOOL CCommunicationInterface::Client_Update(void)
{
  if (cm_ciLocalClient.ci_bUsed && cm_ciLocalClient.ci_pciOther)
    cm_ciLocalClient.ExchangeBuffers();
  cm_ciLocalClient.UpdateOutputBuffers();
  cm_ciLocalClient.UpdateInputBuffers();
  cm_aciClients[SERVER_LOCAL_CLIENT].UpdateOutputBuffers();
  cm_aciClients[SERVER_LOCAL_CLIENT].UpdateInputBuffers();
  return TRUE;
}
''', encoding="latin-1")

    ga = src / "Engine" / "GameAgent" / "Amiga"
    ga.mkdir(parents=True, exist_ok=True)
    (ga / "AmigaGameAgent.cpp").write_text(r'''/* No master-server/network discovery in first-light local-play build. */
#include <Engine/StdH.h>
#include <Engine/GameAgent/GameAgent.h>

CTString ga_strServer = "";
CTString ga_strMSLegacy = "";
BOOL ga_bMSLegacy = FALSE;

CServerRequest::CServerRequest(void) { Clear(); }
CServerRequest::~CServerRequest(void) {}
void CServerRequest::Clear(void)
{
  sr_ulAddress=0; sr_iPort=0; sr_tmRequestTime=0;
}

void GameAgent_ServerInit(void) {}
void GameAgent_ServerEnd(void) {}
void GameAgent_ServerUpdate(void) {}
void GameAgent_ServerStateChanged(void) {}
void GameAgent_EnumTrigger(BOOL) {}
void GameAgent_EnumUpdate(void) {}
void GameAgent_EnumCancel(void) {}
void *_MS_Thread(void *) { return 0; }
void *_LocalNet_Thread(void *) { return 0; }
''', encoding="latin-1")


def make_loader(src: Path) -> None:
    entities = entity_classes(src)
    shaders = shader_names(src)
    out = src / "Engine" / "Base" / "Amiga"
    out.mkdir(parents=True, exist_ok=True)
    lines = [
        "/* AmigaChrome/OpenUp static symbol loader for Serious Engine. */",
        "#include <string.h>",
        "#include <Engine/Engine.h>",
        "#include <Engine/Base/DynamicLoader.h>",
        "#include <Engine/Entities/EntityProperties.h>",
        "#include <Engine/Graphics/Shader.h>",
        "#include <GameMP/Game.h>",
        "",
        'extern "C" CGame *GAME_Create(void);',
    ]
    for name in entities:
        lines.append(f'extern "C" CDLLEntityClass {name}_DLLClass;')
    for name in shaders:
        lines.append(f'extern "C" void Shader_{name}(void);')
        lines.append(f'extern "C" void Shader_Desc_{name}(ShaderDesc &);')
    lines += [
        "",
        "class CAmigaStaticLoader : public CDynamicLoader {",
        "public:",
        "  CAmigaStaticLoader() : err(NULL) {}",
        "  virtual ~CAmigaStaticLoader(void) {}",
        "  virtual void *FindSymbol(const char *sym);",
        "  virtual const char *GetError(void) { return err; }",
        "private:",
        "  const char *err;",
        "};",
        "",
        "void *CAmigaStaticLoader::FindSymbol(const char *sym)",
        "{",
        "  err = NULL;",
        '  if (!sym) { err = "OpenUp static loader: null symbol"; return NULL; }',
        '  if (strcmp(sym, "GAME_Create") == 0) return (void *)&GAME_Create;',
    ]
    for name in entities:
        lines.append(f'  if (strcmp(sym, "{name}_DLLClass") == 0) return (void *)&{name}_DLLClass;')
    for name in shaders:
        lines.append(f'  if (strcmp(sym, "Shader_{name}") == 0) return (void *)&Shader_{name};')
        lines.append(f'  if (strcmp(sym, "Shader_Desc_{name}") == 0) return (void *)&Shader_Desc_{name};')
    lines += [
        '  err = "OpenUp static loader: symbol not built into SeriousSam";',
        "  return NULL;",
        "}",
        "",
        "CDynamicLoader *CDynamicLoader::GetInstance(const char *)",
        "{",
        "  return new CAmigaStaticLoader;",
        "}",
        "",
        "CTFileName CDynamicLoader::ConvertLibNameToPlatform(const char *libname)",
        "{",
        '  return CTFileName(CTString(libname ? libname : ""));',
        "}",
        "",
    ]
    (out / "AmigaDynamicLoader.cpp").write_text("\n".join(lines), encoding="utf-8")


def patch_types(src: Path) -> None:
    path = src / "Engine" / "Base" / "Types.h"
    text = path.read_text(encoding="latin-1")
    text = replace_once(
        text,
        "#if __POWERPC__ || (defined __ppc64__) || (defined __alpha__) || (defined __sparc__) /* rcg03232004 */",
        "#if defined(__m68k__) || defined(__mc68000__) || __POWERPC__ || (defined __ppc64__) || (defined __alpha__) || (defined __sparc__) /* AmigaChrome: m68k is big-endian */",
        "m68k big endian",
    )
    text = replace_once(
        text,
        "#elif defined(__i386) || defined(_M_IX86) || defined(__arm__) || defined(_M_ARM) || defined(__POWERPC__) \\\n      || defined(_M_PPC)",
        "#elif defined(__m68k__) || defined(__mc68000__) || defined(__i386) || defined(_M_IX86) || defined(__arm__) || defined(_M_ARM) || defined(__POWERPC__) \\\n      || defined(_M_PPC)",
        "m68k 32 bit",
    )
    text = text.replace(
        "#if defined(__aarch64__) || defined(__arm__) || PLATFORM_RISCV64",
        "#if defined(__m68k__) || defined(__mc68000__) || defined(__aarch64__) || defined(__arm__) || PLATFORM_RISCV64",
        1,
    )

    # libnix already provides char *strupr(char *).  Upstream's Unix helper
    # becomes a second, void-return declaration after the _strupr macro.
    text = replace_once(
        text,
        """    inline void _strupr(char *str)
    {
        if (str != NULL)
        {
            for (char *ptr = str; *ptr; ptr++)
               *ptr = toupper(*ptr);
        }
    }
""",
        """    #if !defined(PLATFORM_AMIGA)
    inline void _strupr(char *str)
    {
        if (str != NULL)
        {
            for (char *ptr = str; *ptr; ptr++)
               *ptr = toupper(*ptr);
        }
    }
    #endif
""",
        "libnix strupr",
    )

    # On the Amiga Unix-compat personality BOOL and SLONG are both int32_t.
    # The SLONG endian helper therefore already covers BOOL; defining both is
    # an illegal duplicate C++ overload.
    text = replace_once(
        text,
        """    static inline void BYTESWAP(BOOL &val)
    {
        // !!! FIXME: reinterpret_cast ?
        ULONG uval = *((ULONG *) &val);
        BYTESWAP(uval);
        val = *((BOOL *) &uval);
    }
""",
        """    #if !defined(PLATFORM_AMIGA)
    static inline void BYTESWAP(BOOL &val)
    {
        // !!! FIXME: reinterpret_cast ?
        ULONG uval = *((ULONG *) &val);
        BYTESWAP(uval);
        val = *((BOOL *) &uval);
    }
    #endif
""",
        "Amiga BOOL byteswap alias",
    )

    path.write_text(text, encoding="latin-1")


def patch_game(src: Path) -> None:
    path = src / "GameMP" / "Game.cpp"
    text = path.read_text(encoding="latin-1")
    text = replace_once(
        text,
        """    memcpy(&gm_ahseHighScores[i].hse_gdDifficulty, pub, sizeof(INDEX));
    BYTESWAP(gm_ahseHighScores[i].hse_gdDifficulty);
    pub += sizeof(INDEX);
""",
        """    INDEX iDifficulty;
    memcpy(&iDifficulty, pub, sizeof(INDEX));
    BYTESWAP(iDifficulty);
    gm_ahseHighScores[i].hse_gdDifficulty =
      (CSessionProperties::GameDifficulty)iDifficulty;
    pub += sizeof(INDEX);
""",
        "big-endian high-score enum",
    )
    path.write_text(text, encoding="latin-1")


def patch_synchronization(src: Path) -> None:
    path = src / "Engine" / "Base" / "Synchronization.h"
    text = path.read_text(encoding="latin-1")
    text = text.replace(
        "#ifdef PLATFORM_UNIX\n#include <pthread.h>\n#endif",
        "#if defined(PLATFORM_UNIX) && !defined(SINGLE_THREADED)\n#include <pthread.h>\n#endif",
        1,
    )
    text = text.replace(
        "#ifdef PLATFORM_UNIX\ntemplate <typename T>",
        "#if defined(PLATFORM_UNIX) && !defined(SINGLE_THREADED)\ntemplate <typename T>",
        1,
    )
    path.write_text(text, encoding="latin-1")


def patch_base(src: Path) -> None:
    path = src / "Engine" / "Base" / "Base.h"
    text = path.read_text(encoding="latin-1")
    text = replace_once(
        text,
        "#elif (defined __linux__) ",
        """#elif defined(PLATFORM_AMIGA) || defined(__amigaos__) || defined(__AMIGA__)
    #ifndef PLATFORM_AMIGA
      #define PLATFORM_AMIGA 1
    #endif
#elif (defined __linux__) """,
        "Amiga platform recognition",
    )
    path.write_text(text, encoding="latin-1")


def patch_cmake(src: Path) -> None:
    path = src / "CMakeLists.txt"
    text = path.read_text(encoding="utf-8")

    text = replace_once(
        text,
        "project(SeriousEngine)\n",
        """project(SeriousEngine)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR MATCHES "m68k")
    set(AMIGAOS3 TRUE)
    set(CMAKE_OS_NAME "AmigaOS 3.x" CACHE STRING "Operating system name" FORCE)
    message(STATUS "AmigaChrome/OpenUp m68k target")
endif()
""",
        "Amiga target detection",
    )

    old_sdl = """if(NOT USE_SYSTEM_SDL2)
    include_directories(${CMAKE_SOURCE_DIR}/External/SDL2)
else()
    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
\tinclude_directories(${SDL2_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Error USE_SYSTEM_SDL2 is set but neccessary developer files are missing")
    endif()
endif()
"""
    new_sdl = """if(AMIGAOS3)
    if(NOT DEFINED ENV{OPENUP_SDK})
        message(FATAL_ERROR "OPENUP_SDK must name the OpenGPU/OpenUp SDK root")
    endif()
    execute_process(
        COMMAND "$ENV{OPENUP_SDK}/bin/sdl2-config" --cflags
        OUTPUT_VARIABLE _AMIGA_SDL_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _AMIGA_SDL_CFLAGS_RC)
    execute_process(
        COMMAND "$ENV{OPENUP_SDK}/bin/sdl2-config" --libs --gl
        OUTPUT_VARIABLE _AMIGA_SDL_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _AMIGA_SDL_LIBS_RC)
    if(NOT _AMIGA_SDL_CFLAGS_RC EQUAL 0 OR NOT _AMIGA_SDL_LIBS_RC EQUAL 0)
        message(FATAL_ERROR "OpenGPU sdl2-config failed")
    endif()
    separate_arguments(_AMIGA_SDL_CFLAGS UNIX_COMMAND "${_AMIGA_SDL_CFLAGS}")
    separate_arguments(SDL2_LIBRARY UNIX_COMMAND "${_AMIGA_SDL_LIBS}")
    add_compile_options(${_AMIGA_SDL_CFLAGS})
    set(SDL2_FOUND TRUE)
elseif(NOT USE_SYSTEM_SDL2)
    include_directories(${CMAKE_SOURCE_DIR}/External/SDL2)
else()
    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
\tinclude_directories(${SDL2_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Error USE_SYSTEM_SDL2 is set but neccessary developer files are missing")
    endif()
endif()
"""
    text = replace_once(text, old_sdl, new_sdl, "OpenGPU SDL2 discovery")

    text = replace_once(
        text,
        """            if(LOCAL_INSTALL)
\t\t\t    add_compile_options(-march=native)
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i586|i686|x86|amd64|AMD64|x86_64")
""",
        """            if(LOCAL_INSTALL AND NOT AMIGAOS3)
\t\t\t    add_compile_options(-march=native)
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i586|i686|x86|amd64|AMD64|x86_64")
""",
        "no host march on m68k cross target",
    )

    # Position-independent shared-library code is host baggage here: the
    # Amiga target is one monolithic LoadSeg executable.
    text = text.replace(
        "\tadd_compile_options(-fPIC)",
        """    if(NOT AMIGAOS3)
\t  add_compile_options(-fPIC)
    endif()""",
        1,
    )

    text = replace_once(
        text,
        """\tif(MACOSX)
\t\tadd_definitions(-DPLATFORM_UNIX=1)
""",
        """\tif(AMIGAOS3)
        add_definitions(-DPLATFORM_UNIX=1)
        add_definitions(-DPLATFORM_AMIGA=1)
        add_definitions(-DPRAGMA_ONCE=1)
        add_definitions(-DSTATICALLY_LINKED=1)
        add_compile_options(-fsigned-char)
\telseif(MACOSX)
\t\tadd_definitions(-DPLATFORM_UNIX=1)
""",
        "Amiga platform definitions",
    )

    text = text.replace(
        "Engine/Base/Unix/UnixDynamicLoader.cpp",
        "Engine/Base/Amiga/AmigaDynamicLoader.cpp",
    )
    text = text.replace(
        "Engine/Network/CommunicationInterface.cpp",
        "Engine/Network/Amiga/AmigaCommunicationInterface.cpp",
    )
    text = text.replace(
        "Engine/GameAgent/GameAgent.cpp",
        "Engine/GameAgent/Amiga/AmigaGameAgent.cpp",
    )

    text = text.replace("add_library(${ENTITIESMPLIB} SHARED", "add_library(${ENTITIESMPLIB} OBJECT")
    text = text.replace("add_library(${GAMEMPLIB} SHARED", "add_library(${GAMEMPLIB} OBJECT")
    text = text.replace("add_library(${SHADERSLIB} SHARED", "add_library(${SHADERSLIB} OBJECT")
    text = text.replace("add_library(${ENGINELIB} SHARED", "add_library(${ENGINELIB} STATIC")

    text = replace_once(
        text,
        """add_executable(SeriousSam${MP}
    #${ENGINE_SRCS}
""",
        """add_executable(SeriousSam${MP}
    $<TARGET_OBJECTS:${GAMEMPLIB}>
    $<TARGET_OBJECTS:${ENTITIESMPLIB}>
    $<TARGET_OBJECTS:${SHADERSLIB}>
""",
        "monolithic SeriousSam objects",
    )

    text = replace_once(
        text,
        "target_link_libraries(SeriousSam${MP} ${ENGINELIB})\n",
        """target_link_libraries(SeriousSam${MP} ${ENGINELIB})
if(AMIGAOS3)
    target_link_libraries(SeriousSam${MP} ${SDL2_LIBRARY} m socket)
endif()
""",
        "OpenUp runtime links",
    )

    text = replace_once(
        text,
        'set_target_properties(${ENGINELIB} PROPERTIES ENABLE_EXPORTS ON LINK_FLAGS "-Wl,${RPATH_SETTINGS}")\n',
        """if(NOT AMIGAOS3)
    set_target_properties(${ENGINELIB} PROPERTIES ENABLE_EXPORTS ON LINK_FLAGS "-Wl,${RPATH_SETTINGS}")
endif()
""",
        "engine export guard",
    )
    text = replace_once(
        text,
        "set_target_properties(SeriousSam${MP} PROPERTIES ENABLE_EXPORTS ON)\n",
        """if(NOT AMIGAOS3)
    set_target_properties(SeriousSam${MP} PROPERTIES ENABLE_EXPORTS ON)
endif()
""",
        "executable export guard",
    )

    text = replace_once(
        text,
        "# Set output name for not local installation\nif(LOCAL_INSTALL)\n",
        """# Set output name for not local installation
if(AMIGAOS3)
 set_target_properties(SeriousSam${MP} PROPERTIES OUTPUT_NAME "SeriousSam")
elseif(LOCAL_INSTALL)
""",
        "disabled-tool output names",
    )

    path.write_text(text, encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch_amigaos3.py /path/to/SeriousSamClassic")
    root = Path(sys.argv[1]).resolve()
    src = root / "SamTFE" / "Sources"
    if not src.is_dir():
        raise SystemExit(f"not a SeriousSamClassic checkout: {root}")
    patch_types(src)
    patch_game(src)
    patch_synchronization(src)
    patch_base(src)
    make_local_network(src)
    make_loader(src)
    patch_cmake(src)
    print("Serious Sam TFE patched for AmigaChrome/OpenUp")
    print(f"entity symbols: {len(entity_classes(src))}")
    print(f"shader pairs: {len(shader_names(src))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
