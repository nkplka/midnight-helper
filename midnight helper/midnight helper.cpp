#include <iostream>
#include <fstream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <unordered_set>

std::string readConfigValue(const std::string& configFile, const std::string& key) {
    std::ifstream file(configFile);
    std::string line;

    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string configKey = line.substr(0, pos);
            if (configKey == key) {
                std::string configValue = line.substr(pos + 1);
                return configValue;
            }
        }
    }

    return "";
}

void writeConfigValue(const std::string& configFile, const std::string& key, const std::string& value) {
    std::ifstream fileIn(configFile);
    std::ofstream fileOut("config.txt");
    std::string line;

    while (std::getline(fileIn, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string configKey = line.substr(0, pos);
            if (configKey == key) {
                line = key + "=" + value;
            }
        }
        fileOut << line << std::endl;
    }

    fileIn.close();
    fileOut.close();

    std::remove(configFile.c_str());
    std::rename("temp.txt", configFile.c_str());
}

bool isFirstRun(const std::string& configFile) {
    std::ifstream file(configFile);
    return !file.good();
}

void createConfigFile(const std::string& configFile) {
    std::ofstream file(configFile);
    if (file.is_open()) {
        file << "[midnight loader name]" << std::endl;
        file << "exe=rename.exe" << std::endl;
        file << "[commands]" << std::endl;
        file << "vgc=sc stop vgc" << std::endl;
        file << "vgk=sc stop vgk" << std::endl;
        file << "faceit=sc stop faceit" << std::endl;
        file << "[other]" << std::endl;
        file << "hideconsole=false" << std::endl;
        file.close();
    }
}

bool isElevated() {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }

        FreeSid(adminGroup);
    }

    return static_cast<bool>(isAdmin);
}

void executeCommand(const std::string& command, bool showConsole) {
    if (showConsole) {
        
        ShowWindow(GetConsoleWindow(), SW_SHOW);
    }
    else {
        
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    system(command.c_str());

    if (!showConsole) {
        
        HWND consoleWindow = GetConsoleWindow();
        PostMessage(consoleWindow, WM_CLOSE, 0, 0);
    }
}

int monitorApplication(const std::string& applicationName, bool isFirstRun) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32W);

        std::unordered_set<DWORD> processIds;

        while (Process32NextW(snapshot, &processEntry)) {
            std::wstring processNameWide(processEntry.szExeFile);
            std::string processName(processNameWide.begin(), processNameWide.end());

            if (processName == applicationName) {
                processIds.insert(processEntry.th32ProcessID);
            }
        }

        while (true) {
            snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot != INVALID_HANDLE_VALUE) {
                Process32FirstW(snapshot, &processEntry);

                while (Process32NextW(snapshot, &processEntry)) {
                    std::wstring processNameWide(processEntry.szExeFile);
                    std::string processName(processNameWide.begin(), processNameWide.end());

                    if (processName == applicationName) {
                        
                        if (processIds.find(processEntry.th32ProcessID) == processIds.end()) {
                            
                            processIds.insert(processEntry.th32ProcessID);
                            std::cout << "Detected launch of application: " << applicationName << std::endl;
                            if (!isElevated()) {
                                std::cout << "Application launched without admin privileges" << std::endl;
                            }
                            if (isElevated()) {
                                std::string command;
                                std::ifstream configFile("config.txt");
                                std::string line;

                                while (std::getline(configFile, line)) {
                                    size_t pos = line.find('=');
                                    if (pos != std::string::npos) {
                                        std::string configKey = line.substr(0, pos);
                                        if (configKey != "exe") {
                                            std::string configValue = line.substr(pos + 1);
                                            command = configValue;
                                            executeCommand(command, true);
                                            
                                        }
                                    }
                                    
                                }

                                configFile.close();
                            }
                        }
                    }
                    else {
                        
                        processIds.erase(processEntry.th32ProcessID);
                    }
                }

                CloseHandle(snapshot);
            }

            Sleep(1000);  
        }
    }
}

int main() {
    std::string configFile = "config.txt";
    std::string appNameKey = "exe";
    std::string hideConsoleKey = "hideconsole";
    std::string appName;
    std::string hideConsole;

    if (isFirstRun(configFile)) {
        createConfigFile(configFile);
        appName = readConfigValue(configFile, appNameKey);
        hideConsole = readConfigValue(configFile, hideConsoleKey);
        std::cout << "Config file created." << std::endl;
    }
    else {
        appName = readConfigValue(configFile, appNameKey);
        hideConsole = readConfigValue(configFile, hideConsoleKey);
        std::cout << "Config file already exists." << std::endl;
    }

    if (appName.empty()) {
        std::cout << "Error: App name not found in config file." << std::endl;
        return 0;
    }

    std::cout << "App Name: " << appName << std::endl;

    if (hideConsole == "true") {
        ShowWindow(GetConsoleWindow(), SW_HIDE);  
    }
    else {
        ShowWindow(GetConsoleWindow(), SW_SHOW);  
    }

    monitorApplication(appName, isFirstRun);

    return 0;
}
