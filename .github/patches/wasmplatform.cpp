#ifdef __EMSCRIPTEN__
#include <framework/global.h>
#include "platform.h"
#include <emscripten.h>
#include <unistd.h>
#include <sys/stat.h>
#include "crashhandler.h"

void Platform::processArgs(std::vector<std::string>&) {}
std::string Platform::getCurrentDir() { return "/"; }
ticks_t Platform::getFileModificationTime(std::string file) {
    struct stat st;
    if(stat(file.c_str(), &st) == 0) return (ticks_t)st.st_mtime;
    return 0;
}
bool Platform::openDir(std::string, bool) { return false; }
bool Platform::openUrl(std::string url, bool) { emscripten_run_script(("window.open('" + url + "')").c_str()); return true; }
std::string Platform::getCPUName() { return "wasm"; }
double Platform::getTotalSystemMemory() { return 0; }
double Platform::getMemoryUsage() { return 0; }
std::string Platform::getOSName() { return "Web"; }
std::string Platform::traceback(const std::string& where, int, int) { return where; }
std::vector<std::string> Platform::getMacAddresses() { return {}; }
std::string Platform::getUserName() { return "player"; }
int Platform::getProcessId() { return 1; }
bool Platform::isProcessRunning(const std::string&) { return false; }
std::string Platform::getTempPath() { return "/tmp/"; }
std::vector<std::string> Platform::getDlls() { return {}; }
std::vector<std::string> Platform::getWindows() { return {}; }
std::vector<std::string> Platform::getProcesses() { return {}; }

void installCrashHandler() {}
void uninstallCrashHandler() {}

#endif
