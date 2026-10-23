// Copyright (c) 2026 Dalsin Limited. MIT licensed.
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Core/Exception.hpp>
#include <SDL3/SDL.h>
#include <proto/dos.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
namespace OpenLoco::Platform
{
    static BPTR instanceFile;
    static void releaseInstance() { if(instanceFile) { Close(instanceFile);instanceFile=0; } }
    void initialise() {}
    uint32_t getTime() { return static_cast<uint32_t>(SDL_GetTicks()); }
    std::string getEnvironmentVariable(const std::string& name) {
        const char* v=std::getenv(name.c_str());
        if(v) return v;
        char buffer[4096];
        return GetVar(name.c_str(),buffer,sizeof buffer,0)>0?std::string(buffer):std::string();
    }
    static std::string programDirectory() {
        char path[4096];
        if(!NameFromLock(GetProgramDir(),path,sizeof path))
            throw Exception::RuntimeError("Cannot determine Amiga program directory");
        std::string result(path);
        if(!result.empty() && result.back()!=':' && result.back()!='/') result+='/';
        return result;
    }
    fs::path getCurrentExecutablePath() {
        char name[1024];
        if(!GetProgramName(name,sizeof name))
            throw Exception::RuntimeError("Cannot determine Amiga program name");
        return fs::path(programDirectory()+FilePart(name));
    }
    fs::path getDataDirectory() {
        auto path=getEnvironmentVariable("OPENLOCO_DATA_DIR");
        return fs::path(path.empty()?programDirectory()+"data":path);
    }
    fs::path getUserDirectory() {
        auto path=getEnvironmentVariable("OPENLOCO_USER_DIR");
        return fs::path(path.empty()?programDirectory()+"User":path);
    }
    std::vector<fs::path> getDrives() { return {}; }
    std::vector<fs::path> getLocoInstallSearchPaths() {
        const auto path=getEnvironmentVariable("LOCOMOTION_DATA_DIR");
        return path.empty()?std::vector<fs::path>{}:std::vector<fs::path>{fs::path(path)};
    }
    fs::path promptDirectory(const std::string&, void*) {
        std::string input;std::cout<<"Locomotion data folder: ";std::getline(std::cin,input);
        return fs::path(input);
    }
    bool isRunningInWine() { return false; }
    bool isStdOutRedirected() { return !IsInteractive(Output()); }
    bool hasTerminalVT100Support() { return false; }
    bool enableVT100TerminalMode() { return false; }
    std::vector<std::string> getCmdLineVector(int argc,const char** argv) {
        return std::vector<std::string>(argv,argv+argc);
    }
    bool lockSingleInstance() {
        if(instanceFile) return true;
        const auto user=getUserDirectory();
        fs::create_directories(user);
        const auto path=(user/"OpenLoco.lock").string();
        // MODE_NEWFILE holds an exclusive DOS file lock until Close/process exit.
        instanceFile=Open(path.c_str(),MODE_NEWFILE);
        if(!instanceFile) return false;
        std::atexit(releaseInstance);return true;
    }
}
