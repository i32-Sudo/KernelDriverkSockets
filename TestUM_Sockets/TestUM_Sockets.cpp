#include "imports.h"
#include "utils/utils.hpp"
#include <cstdlib>

#define CURL_STATICLIB
#include "externals/curl/curl/curl.h"
#pragma comment(lib, "externals/curl/curl/libcurl_a.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "advapi32.lib")


MemoryIO* memio = nullptr;
bool ShowMenu = true;
bool StyleLoaded = false;


INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT nCmdShow)
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, xorstr_("CONOUT$"), xorstr_("w"), stdout);
    SetConsoleTitleW(xorstr_(L"i64NtNvda.exe"));
	
    std::cout << xorstr_("initializing...\n\n");

    std::cout << xorstr_("Waiting for process ") << xorstr_("notepad.exe") << std::endl;
    while (!IsProcessRunning(xorstr_("notepad.exe"))) {
        Sleep(1000);
    }

    Sleep(1);
    PMem* mem = new PMem();
    const auto connection = mem->connectsystem();
    memio = new MemoryIO(*mem, connection);

    while (true) {
        Sleep(2000);
        std::cout << memio->getCachedPID() << std::endl;
        uint64_t BaseModule = mem->GetModuleBaseAddress(connection, memio->getCachedPID(), "notepad.exe");
        std::cout << BaseModule << std::endl;
    }

    return 0;
}