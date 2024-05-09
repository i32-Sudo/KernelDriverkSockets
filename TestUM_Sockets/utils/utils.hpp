#include "../imports.h"

size_t write_callback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    size_t total_size = size * nmemb;
    data->append(reinterpret_cast<char*>(ptr), total_size);
    return total_size;
}

std::string extract_file_name(const std::string& url)
{
    int i = url.size();
    for (; i >= 0; i--)
    {
        if (url[i] == '/')
        {
            break;
        }
    }

    return url.substr(i + 1, url.size() - 1);
}

bool IsServiceRunning(const char* service_name, SC_HANDLE service_manager) {
    if (service_manager == NULL) {
        std::cerr << "Failed to open service manager: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE service = OpenService(service_manager, service_name, SERVICE_QUERY_STATUS);
    if (service == NULL) {
        std::cerr << "Failed to open service: " << GetLastError() << std::endl;
        CloseServiceHandle(service_manager);
        return false;
    }

    SERVICE_STATUS service_status;
    if (!QueryServiceStatus(service, &service_status)) {
        std::cerr << "Failed to query service status: " << GetLastError() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        return false;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);

    return service_status.dwCurrentState == SERVICE_RUNNING;
}

bool StopAndDeleteService(const char* service_name) {
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL) {
        std::cerr << "Failed to open service manager: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE service = OpenService(service_manager, service_name, SERVICE_STOP | DELETE);
    if (service == NULL) {
        // If the service doesn't exist, return true to indicate success
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
            CloseServiceHandle(service_manager);
            return true;
        }
        else {
            std::cerr << "Failed to open service: " << GetLastError() << std::endl;
            CloseServiceHandle(service_manager);
            return false;
        }
    }

    // Stop the service
    SERVICE_STATUS service_status;
    if (!ControlService(service, SERVICE_CONTROL_STOP, &service_status)) {
        DWORD error = GetLastError();
        if (error != ERROR_SERVICE_NOT_ACTIVE) {
            std::cerr << "Failed to stop service: " << error << std::endl;
            CloseServiceHandle(service);
            CloseServiceHandle(service_manager);
            return false;
        }
    }

    // Delete the service
    if (!DeleteService(service)) {
        std::cerr << "Failed to delete service: " << GetLastError() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        return false;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    return true;
}

namespace fs = std::filesystem;
bool fileExists(const std::string& filePath) {
    return fs::exists(filePath) && fs::is_regular_file(filePath);
}

std::string generateRandomString(int length) {
    // Characters that can be used in the random string
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.length() - 1);

    std::string randomString;
    for (int i = 0; i < length; ++i) {
        randomString += charset[dis(gen)];
    }

    return randomString;
}

bool IsProcessRunning(const std::string& processName) {
    // Get a handle to the snapshot of all processes
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Iterate through the processes
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return false;
    }

    do {
        // Compare process name
        if (strcmp(pe32.szExeFile, processName.c_str()) == 0) {
            CloseHandle(hProcessSnap);
            return true;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return false;
}