#include <windows.h>
#include <dbt.h>
#include <string>
#include <shlobj.h>
#include <shlwapi.h>
#include <filesystem>
#include <commdlg.h>
#include <thread>
#include <mutex>
#include <shellapi.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <winioctl.h>
#include <wininet.h>
#include <wincrypt.h>
#include <dpapi.h>
#include <windowsx.h> 
#include <knownfolders.h> 


// Automatic linking of required libraries (for MSVC)
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")

namespace fs = std::filesystem;

// --- ID TANIMLAMALARI ---
#define WM_TRAYICON (WM_USER + 1)
#define WM_UPDATE_LONELITH_FILES (WM_USER + 2)
#define IDM_EXIT 100
#define IDM_SHOW 101

// Resource ID (Rar.exe'nin .rc dosyasında bu ID ile tanımlı olması gerekir)
#define IDR_RAR_EXE 101 
#define IDI_TRAY_NO_WINRAR       104
#define IDI_TRAY_NO_INTERNET     105
#define IDI_TRAY_CONNECTED       106

// Navigasyon
#define IDB_NAV_HOME 1001
#define IDB_NAV_SETTINGS 1002
#define IDB_NAV_INFO 1003
#define IDB_NAV_LONELITH 1004
#define IDB_NAV_CUSTOMIZATION 1013

// New Lonelith controls
#define IDB_LONELITH_UPLOAD 1005
#define IDB_LONELITH_REFRESH 1006
#define IDB_SPEED_TEST 1007
#define IDC_EDIT_AUTH_KEY 1008
#define IDB_SAVE_AUTH_KEY 1009
#define IDC_CHECK_AUTO_UPLOAD 1010
#define IDC_LONELITH_FILE_LIST 1011

// Progress bar
#define IDC_PROGRESS_BAR 1012

// Customization controls
#define IDC_COMBO_PROGRESS_MODE 1014
#define IDC_EDIT_PROGRESS_CUSTOM 1015
#define IDC_COMBO_TRAY_ICON 1016
#define IDB_APPLY_TRAY_ICON 1017
#define IDB_TOGGLE_THEME 1018  // Theme toggle button

// Kontroller
#define IDB_SELECT_FOLDER 200
#define IDB_SAVE_SETTINGS 202
#define IDC_CHECK_STARTUP 203
#define IDC_EDIT_PATH 204
#define IDC_CHECK_SILENT 205
#define IDC_CHECK_START_TRAY 206
#define IDB_RESET_APP 207 
#define IDC_CHECK_GOODBYE 208        
#define IDB_UNINSTALL_APP 209
#define IDC_EDIT_PASSWORD 210
#define IDB_CLEAR_LOG 211
#define IDB_TEST_GITHUB 212
#define IDC_COMBO_LONELITH_URL 213
#define IDC_EDIT_CUSTOM_URL 214

// Login Penceresi ID'leri
#define IDC_LOGIN_EDIT 301
#define IDB_LOGIN_BTN 302

// --- RENKLER (THEME COLORS) ---
// Light Theme Colors
const COLORREF CLR_LIGHT_BG_MAIN = RGB(248, 249, 250);
const COLORREF CLR_LIGHT_BG_SIDEBAR = RGB(240, 242, 245);
const COLORREF CLR_LIGHT_TEXT_MAIN = RGB(33, 37, 41);
const COLORREF CLR_LIGHT_TEXT_SECONDARY = RGB(108, 117, 125);
const COLORREF CLR_LIGHT_ACCENT = RGB(13, 110, 253);
const COLORREF CLR_LIGHT_BORDER = RGB(222, 226, 230);
const COLORREF CLR_LIGHT_INPUT_BG = RGB(255, 255, 255);

// Dark Theme Colors  
const COLORREF CLR_DARK_BG_MAIN = RGB(18, 18, 18);
const COLORREF CLR_DARK_BG_SIDEBAR = RGB(30, 30, 30);
const COLORREF CLR_DARK_TEXT_MAIN = RGB(230, 230, 230);
const COLORREF CLR_DARK_TEXT_SECONDARY = RGB(160, 160, 160);
const COLORREF CLR_DARK_ACCENT = RGB(99, 179, 237);
const COLORREF CLR_DARK_BORDER = RGB(60, 60, 60);
const COLORREF CLR_DARK_INPUT_BG = RGB(40, 40, 40);

// Common Colors (theme-independent)
const COLORREF CLR_DANGER = RGB(220, 53, 69);
const COLORREF CLR_SUCCESS = RGB(25, 135, 84);
const COLORREF CLR_WARNING = RGB(255, 193, 7);

// Active Theme Colors (will be updated based on theme)
COLORREF CLR_BG_MAIN = CLR_LIGHT_BG_MAIN;
COLORREF CLR_BG_SIDEBAR = CLR_LIGHT_BG_SIDEBAR;
COLORREF CLR_TEXT_MAIN = CLR_LIGHT_TEXT_MAIN;
COLORREF CLR_TEXT_SECONDARY = CLR_LIGHT_TEXT_SECONDARY;
COLORREF CLR_ACCENT = CLR_LIGHT_ACCENT;
COLORREF CLR_BORDER = CLR_LIGHT_BORDER;
COLORREF CLR_INPUT_BG = CLR_LIGHT_INPUT_BG;

// Layout constants
const int NAVBAR_HEIGHT = 60;
const int FOOTER_HEIGHT = 30;
const int PROGRESS_BAR_HEIGHT = 4;
const int TAB_COUNT = 5;  // Home, Lonelith, Settings, SysInfo, Customization

// Network test constants
const wchar_t* SPEED_TEST_URL = L"https://speed.cloudflare.com/__down?bytes=1000000";
const DWORD SPEED_TEST_SIZE = 1000000;  // 1MB

// Log settings
const int MAX_LOG_LINES = 500;
const int LOG_CLEANUP_LINES = 100;

// --- GLOBAL DEĞİŞKENLER ---
HINSTANCE g_hInst = NULL;
HWND g_hMainWindow = NULL;
NOTIFYICONDATA g_nid = {};
std::wstring g_targetPath = L"";
std::wstring g_appPassword = L"145366";
bool g_startWithWindows = false;
bool g_isAutoStart = false;
bool g_startInTray = false;
bool g_leaveGoodbyeNote = false;
bool g_loginSuccess = false;
HDEVNOTIFY g_hDeviceNotify = NULL;

// New global variables for features
bool g_hasWinRAR = false;
bool g_hasInternet = false;
std::wstring g_lonelithAuthKey = L"";
HICON g_hIconNoWinRAR = NULL;
HICON g_hIconNoInternet = NULL;
HICON g_hIconConnected = NULL;
HICON g_hIconDefault = NULL;
std::wstring g_lonelithUrl = L"localhost:3000";
std::wstring g_winrarPath = L"";
std::wstring g_githubTestContent = L"";

// Animation and progress globals
int g_animationOffset = 0;
int g_progressValue = 0;
bool g_isMarquee = true;
bool g_autoUpload = false;
std::wstring g_currentSpeed = L"";
std::wstring g_currentUploadSpeed = L"";
std::wstring g_lonelithServerHealth = L"Unknown";
std::wstring g_githubConnHealth = L"Unknown";

// Customization globals
int g_progressBarMode = 0;  // 0=Marquee, 1=Full, 2=Hide, 3=Custom
int g_customProgressValue = 50;
int g_selectedTrayIcon = 0;  // 0=Default, 1=NoWinRAR, 2=NoInternet, 3=Connected
bool g_manualTrayIconSelection = false;  // Track if user manually selected an icon
bool g_isDarkMode = false;  // Theme mode: false=Light, true=Dark

// Lonelith file list cache
std::vector<std::wstring> g_cachedLonelithFiles;
std::mutex g_lonelithFilesMutex;
bool g_isWindowAlive = true;

// Global brush for edit controls (theme-aware)
HBRUSH g_hBrushEdit = NULL;
bool g_brushesAreStock = false;  // Track if fallback stock brushes are used

// UI Kaynakları
HFONT g_hFontTitle, g_hFontSubtitle, g_hFontNormal, g_hFontSmall, g_hFontMono;
HBRUSH g_hBrushMainBg, g_hBrushSidebar;

// Global Kontrol Handle'ları
HWND g_hNavBtnHome, g_hNavBtnSettings, g_hNavBtnInfo, g_hNavBtnLonelith, g_hNavBtnCustomization;
HWND g_hThemeToggleBtn;  // Theme toggle button handle
HWND g_hPathDisplay, g_hStatusText;
HWND g_hCheckStartup, g_hCheckSilent, g_hEditDefaultPath, g_hCheckStartTray, g_hCheckGoodbye;
HWND g_hEditPassword;
HWND g_hInfoText;
HWND g_hProgressBar;
HWND g_hEditAuthKey, g_hCheckAutoUpload;
HWND g_hLonelithFileList;
HWND g_hSpeedTestResult, g_hUploadSpeedTestResult;
HWND g_hComboLonelithUrl;
HWND g_hEditCustomUrl;
HWND g_hGitHubTestResult;
HWND g_hComboProgressMode, g_hEditProgressCustom, g_hComboTrayIcon;

std::vector<HWND> g_tabControls[TAB_COUNT];

int g_currentTab = 0;
const wchar_t* CLASS_NAME = L"ShadowCopyApp";
const wchar_t* LOGIN_CLASS_NAME = L"ShadowCopyLogin";
const wchar_t* APP_REG_NAME = L"ShadowCopy";
const wchar_t* REG_SUBKEY = L"Software\\ShadowCopy";

// --- FONKSİYON PROTOTİPLERİ ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LoginWndProc(HWND, UINT, WPARAM, LPARAM);
void CreateUI(HWND hWnd);
void InitResources();
void CleanupResources();
void SwitchTab(int index);
void LogMessage(const std::wstring& message);
void StartBackupProcess(const std::wstring& sourceDrive); // İsim güncellendi
bool SelectTargetFolder();
void SaveSettings();
void LoadSettings();
void ResetApp(HWND hWnd);
void CreateTrayIcon();
void CheckExistingDrives();
void RemoveTrayIcon();
void ShowContextMenu(HWND hWnd);
std::wstring GetDefaultPath();
void PaintNavButton(LPDRAWITEMSTRUCT pDIS);
std::wstring GetSystemInfo();
std::wstring FormatBytes(uintmax_t bytes);
bool IsSilentMode();
void SendNotification(const std::wstring& title, const std::wstring& msg);

void SetModernStyle(HWND hControl);
void DestructionWatcher();
void PerformSelfDestruct(bool triggeredByFile);
bool ShowLoginDialog();
bool ExtractRarTool(std::wstring& outPath);

// New function prototypes
bool CheckWinRARInstalled();
bool CheckInternetConnection();
void UpdateTrayIcon();
void InternetMonitorThread();
std::wstring EncryptString(const std::wstring& plainText);
std::wstring DecryptString(const std::wstring& encryptedText);
void SaveAuthKey(const std::wstring& authKey);
std::wstring LoadAuthKey();
bool UploadFileToLonelith(const std::wstring& filePath);
std::vector<std::wstring> GetFilesFromLonelith();
bool ShowFileOnLonelith(const std::wstring& fileId);
void TestInternetSpeed();
void TestUploadSpeed();
void CheckGitHubConnection();
void CheckLonelithHealth();
void UpdateProgressBar(int value, bool marquee);
void AnimatePasswordBox(HWND hEdit, bool shake);
void SetupPageTransition(int fromTab, int toTab);
void ClearLog();
void TestGitHubConnectionManual();
void CheckLonelithUrlHealth(const std::wstring& url);
void ApplyProgressBarMode();
void ApplyTrayIconSelection();
void ToggleTheme();
void ApplyTheme();
void StyleTextBox(HWND hEdit, bool isMultiline = false);

// Yardımcı: UI Oluşturma
HWND CreateCtrl(int tabIndex, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hParent, HMENU hMenu) {
    // No sidebar adjustment needed - top navbar layout
    int adjustedY = y + NAVBAR_HEIGHT;
    HWND hCtrl = CreateWindowW(lpClassName, lpWindowName, dwStyle, x, adjustedY, nWidth, nHeight, hParent, hMenu, g_hInst, NULL);
    if (tabIndex >= 0 && tabIndex < TAB_COUNT) {
        g_tabControls[tabIndex].push_back(hCtrl);
    }
    return hCtrl;
}

HWND CreateLabel(int tabIndex, HWND hParent, LPCWSTR text, int x, int y, int w, int h, HFONT font)
{
    HWND hStatic = CreateCtrl(tabIndex, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, hParent, NULL);
    SendMessage(hStatic, WM_SETFONT, (WPARAM)font, TRUE);
    return hStatic;
}

// --- STARTUP MANAGER ---
namespace StartupManager {
    const wchar_t* REG_RUN_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    bool AddToStartup() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" --autostart";
            DWORD dataSize = static_cast<DWORD>((cmd.length() + 1) * sizeof(wchar_t));
            LONG result = RegSetValueExW(hKey, APP_REG_NAME, 0, REG_SZ, (BYTE*)cmd.c_str(), dataSize);
            RegCloseKey(hKey);
            return (result == ERROR_SUCCESS);
        }
        return false;
    }

    void RemoveFromStartup() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueW(hKey, APP_REG_NAME);
            RegCloseKey(hKey);
        }
    }

    bool IsInStartup() {
        HKEY hKey;
        bool exists = false;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExW(hKey, APP_REG_NAME, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) exists = true;
            RegCloseKey(hKey);
        }
        return exists;
    }
}

// --- SELF DESTRUCT ---
std::wstring GetDesktopPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L"";
}

// --- GÜNCELLENMİŞ SELF DESTRUCT (VERİ SİLME ÖZELLİKLİ) ---
void PerformSelfDestruct(bool triggeredByFile) {

    // 1. Adım: Elveda notu bırakma (Opsiyonel)
    if (g_leaveGoodbyeNote) {
        std::wstring notePath = GetDesktopPath() + L"\\elveda.txt";
        std::wofstream file(notePath);
        if (file.is_open()) {
            file << L"Sistem güvenliği ihlal edildi (Hatalı Parola / Manuel Tetikleme).\n";
            file << L"Tüm yedek dosyaları ve uygulama kalıcı olarak silindi.\nElveda.";
            file.close();
        }
    }

    // 2. Adım: Kayıt Defteri ve Başlangıç Temizliği
    StartupManager::RemoveFromStartup();
    SHDeleteKeyW(HKEY_CURRENT_USER, REG_SUBKEY);

    // 3. Adım: KRİTİK - YEDEK DOSYALARINI SİLME
    try {
        if (!g_targetPath.empty() && fs::exists(g_targetPath)) {
            // Kullanıcıya bilgi veremeyiz (pencere kapanıyor), ama işlemi yapıyoruz.
            std::error_code ec;

            // Klasörün içindeki her şeyi ve klasörün kendisini siler
            // fs::remove_all: İç içe klasörler dahil her şeyi siler.
            uintmax_t deletedCount = fs::remove_all(g_targetPath, ec);

            if (ec) {
                // Eğer silerken hata oluşursa (örn: dosya açıksa), 
                // en azından silebildiği kadarını silmeye çalışır.
                // Loglama yapılamaz çünkü uygulama kapanıyor.
            }
        }
    }
    catch (...) {
        // Beklenmeyen bir dosya sistemi hatasında programın çökmesini engelle
        // ve kendini silme aşamasına geç.
    }

    // 4. Adım: Tetikleyici dosyayı temizle
    if (triggeredByFile) {
        std::wstring triggerPath = GetDesktopPath() + L"\\sil321.txt";
        DeleteFileW(triggerPath.c_str());
    }

    // 5. Adım: Kendi kendini (EXE) silme mekanizması
    wchar_t szExePath[MAX_PATH];
    GetModuleFileNameW(NULL, szExePath, MAX_PATH);

    std::wstring batchPath = std::wstring(szExePath) + L".bat";
    std::wofstream batFile(batchPath);

    // Self-delete batch script'i
    batFile << L"@echo off\n";
    batFile << L":loop\n";
    batFile << L"del \"" << szExePath << L"\"\n"; // Exe'yi silmeyi dene
    batFile << L"if exist \"" << szExePath << L"\" goto loop\n"; // Silinmediyse tekrar dene (Process bitene kadar)
    batFile << L"del \"%~f0\"\n"; // Batch dosyasının kendisini sil
    batFile.close();

    // Batch dosyasını gizli modda çalıştır
    ShellExecuteW(NULL, L"open", batchPath.c_str(), NULL, NULL, SW_HIDE);

    // 6. Adım: Programı sonlandır
    RemoveTrayIcon();
    ExitProcess(0);
}

void DestructionWatcher() {
    std::wstring triggerFile = L"sil321.txt";
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        std::wstring desktop = GetDesktopPath();
        if (desktop.empty()) continue;
        std::wstring fullPath = desktop + L"\\" + triggerFile;
        if (fs::exists(fullPath)) {
            PerformSelfDestruct(true);
            break;
        }
    }
}
void RegisterDeviceNotifications(HWND hWnd) {
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    // USB Cihazları için GUID
    // GUID_DEVINTERFACE_USB_DEVICE
    // Ancak VOLUME (Sürücü) olaylarını garanti altına almak için handle'ı saklıyoruz.
    // GUI uygulamaları varsayılan olarak VOLUME mesajlarını alır ama bu işlem sağlamlaştırır.
    g_hDeviceNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
}
// Bu fonksiyon sürücünün tamamen hazır olmasını bekler
// Sürücünün tamamen hazır ve dosya sisteminin erişilebilir olmasını bekleyen geliştirilmiş fonksiyon
void SmartBackupStarter(std::wstring drivePath) {
    LogMessage(L"⏳ Sürücü algılandı, dosya sistemi bekleniyor: " + drivePath);

    // Maksimum bekleme süresi: 30 saniye (30 x 1000ms)
    // Windows'un sürücü harfini ataması ve dosya sistemini bağlaması bazen uzun sürer.
    int maxRetries = 30;
    bool isReady = false;

    wchar_t volumeName[MAX_PATH + 1] = { 0 };
    wchar_t fileSysName[MAX_PATH + 1] = { 0 };
    DWORD serialNumber = 0;
    DWORD maxCompLen = 0;
    DWORD fileSysFlags = 0;

    for (int i = 0; i < maxRetries; i++) {
        // 1. Adım: Sürücü tipi kontrolü
        UINT type = GetDriveTypeW(drivePath.c_str());

        // Sürücü henüz 'Bilinmiyor' veya 'Kök dizin yok' durumundaysa beklemeye devam et
        if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {

            // 2. Adım: Kritik Kontrol - GetVolumeInformationW
            // Bu fonksiyon SADECE dosya sistemi tamamen mount edildiyse ve okunabilirse başarılı olur.
            // fs::exists bazen sürücü harfi rezerve edildiğinde bile true dönebilir, bu yüzden yetersizdir.
            if (GetVolumeInformationW(
                drivePath.c_str(),
                volumeName,
                MAX_PATH,
                &serialNumber,
                &maxCompLen,
                &fileSysFlags,
                fileSysName,
                MAX_PATH))
            {
                isReady = true;
                break; // Sürücü tamamen hazır!
            }
            else {
                // Erişim hatası varsa logla (Debug amaçlı)
                DWORD err = GetLastError();
                if (err == ERROR_NOT_READY || err == ERROR_ACCESS_DENIED) {
                    // Sürücü henüz hazır değil, döngüye devam et.
                }
            }
        }

        // 1 saniye bekle ve tekrar dene
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (isReady) {
        std::wstringstream ss;
        ss << L"✅ Sürücü Hazır: " << drivePath << L" [" << fileSysName << L"] (" << volumeName << L")";
        LogMessage(ss.str());

        // Stabilite için ekstra yarım saniye bekleme (IO işlemlerinin başlaması için)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        StartBackupProcess(drivePath);
    }
    else {
        LogMessage(L"❌ Sürücü zaman aşımına uğradı (30sn) veya erişilemedi: " + drivePath);
    }
}// --- LOGIN DIALOG ---
int g_wrongAttempts = 0;
const int MAX_ATTEMPTS = 3;
HWND g_hLoginEditCtrl = NULL;

LRESULT CALLBACK LoginWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Consolas");

       // CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 20, 260, 20, hWnd, NULL, g_hInst, NULL);
        g_hLoginEditCtrl = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD | ES_CENTER, 30, 30, 220, 25, hWnd, (HMENU)IDC_LOGIN_EDIT, g_hInst, NULL);
        SendMessage(g_hLoginEditCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);
        HWND hBtn = CreateWindowW(L"BUTTON", L"✔", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 150, 90, 30, 30, hWnd, (HMENU)IDB_LOGIN_BTN, g_hInst, NULL);
        SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        SetFocus(g_hLoginEditCtrl);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDB_LOGIN_BTN) {
            wchar_t buf[64];
            GetWindowTextW(g_hLoginEditCtrl, buf, 64);
            if (wcscmp(buf, g_appPassword.c_str()) == 0) {
                g_loginSuccess = true;
                g_wrongAttempts = 0;
                DestroyWindow(hWnd);
            }
            else {
                g_wrongAttempts++;
                int remaining = MAX_ATTEMPTS - g_wrongAttempts;
                
                // Animate the password box with shake effect
                AnimatePasswordBox(g_hLoginEditCtrl, true);
                
                std::wstring msg = L"Hatalı Parola!\nKalan Hakkınız: " + std::to_wstring(remaining);
                MessageBoxW(hWnd, msg.c_str(), L"Erişim Reddedildi", MB_ICONERROR);
                SetWindowTextW(g_hLoginEditCtrl, L"");
                SetFocus(g_hLoginEditCtrl);
                if (g_wrongAttempts >= MAX_ATTEMPTS) {
                    g_loginSuccess = false;
                    DestroyWindow(hWnd);
                }
            }
        }
        break;
    case WM_CLOSE:
        g_loginSuccess = false;
        DestroyWindow(hWnd);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool ShowLoginDialog() {
    g_loginSuccess = false;
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = LoginWndProc;
    wcex.hInstance = g_hInst;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = LOGIN_CLASS_NAME;
    RegisterClassExW(&wcex);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int w = 300, h = 180;
    int x = (screenW - w) / 2;
    int y = (screenH - h) / 2;

    HWND hLoginWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, LOGIN_CLASS_NAME, L"👀",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, w, h, NULL, NULL, g_hInst, NULL);

    EnableWindow(g_hMainWindow, FALSE);
    MSG msg;
    while (IsWindow(hLoginWnd) && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(g_hMainWindow, TRUE);
    UnregisterClassW(LOGIN_CLASS_NAME, g_hInst);

    if (!g_loginSuccess && g_wrongAttempts >= MAX_ATTEMPTS) {
        PerformSelfDestruct(false);
        return false;
    }
    return g_loginSuccess;
}

// --- NEW FEATURE IMPLEMENTATIONS ---

// Check if WinRAR is installed on the system
bool CheckWinRARInstalled() {
    // Check common WinRAR installation paths
    std::vector<std::wstring> possiblePaths = {
        L"C:\\Program Files\\WinRAR\\WinRAR.exe",
        L"C:\\Program Files (x86)\\WinRAR\\WinRAR.exe"
    };
    
    for (const auto& path : possiblePaths) {
        if (fs::exists(path)) {
            g_winrarPath = path;
            return true;
        }
    }
    
    // Check registry for WinRAR installation
    HKEY hKey;
    bool found = false;
    wchar_t pathBuf[MAX_PATH] = {0};
    DWORD bufSize = sizeof(pathBuf);
    
    // Try 64-bit registry location first
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WinRAR", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // Try common registry value names
        const wchar_t* valueNames[] = {L"exe64", L"exe", L"ExePath"};
        for (const auto& valueName : valueNames) {
            bufSize = sizeof(pathBuf);
            if (RegQueryValueExW(hKey, valueName, NULL, NULL, (BYTE*)pathBuf, &bufSize) == ERROR_SUCCESS) {
                g_winrarPath = std::wstring(pathBuf);
                found = true;
                break;
            }
        }
        RegCloseKey(hKey);
    }
    
    // Try 32-bit registry location (WOW6432Node on 64-bit Windows)
    if (!found && RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\WinRAR", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        const wchar_t* valueNames[] = {L"exe32", L"exe", L"ExePath"};
        for (const auto& valueName : valueNames) {
            bufSize = sizeof(pathBuf);
            if (RegQueryValueExW(hKey, valueName, NULL, NULL, (BYTE*)pathBuf, &bufSize) == ERROR_SUCCESS) {
                g_winrarPath = std::wstring(pathBuf);
                found = true;
                break;
            }
        }
        RegCloseKey(hKey);
    }
    
    if (!found) {
        g_winrarPath = L"Not Installed";
    }
    
    return found;
}

// Check internet connection
bool CheckInternetConnection() {
    DWORD flags;
    // Try to connect to a reliable server to check internet
    BOOL result = InternetGetConnectedState(&flags, 0);
    
    if (result) {
        // Double-check with actual connection attempt using HTTPS for security
        HINTERNET hInternet = InternetOpenW(L"ShadowCopy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (hInternet) {
            HINTERNET hUrl = InternetOpenUrlW(hInternet, L"https://raw.githubusercontent.com/prescionx/ConnectionTest/refs/heads/main/connection.txt", NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
            if (hUrl) {
                InternetCloseHandle(hUrl);
                InternetCloseHandle(hInternet);
                return true;
            }
            InternetCloseHandle(hInternet);
        }
    }
    
    return false;
}

// Update the tray icon based on current status
void UpdateTrayIcon() {
    // Don't auto-update if user manually selected an icon
    if (g_manualTrayIconSelection) {
        return;
    }
    
    HICON newIcon = NULL;
    std::wstring tooltip = L"ShadowCopy";
    
    if (!g_hasWinRAR) {
        newIcon = g_hIconNoWinRAR;
        tooltip = L"ShadowCopy - WinRAR Not Found";
    }
    else if (!g_hasInternet) {
        newIcon = g_hIconNoInternet;
        tooltip = L"ShadowCopy - No Internet Connection";
    }
    else {
        newIcon = g_hIconConnected;
        tooltip = L"ShadowCopy - Connected";
    }
    
    if (newIcon) {
        g_nid.hIcon = newIcon;
        wcscpy_s(g_nid.szTip, tooltip.c_str());
        Shell_NotifyIcon(NIM_MODIFY, &g_nid);
    }
}

// Thread function to monitor internet connection every 15 seconds
void InternetMonitorThread() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        
        bool previousState = g_hasInternet;
        g_hasInternet = CheckInternetConnection();
        
        if (previousState != g_hasInternet) {
            UpdateTrayIcon();
            if (g_hasInternet) {
                LogMessage(L"✅ Internet bağlantısı aktif.");
            }
            else {
                LogMessage(L"⚠️ Internet bağlantısı kesildi.");
            }
        }
    }
}

// Encrypt string using Windows DPAPI
std::wstring EncryptString(const std::wstring& plainText) {
    if (plainText.empty()) return L"";
    
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
    
    dataIn.pbData = (BYTE*)plainText.c_str();
    dataIn.cbData = (DWORD)((plainText.length() + 1) * sizeof(wchar_t));
    
    if (CryptProtectData(&dataIn, L"ShadowCopyAuthKey", NULL, NULL, NULL, 0, &dataOut)) {
        // Convert to hex string for storage
        std::wstringstream ss;
        for (DWORD i = 0; i < dataOut.cbData; i++) {
            ss << std::hex << std::setw(2) << std::setfill(L'0') << (int)dataOut.pbData[i];
        }
        LocalFree(dataOut.pbData);
        return ss.str();
    }
    
    return L"";
}

// Decrypt string using Windows DPAPI
std::wstring DecryptString(const std::wstring& encryptedText) {
    if (encryptedText.empty()) return L"";
    
    // Convert hex string back to bytes
    std::vector<BYTE> bytes;
    for (size_t i = 0; i < encryptedText.length(); i += 2) {
        std::wstring byteString = encryptedText.substr(i, 2);
        BYTE byte = (BYTE)wcstol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
    
    dataIn.pbData = bytes.data();
    dataIn.cbData = (DWORD)bytes.size();
    
    if (CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
        std::wstring result((wchar_t*)dataOut.pbData);
        LocalFree(dataOut.pbData);
        return result;
    }
    
    return L"";
}

// Save encrypted auth key to registry
void SaveAuthKey(const std::wstring& authKey) {
    std::wstring encrypted = EncryptString(authKey);
    
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD dataSize = (DWORD)((encrypted.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"LonelithAuthKey", 0, REG_SZ, (BYTE*)encrypted.c_str(), dataSize);
        RegCloseKey(hKey);
    }
}

// Load and decrypt auth key from registry
std::wstring LoadAuthKey() {
    HKEY hKey;
    wchar_t buffer[1024] = {0};
    DWORD bufferSize = sizeof(buffer);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"LonelithAuthKey", NULL, NULL, (BYTE*)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return DecryptString(buffer);
        }
        RegCloseKey(hKey);
    }
    
    return L"";
}

// Upload file to Lonelith server using HTTP POST with multipart/form-data
// Based on: curl -X POST http://localhost:3000/upload -H "X-API-Key: your-secret-api-key-here" -F "file=@/path/to/your/file.txt"
bool UploadFileToLonelith(const std::wstring& filePath) {
    if (!g_hasInternet) {
        LogMessage(L"⚠️ İnternet bağlantısı yok, dosya yüklenemedi.");
        return false;
    }
    
    if (g_lonelithAuthKey.empty()) {
        LogMessage(L"⚠️ Lonelith auth key ayarlanmamış.");
        return false;
    }
    
    if (!fs::exists(filePath)) {
        LogMessage(L"⚠️ Dosya bulunamadı: " + filePath);
        return false;
    }
    
    LogMessage(L"📤 Lonelith'e yükleme başlatılıyor: " + filePath);
    
    // Parse URL to extract server and port
    std::wstring serverUrl = g_lonelithUrl;
    std::wstring serverHost;
    INTERNET_PORT serverPort = INTERNET_DEFAULT_HTTP_PORT;
    
    // Extract host and port from URL (e.g., "localhost:3000" or "lonelith.556.space")
    size_t colonPos = serverUrl.find(L':');
    if (colonPos != std::wstring::npos) {
        serverHost = serverUrl.substr(0, colonPos);
        serverPort = (INTERNET_PORT)_wtoi(serverUrl.substr(colonPos + 1).c_str());
    } else {
        serverHost = serverUrl;
    }
    
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        LogMessage(L"❌ Internet session oluşturulamadı.");
        return false;
    }
    
    HINTERNET hConnect = InternetConnectW(hInternet, serverHost.c_str(), serverPort, 
        NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    
    if (!hConnect) {
        LogMessage(L"❌ Server'a bağlanılamadı: " + serverHost);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", L"/upload", NULL, NULL, NULL, 
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    
    if (!hRequest) {
        LogMessage(L"❌ HTTP request oluşturulamadı.");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    // Read file content
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LogMessage(L"❌ Dosya okunamadı.");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> fileContent(fileSize);
    file.read(fileContent.data(), fileSize);
    file.close();
    
    // Extract filename from path
    std::wstring fileName = fs::path(filePath).filename().wstring();
    
    // Create multipart/form-data boundary
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    
    // Build multipart body
    std::stringstream bodyStream;
    bodyStream << "--" << boundary << "\r\n";
    bodyStream << "Content-Disposition: form-data; name=\"file\"; filename=\"";
    
    // Convert filename to UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, fileName.c_str(), -1, NULL, 0, NULL, NULL);
    std::vector<char> utf8FileName(utf8Size);
    WideCharToMultiByte(CP_UTF8, 0, fileName.c_str(), -1, utf8FileName.data(), utf8Size, NULL, NULL);
    
    bodyStream << utf8FileName.data() << "\"\r\n";
    bodyStream << "Content-Type: application/octet-stream\r\n\r\n";
    
    std::string bodyPrefix = bodyStream.str();
    std::string bodySuffix = "\r\n--" + boundary + "--\r\n";
    
    // Combine all parts
    std::vector<char> fullBody;
    fullBody.insert(fullBody.end(), bodyPrefix.begin(), bodyPrefix.end());
    fullBody.insert(fullBody.end(), fileContent.begin(), fileContent.end());
    fullBody.insert(fullBody.end(), bodySuffix.begin(), bodySuffix.end());
    
    // Prepare headers
    std::wstring authHeader = L"X-API-Key: " + g_lonelithAuthKey;
    std::wstring contentTypeHeader = L"Content-Type: multipart/form-data; boundary=" + 
        std::wstring(boundary.begin(), boundary.end());
    
    std::wstring allHeaders = authHeader + L"\r\n" + contentTypeHeader;
    
    // Send the request
    BOOL result = HttpSendRequestW(hRequest, allHeaders.c_str(), allHeaders.length(), 
        fullBody.data(), (DWORD)fullBody.size());
    
    if (!result) {
        LogMessage(L"❌ Dosya gönderilemedi.");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    // Check response status
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
        &statusCode, &statusCodeSize, NULL);
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    if (statusCode == 200 || statusCode == 201) {
        LogMessage(L"✅ Dosya başarıyla yüklendi!");
        return true;
    } else {
        LogMessage(L"⚠️ Server yanıtı: HTTP " + std::to_wstring(statusCode));
        return false;
    }
}

// Get list of files from Lonelith server
// Based on: curl -X GET http://localhost:3000/files -H "X-API-Key: your-secret-api-key-here"
std::vector<std::wstring> GetFilesFromLonelith() {
    std::vector<std::wstring> files;
    
    if (!g_hasInternet) {
        LogMessage(L"⚠️ İnternet bağlantısı yok.");
        return files;
    }
    
    if (g_lonelithAuthKey.empty()) {
        LogMessage(L"⚠️ Lonelith auth key ayarlanmamış.");
        return files;
    }
    
    LogMessage(L"📋 Lonelith dosya listesi alınıyor...");
    
    // Parse URL
    std::wstring serverUrl = g_lonelithUrl;
    std::wstring serverHost;
    INTERNET_PORT serverPort = INTERNET_DEFAULT_HTTP_PORT;
    
    size_t colonPos = serverUrl.find(L':');
    if (colonPos != std::wstring::npos) {
        serverHost = serverUrl.substr(0, colonPos);
        serverPort = (INTERNET_PORT)_wtoi(serverUrl.substr(colonPos + 1).c_str());
    } else {
        serverHost = serverUrl;
    }
    
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        LogMessage(L"❌ Internet session oluşturulamadı.");
        return files;
    }
    
    HINTERNET hConnect = InternetConnectW(hInternet, serverHost.c_str(), serverPort, 
        NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    
    if (!hConnect) {
        LogMessage(L"❌ Server'a bağlanılamadı.");
        InternetCloseHandle(hInternet);
        return files;
    }
    
    HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", L"/files", NULL, NULL, NULL, 
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    
    if (!hRequest) {
        LogMessage(L"❌ HTTP request oluşturulamadı.");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return files;
    }
    
    // Add auth header
    std::wstring authHeader = L"X-API-Key: " + g_lonelithAuthKey;
    HttpAddRequestHeadersW(hRequest, authHeader.c_str(), authHeader.length(), 
        HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
    
    // Send request
    if (!HttpSendRequestW(hRequest, NULL, 0, NULL, 0)) {
        LogMessage(L"❌ Request gönderilemedi.");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return files;
    }
    
    // Read response
    char buffer[4096];
    DWORD bytesRead = 0;
    std::string response;
    
    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        response += buffer;
    }
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    // Parse JSON response (simple parsing - assumes array of filename strings)
    // For a full implementation, use a JSON library
    // For now, we'll do simple string parsing
    if (!response.empty()) {
        // Convert response to wide string
        int wideSize = MultiByteToWideChar(CP_UTF8, 0, response.c_str(), -1, NULL, 0);
        std::vector<wchar_t> wideResponse(wideSize);
        MultiByteToWideChar(CP_UTF8, 0, response.c_str(), -1, wideResponse.data(), wideSize);
        
        LogMessage(L"✅ Dosya listesi alındı.");
        LogMessage(L"Yanıt: " + std::wstring(wideResponse.data()));
        
        // Simple parsing: look for quoted strings (filenames)
        // Note: This is basic parsing and may not handle all JSON edge cases
        // For production, consider using a JSON library
        std::wstring responseStr(wideResponse.data());
        size_t pos = 0;
        while ((pos = responseStr.find(L'"', pos)) != std::wstring::npos) {
            size_t endPos = responseStr.find(L'"', pos + 1);
            if (endPos != std::wstring::npos) {
                std::wstring filename = responseStr.substr(pos + 1, endPos - pos - 1);
                // Accept any non-empty filename (not just those with dots)
                if (!filename.empty() && filename.length() > 0 && 
                    filename.find(L'{') == std::wstring::npos && 
                    filename.find(L'}') == std::wstring::npos) {
                    files.push_back(filename);
                }
                pos = endPos + 1;
            } else {
                break;
            }
        }
    }
    
    return files;
}

// Download file from Lonelith server
// Based on: curl -X GET http://localhost:3000/download/1234567890-file.txt -H "X-API-Key: your-secret-api-key-here" -o downloaded-file.txt
bool ShowFileOnLonelith(const std::wstring& fileId) {
    if (!g_hasInternet) {
        LogMessage(L"⚠️ İnternet bağlantısı yok.");
        return false;
    }
    
    if (g_lonelithAuthKey.empty()) {
        LogMessage(L"⚠️ Lonelith auth key ayarlanmamış.");
        return false;
    }
    
    LogMessage(L"📥 Lonelith'ten dosya indiriliyor: " + fileId);
    
    // Parse URL
    std::wstring serverUrl = g_lonelithUrl;
    std::wstring serverHost;
    INTERNET_PORT serverPort = INTERNET_DEFAULT_HTTP_PORT;
    
    size_t colonPos = serverUrl.find(L':');
    if (colonPos != std::wstring::npos) {
        serverHost = serverUrl.substr(0, colonPos);
        serverPort = (INTERNET_PORT)_wtoi(serverUrl.substr(colonPos + 1).c_str());
    } else {
        serverHost = serverUrl;
    }
    
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        LogMessage(L"❌ Internet session oluşturulamadı.");
        return false;
    }
    
    HINTERNET hConnect = InternetConnectW(hInternet, serverHost.c_str(), serverPort, 
        NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    
    if (!hConnect) {
        LogMessage(L"❌ Server'a bağlanılamadı.");
        InternetCloseHandle(hInternet);
        return false;
    }
    
    // Build download path
    std::wstring downloadPath = L"/download/" + fileId;
    
    HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", downloadPath.c_str(), NULL, NULL, NULL, 
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    
    if (!hRequest) {
        LogMessage(L"❌ HTTP request oluşturulamadı.");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    // Add auth header
    std::wstring authHeader = L"X-API-Key: " + g_lonelithAuthKey;
    HttpAddRequestHeadersW(hRequest, authHeader.c_str(), authHeader.length(), 
        HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
    
    // Send request
    if (!HttpSendRequestW(hRequest, NULL, 0, NULL, 0)) {
        LogMessage(L"❌ Request gönderilemedi.");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
        &statusCode, &statusCodeSize, NULL);
    
    if (statusCode != 200) {
        LogMessage(L"⚠️ Dosya bulunamadı veya erişim hatası. HTTP " + std::to_wstring(statusCode));
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    // Save to downloads folder
    wchar_t downloadsPath[MAX_PATH];
    // Use CSIDL_DOWNLOADS for Downloads folder (Vista+)
    // Falls back to Documents if Downloads folder is not available
    PWSTR pszPath = NULL;
    // Modern API (Vista+) ile İndirilenler klasörünü bulmaya çalış
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &pszPath);

    if (SUCCEEDED(hr)) {
        wcscpy_s(downloadsPath, MAX_PATH, pszPath);
        CoTaskMemFree(pszPath); // Hafızayı temizle
    }
    else {
        // Eğer İndirilenler bulunamazsa Belgelerim klasörüne dön
        hr = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, downloadsPath);
    }
    
    if (SUCCEEDED(hr)) {
        std::wstring savePath = std::wstring(downloadsPath) + L"\\" + fileId;
        
        // Read and save file
        std::ofstream outFile(savePath, std::ios::binary);
        if (!outFile.is_open()) {
            LogMessage(L"❌ Dosya kaydedilemedi.");
            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
        }
        
        char buffer[8192];
        DWORD bytesRead = 0;
        DWORD totalBytes = 0;
        
        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            outFile.write(buffer, bytesRead);
            totalBytes += bytesRead;
        }
        
        outFile.close();
        
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        LogMessage(L"✅ Dosya indirildi: " + savePath + L" (" + FormatBytes(totalBytes) + L")");
        
        // Open the downloaded file
        ShellExecuteW(NULL, L"open", savePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        
        return true;
    }
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    return false;
}

// Test internet speed
void TestInternetSpeed() {
    if (!g_hasInternet) {
        g_currentSpeed = L"İnternet bağlantısı yok";
        LogMessage(L"⚠️ İnternet bağlantısı yok, hız testi yapılamıyor.");
        return;
    }
    
    LogMessage(L"🚀 İnternet hız testi başlatılıyor...");
    UpdateProgressBar(0, false);
    
    // Simple speed test: Download a test file and measure time
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet) {
        DWORD startTime = GetTickCount();
        
        // Download test file
        HINTERNET hUrl = InternetOpenUrlW(hInternet, 
            SPEED_TEST_URL, 
            NULL, 0, 
            INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
        
        if (hUrl) {
            BYTE buffer[8192];
            DWORD bytesRead = 0;
            DWORD totalBytes = 0;
            
            while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                totalBytes += bytesRead;
                UpdateProgressBar((totalBytes * 100) / SPEED_TEST_SIZE, false);
            }
            
            DWORD elapsedMs = GetTickCount() - startTime;
            if (elapsedMs > 0) {
                double speedMbps = (totalBytes * 8.0 / 1000000.0) / (elapsedMs / 1000.0);
                wchar_t speedBuf[128];
                swprintf_s(speedBuf, L"%.2f Mbps", speedMbps);
                g_currentSpeed = speedBuf;
                
                LogMessage(L"✅ İndirme hızı: " + g_currentSpeed);
                if (g_hSpeedTestResult) {
                    SetWindowTextW(g_hSpeedTestResult, g_currentSpeed.c_str());
                }
            }
            
            InternetCloseHandle(hUrl);
        } else {
            g_currentSpeed = L"Test başarısız";
            LogMessage(L"⚠️ Hız testi başarısız oldu.");
        }
        
        InternetCloseHandle(hInternet);
    }
    
    // Now test upload speed
    TestUploadSpeed();
    
    UpdateProgressBar(0, true);  // Return to marquee
}

// Test upload speed
void TestUploadSpeed() {
    if (!g_hasInternet) {
        g_currentUploadSpeed = L"İnternet bağlantısı yok";
        LogMessage(L"⚠️ İnternet bağlantısı yok, yükleme hız testi yapılamıyor.");
        return;
    }
    
    LogMessage(L"🚀 Yükleme hız testi başlatılıyor...");
    
    // Note: Using httpbin.org for testing (a common HTTP testing service)
    // If this service is unavailable, the test will fail gracefully
    // Consider making this configurable for production use
    
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet) {
        // Use httpbin.org for testing
        HINTERNET hConnect = InternetConnectW(hInternet, L"httpbin.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        
        if (hConnect) {
            HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", L"/post", NULL, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
            
            if (hRequest) {
                // Create test data (100KB)
                const DWORD testDataSize = 100000;
                std::vector<BYTE> testData(testDataSize, 'A');
                
                DWORD startTime = GetTickCount();
                
                // Send the request
                if (HttpSendRequestW(hRequest, NULL, 0, testData.data(), testDataSize)) {
                    DWORD elapsedMs = GetTickCount() - startTime;
                    
                    if (elapsedMs > 0) {
                        double speedMbps = (testDataSize * 8.0 / 1000000.0) / (elapsedMs / 1000.0);
                        wchar_t speedBuf[128];
                        swprintf_s(speedBuf, L"%.2f Mbps", speedMbps);
                        g_currentUploadSpeed = speedBuf;
                        
                        LogMessage(L"✅ Yükleme hızı: " + g_currentUploadSpeed);
                        if (g_hUploadSpeedTestResult) {
                            SetWindowTextW(g_hUploadSpeedTestResult, g_currentUploadSpeed.c_str());
                        }
                    }
                }
                else {
                    g_currentUploadSpeed = L"Test başarısız";
                    LogMessage(L"⚠️ Yükleme hız testi başarısız oldu.");
                }
                
                InternetCloseHandle(hRequest);
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    else {
        g_currentUploadSpeed = L"Bağlantı hatası";
        LogMessage(L"⚠️ Upload speed test connection error.");
    }
}

// Check GitHub connection.txt file
void CheckGitHubConnection() {
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hUrl = InternetOpenUrlW(hInternet, 
            L"https://raw.githubusercontent.com/prescionx/ConnectionTest/refs/heads/main/connection.txt", 
            NULL, 0, 
            INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
        
        if (hUrl) {
            g_githubConnHealth = L"✅ Connected";
            LogMessage(L"✅ GitHub bağlantısı başarılı.");
            InternetCloseHandle(hUrl);
        } else {
            g_githubConnHealth = L"❌ Failed";
            LogMessage(L"⚠️ GitHub bağlantısı başarısız.");
        }
        
        InternetCloseHandle(hInternet);
    } else {
        g_githubConnHealth = L"❌ No Internet";
    }
}

// Check Lonelith server health
void CheckLonelithHealth() {
    if (!g_hasInternet) {
        g_lonelithServerHealth = L"❌ No Internet";
        return;
    }
    
    // TODO: Implement actual health check when API is available
    // Would check /health endpoint
    g_lonelithServerHealth = L"⚠️ API Not Available";
    LogMessage(L"ℹ️ Lonelith health check - API detayları için repo erişimi gerekli.");
}

// Update progress bar
void UpdateProgressBar(int value, bool marquee) {
    g_progressValue = value;
    g_isMarquee = marquee;
    
    if (g_hProgressBar) {
        if (marquee) {
            SetWindowLongPtr(g_hProgressBar, GWL_STYLE, 
                GetWindowLongPtr(g_hProgressBar, GWL_STYLE) | PBS_MARQUEE);
            SendMessage(g_hProgressBar, PBM_SETMARQUEE, TRUE, 50);
        } else {
            SetWindowLongPtr(g_hProgressBar, GWL_STYLE, 
                GetWindowLongPtr(g_hProgressBar, GWL_STYLE) & ~PBS_MARQUEE);
            SendMessage(g_hProgressBar, PBM_SETMARQUEE, FALSE, 0);
            SendMessage(g_hProgressBar, PBM_SETPOS, value, 0);
        }
    }
}

// Animate password box (shake on error)
void AnimatePasswordBox(HWND hEdit, bool shake) {
    if (!hEdit) return;
    
    if (shake) {
        RECT rc;
        GetWindowRect(hEdit, &rc);
        POINT pt = {rc.left, rc.top};
        ScreenToClient(GetParent(hEdit), &pt);
        
        // Note: Using Sleep() for shake animation is intentional
        // This blocks the UI briefly (300ms total) to provide immediate visual feedback
        // Alternative: Use SetTimer for non-blocking animation if needed
        
        // Shake animation
        for (int i = 0; i < 3; i++) {
            SetWindowPos(hEdit, NULL, pt.x + 5, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            Sleep(50);
            SetWindowPos(hEdit, NULL, pt.x - 5, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            Sleep(50);
        }
        SetWindowPos(hEdit, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        
        // Flash red background
        HWND hParent = GetParent(hEdit);
        HDC hdc = GetDC(hEdit);
        RECT editRc;
        GetClientRect(hEdit, &editRc);
        
        for (int i = 0; i < 2; i++) {
            HBRUSH redBrush = CreateSolidBrush(RGB(255, 200, 200));
            FrameRect(hdc, &editRc, redBrush);
            DeleteObject(redBrush);
            Sleep(100);
            InvalidateRect(hEdit, NULL, TRUE);
            Sleep(100);
        }
        
        ReleaseDC(hEdit, hdc);
    }
}

// Setup page transition animation
void SetupPageTransition(int fromTab, int toTab) {
    // Simple fade transition by hiding old and showing new
    // More complex transitions could use SetLayeredWindowAttributes
    
    // For now, just do instant switch with future potential for animation
    // This can be enhanced with actual slide animations if needed
    g_animationOffset = 0;
}

// Clear log function
void ClearLog() {
    if (g_hStatusText) {
        SetWindowTextW(g_hStatusText, L"");
        LogMessage(L"📋 Günlük temizlendi.");
    }
}

// Manual GitHub connection test with content display
void TestGitHubConnectionManual() {
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hUrl = InternetOpenUrlW(hInternet, 
            L"https://raw.githubusercontent.com/prescionx/ConnectionTest/refs/heads/main/connection.txt", 
            NULL, 0, 
            INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
        
        if (hUrl) {
            char buffer[4096] = {0};
            DWORD bytesRead = 0;
            std::string content;
            
            // Read the content
            while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = 0;
                content += buffer;
            }
            
            // Convert to wide string
            int wideSize = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, NULL, 0);
            std::vector<wchar_t> wideContent(wideSize);
            MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, wideContent.data(), wideSize);
            
            g_githubTestContent = wideContent.data();
            g_githubConnHealth = L"✅ Connected";
            
            LogMessage(L"✅ GitHub bağlantı testi başarılı!");
            LogMessage(L"📄 İçerik: " + g_githubTestContent);
            
            // Update UI
            if (g_hGitHubTestResult) {
                SetWindowTextW(g_hGitHubTestResult, g_githubConnHealth.c_str());
            }
            
            InternetCloseHandle(hUrl);
        } else {
            g_githubConnHealth = L"❌ Failed";
            g_githubTestContent = L"";
            LogMessage(L"⚠️ GitHub bağlantı testi başarısız!");
            
            if (g_hGitHubTestResult) {
                SetWindowTextW(g_hGitHubTestResult, g_githubConnHealth.c_str());
            }
        }
        
        InternetCloseHandle(hInternet);
    } else {
        g_githubConnHealth = L"❌ No Internet";
        g_githubTestContent = L"";
        LogMessage(L"⚠️ İnternet bağlantısı yok!");
        
        if (g_hGitHubTestResult) {
            SetWindowTextW(g_hGitHubTestResult, g_githubConnHealth.c_str());
        }
    }
}

// Check Lonelith URL health
void CheckLonelithUrlHealth(const std::wstring& url) {
    if (!g_hasInternet) {
        g_lonelithServerHealth = L"❌ No Internet";
        LogMessage(L"⚠️ İnternet bağlantısı yok, Lonelith sağlık kontrolü yapılamıyor.");
        return;
    }
    
    // Construct health endpoint URL
    std::wstring healthUrl = L"http://" + url + L"/health";
    
    HINTERNET hInternet = InternetOpenW(L"ShadowCopy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hUrl = InternetOpenUrlW(hInternet, healthUrl.c_str(), NULL, 0, 
            INTERNET_FLAG_NO_CACHE_WRITE, 0);
        
        if (hUrl) {
            g_lonelithServerHealth = L"✅ Healthy";
            LogMessage(L"✅ Lonelith server sağlıklı: " + url);
            InternetCloseHandle(hUrl);
        } else {
            g_lonelithServerHealth = L"❌ Unreachable";
            LogMessage(L"⚠️ Lonelith server erişilemez: " + url);
        }
        
        InternetCloseHandle(hInternet);
    } else {
        g_lonelithServerHealth = L"❌ Connection Error";
        LogMessage(L"⚠️ Bağlantı hatası.");
    }
}

// Apply progress bar mode based on selection
void ApplyProgressBarMode() {
    if (!g_hProgressBar) return;
    
    switch (g_progressBarMode) {
        case 0: // Marquee
            ShowWindow(g_hProgressBar, SW_SHOW);
            SetWindowLongPtr(g_hProgressBar, GWL_STYLE, 
                GetWindowLongPtr(g_hProgressBar, GWL_STYLE) | PBS_MARQUEE);
            SendMessage(g_hProgressBar, PBM_SETMARQUEE, TRUE, 50);
            LogMessage(L"✅ İlerleme çubuğu modu: Marquee");
            break;
            
        case 1: // Full
            ShowWindow(g_hProgressBar, SW_SHOW);
            SetWindowLongPtr(g_hProgressBar, GWL_STYLE, 
                GetWindowLongPtr(g_hProgressBar, GWL_STYLE) & ~PBS_MARQUEE);
            SendMessage(g_hProgressBar, PBM_SETMARQUEE, FALSE, 0);
            SendMessage(g_hProgressBar, PBM_SETPOS, 100, 0);
            LogMessage(L"✅ İlerleme çubuğu modu: Full (100%)");
            break;
            
        case 2: // Hide
            ShowWindow(g_hProgressBar, SW_HIDE);
            LogMessage(L"✅ İlerleme çubuğu gizlendi");
            break;
            
        case 3: // Custom percentage
            {
                ShowWindow(g_hProgressBar, SW_SHOW);
                SetWindowLongPtr(g_hProgressBar, GWL_STYLE, 
                    GetWindowLongPtr(g_hProgressBar, GWL_STYLE) & ~PBS_MARQUEE);
                SendMessage(g_hProgressBar, PBM_SETMARQUEE, FALSE, 0);
                
                // Get custom value from edit box with validation
                wchar_t buf[16];
                GetWindowTextW(g_hEditProgressCustom, buf, 16);
                
                // Validate that it's a number
                bool isValid = true;
                for (int i = 0; buf[i] != L'\0'; i++) {
                    if (!iswdigit(buf[i])) {
                        isValid = false;
                        break;
                    }
                }
                
                int customValue = 50; // Default if invalid
                if (isValid && buf[0] != L'\0') {
                    customValue = _wtoi(buf);
                    if (customValue < 0) customValue = 0;
                    if (customValue > 100) customValue = 100;
                } else {
                    // Invalid input, set to default and update UI
                    SetWindowTextW(g_hEditProgressCustom, L"50");
                }
                
                g_customProgressValue = customValue;
                
                SendMessage(g_hProgressBar, PBM_SETPOS, customValue, 0);
                LogMessage(L"✅ İlerleme çubuğu modu: Custom %" + std::to_wstring(customValue));
            }
            break;
    }
}

// Apply tray icon selection
void ApplyTrayIconSelection() {
    HICON selectedIcon = NULL;
    std::wstring tooltip = L"ShadowCopy";
    
    switch (g_selectedTrayIcon) {
        case 0: // Default - Auto mode
            g_manualTrayIconSelection = false;
            UpdateTrayIcon();  // Let auto-update take over
            return;
        case 1: // No WinRAR
            selectedIcon = g_hIconNoWinRAR ? g_hIconNoWinRAR : g_hIconDefault;
            tooltip = L"ShadowCopy - WinRAR Not Found";
            g_manualTrayIconSelection = true;
            break;
        case 2: // No Internet
            selectedIcon = g_hIconNoInternet ? g_hIconNoInternet : g_hIconDefault;
            tooltip = L"ShadowCopy - No Internet";
            g_manualTrayIconSelection = true;
            break;
        case 3: // Connected
            selectedIcon = g_hIconConnected ? g_hIconConnected : g_hIconDefault;
            tooltip = L"ShadowCopy - Connected";
            g_manualTrayIconSelection = true;
            break;
    }
    
    if (selectedIcon) {
        g_nid.hIcon = selectedIcon;
        wcscpy_s(g_nid.szTip, tooltip.c_str());
        Shell_NotifyIcon(NIM_MODIFY, &g_nid);
        LogMessage(L"✅ Tray icon değiştirildi: " + tooltip);
    }
}

// --- MAIN ---
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInst = hInstance;
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"ShadowCopyInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (wcscmp(argv[i], L"--autostart") == 0) g_isAutoStart = true;
        }
    }
    LocalFree(argv);

    if (FAILED(CoInitialize(NULL))) return FALSE;
    InitCommonControls();
    InitResources();
    LoadSettings();
    ApplyTheme();  // Apply theme based on loaded settings

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = g_hBrushMainBg;
    wcex.lpszClassName = CLASS_NAME;
    
    // Load application icon for titlebar
    HICON hAppIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    if (!hAppIcon) {
        hAppIcon = LoadIcon(hInstance, L"ShadowCopy.ico");
    }
    wcex.hIcon = hAppIcon;
    wcex.hIconSm = hAppIcon;
    RegisterClassExW(&wcex);

    g_hMainWindow = CreateWindowExW(0, CLASS_NAME, L"Shadow Copy",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 900, 650,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hMainWindow) return FALSE;
    
    // Set window icon explicitly
    if (hAppIcon) {
        SendMessage(g_hMainWindow, WM_SETICON, ICON_BIG, (LPARAM)hAppIcon);
        SendMessage(g_hMainWindow, WM_SETICON, ICON_SMALL, (LPARAM)hAppIcon);
    }

    BOOL dark = FALSE;
    DwmSetWindowAttribute(g_hMainWindow, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    CreateUI(g_hMainWindow);
    
    // Set Lonelith URL combo box based on loaded setting
    if (g_lonelithUrl == L"localhost:3000") {
        SendMessage(g_hComboLonelithUrl, CB_SETCURSEL, 0, 0);
    } else if (g_lonelithUrl == L"lonelith.556.space") {
        SendMessage(g_hComboLonelithUrl, CB_SETCURSEL, 1, 0);
    } else {
        // Custom URL
        SendMessage(g_hComboLonelithUrl, CB_SETCURSEL, 2, 0);
        SetWindowTextW(g_hEditCustomUrl, g_lonelithUrl.c_str());
        ShowWindow(g_hEditCustomUrl, SW_SHOW);
    }
    
    SwitchTab(0);
    CreateTrayIcon();
    CheckExistingDrives();

    // Initialize new features
    g_hasWinRAR = CheckWinRARInstalled();
    g_hasInternet = CheckInternetConnection();
    g_lonelithAuthKey = LoadAuthKey();
    
    // Update tray icon based on initial status
    if (g_manualTrayIconSelection) {
        ApplyTrayIconSelection();  // Apply manual selection
    } else {
        UpdateTrayIcon();  // Use auto mode
    }
    
    // Log WinRAR status
    if (!g_hasWinRAR) {
        LogMessage(L"⚠️ WinRAR bulunamadı! Sistem tepsisi kırmızı X simgesi gösteriyor.");
    } else {
        LogMessage(L"✅ WinRAR tespit edildi.");
    }
    
    // Log initial internet status
    if (g_hasInternet) {
        LogMessage(L"✅ İnternet bağlantısı aktif.");
        
        // Run initial health checks
        CheckGitHubConnection();
        CheckLonelithHealth();
        
        // Run speed test on first connection
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            TestInternetSpeed();
        }).detach();
    } else {
        LogMessage(L"⚠️ İnternet bağlantısı yok.");
    }
    
    // Start internet monitoring thread
    std::thread(InternetMonitorThread).detach();

    std::thread(DestructionWatcher).detach();

    if (g_isAutoStart || g_startInTray) {
        ShowWindow(g_hMainWindow, SW_HIDE);
        if (!IsSilentMode()) SendNotification(L"Shadow Copy Aktif", L"Sistem tepsisinde çalışıyor. 🛡");
    }
    else {
        if (ShowLoginDialog()) {
            ShowWindow(g_hMainWindow, SW_SHOW);
            UpdateWindow(g_hMainWindow);
            SetForegroundWindow(g_hMainWindow);
        }
        else {
            RemoveTrayIcon();
            return 0;
        }
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveTrayIcon();
    CleanupResources();
    CoUninitialize();
    if (hMutex) ReleaseMutex(hMutex);
    return (int)msg.wParam;
}

// --- KAYNAK YÖNETİMİ ---
void InitResources()
{
    // Use modern fonts with Windows fallback handling
    // CreateFontW will automatically substitute if font not available
    g_hFontTitle = CreateFontW(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI Variable Display");
    g_hFontSubtitle = CreateFontW(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI Variable Text");
    g_hFontNormal = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI Variable Text");
    g_hFontSmall = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI Variable Text");
    g_hFontMono = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Cascadia Code");

    // CreateFontW should never return NULL, but provide fallback to system font just in case
    if (!g_hFontTitle) g_hFontTitle = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_hFontSubtitle) g_hFontSubtitle = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_hFontNormal) g_hFontNormal = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_hFontSmall) g_hFontSmall = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_hFontMono) g_hFontMono = (HFONT)GetStockObject(ANSI_FIXED_FONT);

    g_hBrushMainBg = CreateSolidBrush(CLR_BG_MAIN);
    g_hBrushSidebar = CreateSolidBrush(CLR_BG_SIDEBAR);
    g_hBrushEdit = CreateSolidBrush(CLR_INPUT_BG);
    
    // Check if brush creation failed
    if (!g_hBrushMainBg || !g_hBrushSidebar || !g_hBrushEdit) {
        // Clean up any that succeeded
        if (g_hBrushMainBg) DeleteObject(g_hBrushMainBg);
        if (g_hBrushSidebar) DeleteObject(g_hBrushSidebar);
        if (g_hBrushEdit) DeleteObject(g_hBrushEdit);
        
        // Use stock objects
        g_hBrushMainBg = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        g_hBrushSidebar = (HBRUSH)GetStockObject(GRAY_BRUSH);
        g_hBrushEdit = (HBRUSH)GetStockObject(WHITE_BRUSH);
        g_brushesAreStock = true;
    } else {
        g_brushesAreStock = false;
    }
    
    // Load status icons
    g_hIconNoWinRAR = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_TRAY_NO_WINRAR));
    g_hIconNoInternet = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_TRAY_NO_INTERNET));
    g_hIconConnected = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_TRAY_CONNECTED));
    g_hIconDefault = LoadIcon(g_hInst, IDI_APPLICATION);
    
    // Fallback to default icon if custom icons not loaded
    if (!g_hIconNoWinRAR) g_hIconNoWinRAR = g_hIconDefault;
    if (!g_hIconNoInternet) g_hIconNoInternet = g_hIconDefault;
    if (!g_hIconConnected) g_hIconConnected = g_hIconDefault;
}

void CleanupResources()
{
    // Only delete fonts if they're not stock objects
    HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    
    if (g_hFontTitle && g_hFontTitle != hDefaultFont) DeleteObject(g_hFontTitle);
    if (g_hFontSubtitle && g_hFontSubtitle != hDefaultFont) DeleteObject(g_hFontSubtitle);
    if (g_hFontNormal && g_hFontNormal != hDefaultFont) DeleteObject(g_hFontNormal);
    if (g_hFontSmall && g_hFontSmall != hDefaultFont) DeleteObject(g_hFontSmall);
    if (g_hFontMono && g_hFontMono != hFixedFont && g_hFontMono != hDefaultFont) DeleteObject(g_hFontMono);
    
    // Only delete brushes if they're not stock objects
    if (!g_brushesAreStock) {
        if (g_hBrushMainBg) DeleteObject(g_hBrushMainBg);
        if (g_hBrushSidebar) DeleteObject(g_hBrushSidebar);
        if (g_hBrushEdit) DeleteObject(g_hBrushEdit);
    }
    
    // Clean up icons
    if (g_hIconNoWinRAR && g_hIconNoWinRAR != g_hIconDefault) DestroyIcon(g_hIconNoWinRAR);
    if (g_hIconNoInternet && g_hIconNoInternet != g_hIconDefault) DestroyIcon(g_hIconNoInternet);
    if (g_hIconConnected && g_hIconConnected != g_hIconDefault) DestroyIcon(g_hIconConnected);
    if (g_hIconDefault) DestroyIcon(g_hIconDefault);
}

void SetModernStyle(HWND hControl) {
    SendMessage(hControl, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SetWindowTheme(hControl, L"Explorer", NULL);
}

void StyleTextBox(HWND hEdit, bool isMultiline) {
    // Apply modern font
    SendMessage(hEdit, WM_SETFONT, (WPARAM)(isMultiline ? g_hFontSmall : g_hFontNormal), TRUE);
    
    // Use modern theme
    SetWindowTheme(hEdit, L"Explorer", NULL);
    
    // Add some padding (via EM_SETMARGINS for single-line edit controls)
    if (!isMultiline) {
        SendMessage(hEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(5, 5));
    }
    // Note: This function is available for future use. Currently, textboxes are styled
    // manually in CreateUI for consistency with existing code patterns.
}

// --- UI OLUŞTURMA ---
void CreateUI(HWND hWnd)
{
    // Create top navigation bar (horizontal) - New order: Home, Lonelith, Settings, SysInfo, Customization
    int btnW = 140; int btnH = 40; int btnX = 20;
    g_hNavBtnHome = CreateWindowW(L"BUTTON", L"🏠 Ana Sayfa", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, btnX, 10, btnW, btnH, hWnd, (HMENU)IDB_NAV_HOME, g_hInst, NULL);
    btnX += btnW + 10;
    g_hNavBtnLonelith = CreateWindowW(L"BUTTON", L"☁️ Lonelith", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, btnX, 10, btnW, btnH, hWnd, (HMENU)IDB_NAV_LONELITH, g_hInst, NULL);
    btnX += btnW + 10;
    g_hNavBtnSettings = CreateWindowW(L"BUTTON", L"⚙ Ayarlar", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, btnX, 10, btnW, btnH, hWnd, (HMENU)IDB_NAV_SETTINGS, g_hInst, NULL);
    btnX += btnW + 10;
    g_hNavBtnInfo = CreateWindowW(L"BUTTON", L"ℹ️ Sistem Bilgisi", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, btnX, 10, btnW, btnH, hWnd, (HMENU)IDB_NAV_INFO, g_hInst, NULL);
    btnX += btnW + 10;
    g_hNavBtnCustomization = CreateWindowW(L"BUTTON", L"🎨 Tema", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, btnX, 10, btnW, btnH, hWnd, (HMENU)IDB_NAV_CUSTOMIZATION, g_hInst, NULL);
    
    // Create progress bar in footer
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int footerY = clientRect.bottom - FOOTER_HEIGHT;
    g_hProgressBar = CreateWindowW(PROGRESS_CLASSW, NULL, 
        WS_CHILD | WS_VISIBLE | PBS_MARQUEE, 
        0, footerY, clientRect.right, PROGRESS_BAR_HEIGHT, 
        hWnd, (HMENU)IDC_PROGRESS_BAR, g_hInst, NULL);
    SendMessage(g_hProgressBar, PBM_SETMARQUEE, TRUE, 50);

    // TAB 0: HOME
    CreateLabel(0, hWnd, L"Shadow Copy", 40, 10, 400, 40, g_hFontTitle);
    CreateLabel(0, hWnd, L"USB algılandığında şifreli yedekleme başlatılır.", 40, 55, 550, 25, g_hFontNormal);

    CreateLabel(0, hWnd, L"📝 İşlem Günlüğü:", 40, 100, 200, 25, g_hFontSubtitle);
    g_hStatusText = CreateCtrl(0, L"EDIT", L"Sistem hazır. USB bekleniyor...\r\n", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY | WS_BORDER, 40, 130, 560, 280, hWnd, NULL);
    SendMessage(g_hStatusText, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

    HWND hBtnClearLog = CreateCtrl(0, L"BUTTON", L"🗑️ Günlüğü Temizle", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 620, 130, 160, 35, hWnd, (HMENU)IDB_CLEAR_LOG);
    SetModernStyle(hBtnClearLog);
    
    // TODO Features section
    CreateLabel(0, hWnd, L"📋 Özellik Durumu:", 40, 425, 200, 25, g_hFontSubtitle);
    HWND hTodoText = CreateCtrl(0, L"EDIT", 
        L"✅ Lonelith API temel entegrasyonu\r\n"
        L"✅ Dosya yükleme özelliği\r\n"
        L"✅ Dosya indirme özelliği\r\n"
        L"✅ Bulut dosya listesi görüntüleme\r\n"
        L"✅ Tray ikon özelleştirme\r\n"
        L"✅ Gelişmiş ilerleme çubuğu seçenekleri\r\n"
        L"✅ Otomatik internet bağlantı kontrolü\r\n"
        L"✅ Hız testi (indirme ve yükleme)\r\n"
        L"🔄 Tam Lonelith API entegrasyonu (devam ediyor)\r\n"
        L"🔄 Gelişmiş dosya yönetimi (planlı)\r\n",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_BORDER, 40, 455, 740, 65, hWnd, NULL);
    SendMessage(hTodoText, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

    // TAB 1: LONELITH
    CreateLabel(1, hWnd, L"Lonelith Bulut Yönetimi", 40, 10, 400, 40, g_hFontTitle);
    
    CreateLabel(1, hWnd, L"Server URL:", 40, 60, 150, 20, g_hFontNormal);
    g_hComboLonelithUrl = CreateCtrl(1, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 40, 85, 350, 200, hWnd, (HMENU)IDC_COMBO_LONELITH_URL);
    SendMessage(g_hComboLonelithUrl, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessage(g_hComboLonelithUrl, CB_ADDSTRING, 0, (LPARAM)L"localhost:3000");
    SendMessage(g_hComboLonelithUrl, CB_ADDSTRING, 0, (LPARAM)L"lonelith.556.space");
    SendMessage(g_hComboLonelithUrl, CB_ADDSTRING, 0, (LPARAM)L"▼ Başka bir URL gir...");
    SendMessage(g_hComboLonelithUrl, CB_SETCURSEL, 0, 0);
    
    g_hEditCustomUrl = CreateCtrl(1, L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 40, 115, 350, 30, hWnd, (HMENU)IDC_EDIT_CUSTOM_URL);
    SendMessage(g_hEditCustomUrl, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    ShowWindow(g_hEditCustomUrl, SW_HIDE);
    
    CreateLabel(1, hWnd, L"Auth Key:", 40, 155, 150, 20, g_hFontNormal);
    g_hEditAuthKey = CreateCtrl(1, L"EDIT", g_lonelithAuthKey.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD, 40, 180, 350, 30, hWnd, (HMENU)IDC_EDIT_AUTH_KEY);
    SendMessage(g_hEditAuthKey, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    
    HWND hBtnSaveKey = CreateCtrl(1, L"BUTTON", L"💾 Kaydet", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 400, 180, 100, 30, hWnd, (HMENU)IDB_SAVE_AUTH_KEY);
    SetModernStyle(hBtnSaveKey);
    
    g_hCheckAutoUpload = CreateCtrl(1, L"BUTTON", L"İnternet bağlantısı olduğunda otomatik yükle", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 220, 400, 30, hWnd, (HMENU)IDC_CHECK_AUTO_UPLOAD);
    SetModernStyle(g_hCheckAutoUpload);
    if (g_autoUpload) SendMessage(g_hCheckAutoUpload, BM_SETCHECK, BST_CHECKED, 0);
    
    CreateLabel(1, hWnd, L"Bağlantı Durumu:", 40, 260, 200, 25, g_hFontSubtitle);
    
    CreateLabel(1, hWnd, L"GitHub:", 40, 290, 100, 20, g_hFontNormal);
    g_hGitHubTestResult = CreateCtrl(1, L"STATIC", g_githubConnHealth.c_str(), WS_CHILD | WS_VISIBLE, 150, 290, 200, 20, hWnd, NULL);
    SendMessage(g_hGitHubTestResult, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    
    HWND hBtnTestGitHub = CreateCtrl(1, L"BUTTON", L"🔍 Test Et", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 360, 287, 100, 25, hWnd, (HMENU)IDB_TEST_GITHUB);
    SetModernStyle(hBtnTestGitHub);
    
    CreateLabel(1, hWnd, L"Lonelith Server:", 40, 315, 100, 20, g_hFontNormal);
    HWND hLonelithStatus = CreateCtrl(1, L"STATIC", g_lonelithServerHealth.c_str(), WS_CHILD | WS_VISIBLE, 150, 315, 300, 20, hWnd, NULL);
    SendMessage(hLonelithStatus, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    
    CreateLabel(1, hWnd, L"İndirme Hızı:", 40, 340, 100, 20, g_hFontNormal);
    g_hSpeedTestResult = CreateCtrl(1, L"STATIC", g_currentSpeed.c_str(), WS_CHILD | WS_VISIBLE, 150, 340, 200, 20, hWnd, NULL);
    SendMessage(g_hSpeedTestResult, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    
    CreateLabel(1, hWnd, L"Yükleme Hızı:", 40, 365, 100, 20, g_hFontNormal);
    g_hUploadSpeedTestResult = CreateCtrl(1, L"STATIC", g_currentUploadSpeed.c_str(), WS_CHILD | WS_VISIBLE, 150, 365, 200, 20, hWnd, NULL);
    SendMessage(g_hUploadSpeedTestResult, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    
    HWND hBtnSpeedTest = CreateCtrl(1, L"BUTTON", L"🚀 Hız Testi Yap", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 360, 347, 140, 25, hWnd, (HMENU)IDB_SPEED_TEST);
    SetModernStyle(hBtnSpeedTest);
    
    CreateLabel(1, hWnd, L"Yüklü Dosyalar:", 40, 400, 200, 25, g_hFontSubtitle);
    g_hLonelithFileList = CreateCtrl(1, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 40, 430, 500, 90, hWnd, (HMENU)IDC_LONELITH_FILE_LIST);
    SendMessage(g_hLonelithFileList, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
    
    HWND hBtnRefresh = CreateCtrl(1, L"BUTTON", L"🔄 Yenile", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 550, 430, 100, 30, hWnd, (HMENU)IDB_LONELITH_REFRESH);
    SetModernStyle(hBtnRefresh);
    
    HWND hBtnUpload = CreateCtrl(1, L"BUTTON", L"⬆️ Manuel Yükle", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 550, 470, 120, 30, hWnd, (HMENU)IDB_LONELITH_UPLOAD);
    SetModernStyle(hBtnUpload);

    // TAB 2: SETTINGS
    CreateLabel(2, hWnd, L"Ayarlar", 40, 10, 200, 40, g_hFontTitle);
    
    CreateLabel(2, hWnd, L"📂 Hedef Klasör:", 40, 60, 200, 25, g_hFontSubtitle);
    g_hPathDisplay = CreateCtrl(2, L"EDIT", g_targetPath.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY | WS_BORDER, 40, 90, 450, 30, hWnd, (HMENU)IDC_EDIT_PATH);
    SendMessage(g_hPathDisplay, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND hBtnChange = CreateCtrl(2, L"BUTTON", L"Değiştir", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 500, 90, 100, 30, hWnd, (HMENU)IDB_SELECT_FOLDER);
    SetModernStyle(hBtnChange);
    
    CreateLabel(2, hWnd, L"Başlangıç Seçenekleri", 40, 140, 300, 25, g_hFontSubtitle);
    g_hCheckStartup = CreateCtrl(2, L"BUTTON", L"Windows ile başlat", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 170, 300, 30, hWnd, (HMENU)IDC_CHECK_STARTUP);
    SetModernStyle(g_hCheckStartup);
    if (g_startWithWindows) SendMessage(g_hCheckStartup, BM_SETCHECK, BST_CHECKED, 0);

    g_hCheckStartTray = CreateCtrl(2, L"BUTTON", L"Program açılışta gizli başlasın (Tepsi)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 200, 350, 30, hWnd, (HMENU)IDC_CHECK_START_TRAY);
    SetModernStyle(g_hCheckStartTray);
    if (g_startInTray) SendMessage(g_hCheckStartTray, BM_SETCHECK, BST_CHECKED, 0);

    g_hCheckSilent = CreateCtrl(2, L"BUTTON", L"Sessiz Mod (Bildirim gönderme)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 230, 300, 30, hWnd, (HMENU)IDC_CHECK_SILENT);
    SetModernStyle(g_hCheckSilent);
    if (IsSilentMode()) SendMessage(g_hCheckSilent, BM_SETCHECK, BST_CHECKED, 0);

    g_hCheckGoodbye = CreateCtrl(2, L"BUTTON", L"Kendini imha ederken 'elveda.txt' bırak", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 260, 400, 30, hWnd, (HMENU)IDC_CHECK_GOODBYE);
    SetModernStyle(g_hCheckGoodbye);
    if (g_leaveGoodbyeNote) SendMessage(g_hCheckGoodbye, BM_SETCHECK, BST_CHECKED, 0);

    int secX = 350;
    CreateLabel(2, hWnd, L"Güvenlik", 40 + secX, 140, 200, 25, g_hFontSubtitle);
    CreateLabel(2, hWnd, L"Erişim Parolası:", 40 + secX, 170, 150, 20, g_hFontNormal);
    g_hEditPassword = CreateCtrl(2, L"EDIT", g_appPassword.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 40 + secX, 195, 180, 30, hWnd, (HMENU)IDC_EDIT_PASSWORD);
    SendMessage(g_hEditPassword, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    CreateLabel(2, hWnd, L"(Hatalı girişte imha tetiklenir)", 40 + secX, 230, 220, 20, g_hFontSmall);

    CreateLabel(2, hWnd, L"Varsayılan Yedekleme Yolu:", 40, 310, 300, 25, g_hFontSubtitle);
    g_hEditDefaultPath = CreateCtrl(2, L"EDIT", g_targetPath.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY | WS_BORDER, 40, 340, 450, 30, hWnd, (HMENU)IDC_EDIT_PATH);
    SendMessage(g_hEditDefaultPath, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND hBtnSave = CreateCtrl(2, L"BUTTON", L"💾  Ayarları Kaydet", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 390, 180, 40, hWnd, (HMENU)IDB_SAVE_SETTINGS);
    SetModernStyle(hBtnSave);

    CreateLabel(2, hWnd, L"Tehlikeli Bölge", 40, 450, 200, 25, g_hFontSubtitle);
    HWND hBtnReset = CreateCtrl(2, L"BUTTON", L"⚠️ Uygulamayı Sıfırla (Temizle)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 480, 250, 35, hWnd, (HMENU)IDB_RESET_APP);
    SendMessage(hBtnReset, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    HWND hBtnUninstall = CreateCtrl(2, L"BUTTON", L"☢️ KALDIR VE YOK ET (UNINSTALL)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 310, 480, 280, 35, hWnd, (HMENU)IDB_UNINSTALL_APP);
    SendMessage(hBtnUninstall, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    // TAB 3: SYSINFO
    CreateLabel(3, hWnd, L"Sistem Bilgisi", 40, 10, 300, 40, g_hFontTitle);
    g_hInfoText = CreateCtrl(3, L"EDIT", L"Yükleniyor...", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY | WS_BORDER, 40, 70, 800, 450, hWnd, NULL);
    SendMessage(g_hInfoText, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
    
    // TAB 4: CUSTOMIZATION
    CreateLabel(4, hWnd, L"Özelleştirme", 40, 10, 300, 40, g_hFontTitle);
    
    // Theme Selection Section
    CreateLabel(4, hWnd, L"Tema Seçimi", 40, 70, 300, 25, g_hFontSubtitle);
    CreateLabel(4, hWnd, L"Aydınlık/Karanlık Tema:", 40, 100, 200, 20, g_hFontNormal);
    
    g_hThemeToggleBtn = CreateCtrl(4, L"BUTTON", g_isDarkMode ? L"☀️ Aydınlık Moda Geç" : L"🌙 Karanlık Moda Geç", 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 125, 200, 35, hWnd, (HMENU)IDB_TOGGLE_THEME);
    SendMessage(g_hThemeToggleBtn, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SetModernStyle(g_hThemeToggleBtn);
    
    CreateLabel(4, hWnd, L"Not: Tema tercihiniz otomatik olarak kaydedilir.", 40, 170, 400, 20, g_hFontSmall);
    
    // Progress Bar Behavior Section
    CreateLabel(4, hWnd, L"İlerleme Çubuğu Davranışı", 40, 210, 300, 25, g_hFontSubtitle);
    CreateLabel(4, hWnd, L"Boşta kalma durumu:", 40, 240, 150, 20, g_hFontNormal);
    
    g_hComboProgressMode = CreateCtrl(4, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 40, 265, 250, 200, hWnd, (HMENU)IDC_COMBO_PROGRESS_MODE);
    SendMessage(g_hComboProgressMode, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessage(g_hComboProgressMode, CB_ADDSTRING, 0, (LPARAM)L"Marquee (Animasyonlu)");
    SendMessage(g_hComboProgressMode, CB_ADDSTRING, 0, (LPARAM)L"Full (100%)");
    SendMessage(g_hComboProgressMode, CB_ADDSTRING, 0, (LPARAM)L"Hide (Gizli)");
    SendMessage(g_hComboProgressMode, CB_ADDSTRING, 0, (LPARAM)L"Custom (Özel Yüzde)");
    SendMessage(g_hComboProgressMode, CB_SETCURSEL, g_progressBarMode, 0);
    
    CreateLabel(4, hWnd, L"Özel yüzde değeri:", 40, 300, 150, 20, g_hFontNormal);
    g_hEditProgressCustom = CreateCtrl(4, L"EDIT", std::to_wstring(g_customProgressValue).c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 200, 297, 90, 25, hWnd, (HMENU)IDC_EDIT_PROGRESS_CUSTOM);
    SendMessage(g_hEditProgressCustom, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    EnableWindow(g_hEditProgressCustom, g_progressBarMode == 3);
    
    CreateLabel(4, hWnd, L"(Sadece Custom seçiliyken aktif)", 40, 330, 250, 20, g_hFontSmall);
    
    // Tray Icon Selection Section
    CreateLabel(4, hWnd, L"Tepsi İkonu Seçimi", 40, 370, 300, 25, g_hFontSubtitle);
    CreateLabel(4, hWnd, L"Aktif ikon:", 40, 400, 150, 20, g_hFontNormal);
    
    g_hComboTrayIcon = CreateCtrl(4, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 40, 425, 250, 200, hWnd, (HMENU)IDC_COMBO_TRAY_ICON);
    SendMessage(g_hComboTrayIcon, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessage(g_hComboTrayIcon, CB_ADDSTRING, 0, (LPARAM)L"Varsayılan");
    SendMessage(g_hComboTrayIcon, CB_ADDSTRING, 0, (LPARAM)L"WinRAR Bulunamadı (❌)");
    SendMessage(g_hComboTrayIcon, CB_ADDSTRING, 0, (LPARAM)L"İnternet Yok (⚠️)");
    SendMessage(g_hComboTrayIcon, CB_ADDSTRING, 0, (LPARAM)L"Bağlantılı (✅)");
    SendMessage(g_hComboTrayIcon, CB_SETCURSEL, g_selectedTrayIcon, 0);
    
    HWND hBtnApplyTrayIcon = CreateCtrl(4, L"BUTTON", L"🎨 Uygula", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 300, 425, 100, 30, hWnd, (HMENU)IDB_APPLY_TRAY_ICON);
    SetModernStyle(hBtnApplyTrayIcon);
    
    CreateLabel(4, hWnd, L"Not: Tray ikonları otomatik olarak sistem durumuna göre değişir.\nBu ayar manuel seçim yapmak içindir.", 40, 470, 600, 40, g_hFontSmall);
}


// ... YARDIMCI FONKSİYONLAR ...
bool IsSilentMode() {
    HKEY hKey;
    DWORD val = 1;
    DWORD size = sizeof(val);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"SilentMode", NULL, NULL, (BYTE*)&val, &size) != ERROR_SUCCESS) val = 1;
        RegCloseKey(hKey);
    }
    return (val != 0);
}

// Sürücünün fiziksel bağlantı tipini (USB mi?) kontrol eder
bool IsUsbDevice(std::wstring driveRoot) {
    // Sürücü yolunu "\\.\X:" formatına çevir (Windows Kernel formatı)
    std::wstring driveLetter = driveRoot.substr(0, 2); // "E:"
    std::wstring handlePath = L"\\\\.\\" + driveLetter;

    HANDLE hDevice = CreateFileW(handlePath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) return false;

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    STORAGE_DEVICE_DESCRIPTOR pDevDesc = {};
    DWORD dwBytesReturned = 0;

    // Sürücüden donanım bilgisini iste
    if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
        &query, sizeof(query),
        &pDevDesc, sizeof(pDevDesc),
        &dwBytesReturned, NULL)) {

        CloseHandle(hDevice);

        // BusType 7 = BusTypeUsb (Winioctl.h içinde tanımlıdır)
        if (pDevDesc.BusType == BusTypeUsb) {
            return true;
        }
    }
    else {
        CloseHandle(hDevice);
    }

    return false;
}

void SendNotification(const std::wstring& title, const std::wstring& msg) {
    if (IsSilentMode()) {
        LogMessage(L"🔕 (Sessiz Mod) Bildirim: " + title);
        return;
    }
    g_nid.uFlags = NIF_INFO;
    wcscpy_s(g_nid.szInfoTitle, title.c_str());
    wcscpy_s(g_nid.szInfo, msg.c_str());
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

std::wstring GetRegString(HKEY hKeyRoot, LPCWSTR subKey, LPCWSTR valueName) {
    HKEY hKey;
    wchar_t buffer[256] = L"Bilinmiyor";
    DWORD bufferSize = sizeof(buffer);
    if (RegOpenKeyExW(hKeyRoot, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, valueName, NULL, NULL, (BYTE*)buffer, &bufferSize);
        RegCloseKey(hKey);
    }
    return std::wstring(buffer);
}

std::wstring GetSystemInfo() {
    std::wstringstream ss;
    ss << L"=== SİSTEM ÖZETİ ===\r\n\r\n";
    SYSTEM_INFO si; GetNativeSystemInfo(&si);
    MEMORYSTATUSEX statex; statex.dwLength = sizeof(statex); GlobalMemoryStatusEx(&statex);

    // CPU Information
    ss << L" [CPU]\r\n";
    ss << L"   Çekirdek Sayısı: " << si.dwNumberOfProcessors << L"\r\n";
    ss << L"   Mimari: " << (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? L"x64 (64-bit)" : L"x86 (32-bit)") << L"\r\n";
    ss << L"   Sayfa Boyutu: " << si.dwPageSize << L" bytes\r\n";
    
    // RAM Information
    ss << L"\r\n [RAM]\r\n";
    ss << L"   Toplam Fiziksel RAM: " << FormatBytes(statex.ullTotalPhys) << L"\r\n";
    ss << L"   Kullanılabilir RAM: " << FormatBytes(statex.ullAvailPhys) << L"\r\n";
    ss << L"   RAM Kullanım: %" << statex.dwMemoryLoad << L"\r\n";
    ss << L"   Toplam Sanal Bellek: " << FormatBytes(statex.ullTotalVirtual) << L"\r\n";
    ss << L"   Boş Sanal Bellek: " << FormatBytes(statex.ullAvailVirtual) << L"\r\n";

    // Motherboard Information
    std::wstring moboBrand = GetRegString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardManufacturer");
    std::wstring moboModel = GetRegString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct");
    std::wstring biosVersion = GetRegString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVersion");
    ss << L"\r\n [ANAKART]\r\n";
    ss << L"   Üretici: " << moboBrand << L"\r\n";
    ss << L"   Model: " << moboModel << L"\r\n";
    ss << L"   BIOS Sürüm: " << biosVersion << L"\r\n";

    // GPU Information
    DISPLAY_DEVICEW dd; dd.cb = sizeof(dd);
    ss << L"\r\n [GPU]\r\n";
    if (EnumDisplayDevicesW(NULL, 0, &dd, 0)) {
        ss << L"   " << dd.DeviceString << L"\r\n";
    } else {
        ss << L"   Algılanamadı\r\n";
    }
    
    // OS Information
    ss << L"\r\n [İŞLETİM SİSTEMİ]\r\n";
    std::wstring osName = GetRegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName");
    std::wstring osBuild = GetRegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuild");
    std::wstring osVersion = GetRegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DisplayVersion");
    ss << L"   " << osName << L"\r\n";
    ss << L"   Sürüm: " << osVersion << L" (Build " << osBuild << L")\r\n";
    
    // WinRAR Information
    ss << L"\r\n [YÜKLENMIŞ YAZILIMLAR]\r\n";
    ss << L"   WinRAR: " << (g_hasWinRAR ? g_winrarPath : L"Yüklü Değil") << L"\r\n";
    
    // Disk Drives
    ss << L"\r\n=== DİSK SÜRÜCÜLERİ ===\r\n";
    DWORD dwSize = GetLogicalDriveStringsW(0, NULL);
    std::vector<wchar_t> buffer(dwSize + 1);
    GetLogicalDriveStringsW(dwSize, buffer.data());
    wchar_t* pDrive = buffer.data();
    while (*pDrive) {
        UINT type = GetDriveTypeW(pDrive);
        std::wstring typeStr = L"Bilinmiyor";
        if (type == DRIVE_FIXED) typeStr = L"Sabit Disk";
        else if (type == DRIVE_REMOVABLE) typeStr = L"USB/Çıkarılabilir";
        else if (type == DRIVE_CDROM) typeStr = L"CD/DVD-ROM";
        else if (type == DRIVE_REMOTE) typeStr = L"Ağ Sürücüsü";

        ULARGE_INTEGER freeBytes, totalBytes, totalFree;
        if (GetDiskFreeSpaceExW(pDrive, &freeBytes, &totalBytes, &totalFree)) {
            ss << L" " << pDrive << L"  [" << typeStr << L"]\r\n";
            ss << L"        Toplam: " << FormatBytes(totalBytes.QuadPart)
                << L" | Boş: " << FormatBytes(totalFree.QuadPart) << L"\r\n";
        }
        pDrive += wcslen(pDrive) + 1;
    }
    
    // Network Status
    ss << L"\r\n=== AĞ DURUMU ===\r\n";
    ss << L" İnternet Bağlantısı: " << (g_hasInternet ? L"✅ Aktif" : L"❌ Yok") << L"\r\n";
    ss << L" GitHub Erişimi: " << g_githubConnHealth << L"\r\n";
    ss << L" Lonelith Server: " << g_lonelithServerHealth << L"\r\n";
    if (!g_currentSpeed.empty()) {
        ss << L" İnternet Hızı: " << g_currentSpeed << L"\r\n";
    }
    
    ss << L"\r\n=== UYGULAMA BİLGİSİ ===\r\n";
    ss << L" Sürüm: v3.0 (RAR Encrypted)\r\n";
    ss << L" Durum: Korumalı\r\n";
    ss << L" Hedef Klasör: " << g_targetPath << L"\r\n";
    
    return ss.str();
}

void CheckExistingDrives() {
    LogMessage(L"🚀 Sistem taraması: Sürücüler ve veri yolları analiz ediliyor...");

    DWORD dwSize = GetLogicalDriveStringsW(0, NULL);
    if (dwSize == 0) return;

    std::vector<wchar_t> buffer(dwSize + 1);
    if (GetLogicalDriveStringsW(dwSize, buffer.data()) == 0) return;

    // Sistem sürücüsünü (Genellikle C:) bul ve hariç tut
    wchar_t sysPath[MAX_PATH];
    GetSystemDirectoryW(sysPath, MAX_PATH);
    std::wstring systemDrive = std::wstring(sysPath).substr(0, 3); // "C:\"

    wchar_t* pDrive = buffer.data();
    int activeUsbCount = 0;

    while (*pDrive) {
        std::wstring drivePath = pDrive;
        UINT type = GetDriveTypeW(pDrive);

        // Kural 1: Sistem sürücüsü asla yedeklenmez.
        if (drivePath != systemDrive) {

            bool isTarget = false;

            // Durum A: Klasik Çıkarılabilir Sürücü (Flash Bellekler)
            if (type == DRIVE_REMOVABLE) {
                isTarget = true;
            }
            // Durum B: Sabit Disk olarak görünen USB'ler (Harici HDD/SSD)
            else if (type == DRIVE_FIXED) {
                // Burada "IsUsbDevice" ile donanım sorgusu yapıyoruz
                if (IsUsbDevice(drivePath)) {
                    isTarget = true;
                }
            }

            if (isTarget) {
                // Medya kontrolü (İçi boş kart okuyucu mu?)
                ULARGE_INTEGER freeBytes, totalBytes, totalFree;
                if (GetDiskFreeSpaceExW(drivePath.c_str(), &freeBytes, &totalBytes, &totalFree)) {

                    LogMessage(L"✅ Hedef Sürücü Algılandı: " + drivePath + L" (USB/Harici)");

                    std::thread([drivePath]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        SmartBackupStarter(drivePath);
                        }).detach();

                    activeUsbCount++;
                }
            }
        }

        pDrive += wcslen(pDrive) + 1;
    }

    if (activeUsbCount == 0) {
        LogMessage(L"ℹ️ Harici sürücü bulunamadı. Bekleme moduna geçildi.");
    }
}
void SwitchTab(int index)
{
    SetupPageTransition(g_currentTab, index);
    g_currentTab = index;
    for (int i = 0; i < TAB_COUNT; i++) for (HWND hCtrl : g_tabControls[i]) ShowWindow(hCtrl, SW_HIDE);
    for (HWND hCtrl : g_tabControls[index]) ShowWindow(hCtrl, SW_SHOW);
    
    // Tab 1 is now Lonelith
    if (index == 1) {
        // Clear the list first and show a loading message
        SendMessage(g_hLonelithFileList, LB_RESETCONTENT, 0, 0);
        SendMessage(g_hLonelithFileList, LB_ADDSTRING, 0, (LPARAM)L"📥 Yükleniyor...");
        
        // Refresh Lonelith file list asynchronously when switching to Lonelith tab
        std::thread([]() {
            std::vector<std::wstring> files = GetFilesFromLonelith();
            
            // Thread-safe update of cached files
            {
                std::lock_guard<std::mutex> lock(g_lonelithFilesMutex);
                g_cachedLonelithFiles = files;
            }
            
            // Update UI in main thread only if window is still alive
            if (g_isWindowAlive && g_hMainWindow) {
                PostMessage(g_hMainWindow, WM_UPDATE_LONELITH_FILES, 0, 0);
            }
        }).detach();
    }
    
    // Tab 3 is now SysInfo
    if (index == 3) {
        SetWindowTextW(g_hInfoText, GetSystemInfo().c_str());
    }
    
    // Tab 4 is Customization - load current settings
    if (index == 4) {
        SendMessage(g_hComboProgressMode, CB_SETCURSEL, g_progressBarMode, 0);
        SendMessage(g_hComboTrayIcon, CB_SETCURSEL, g_selectedTrayIcon, 0);
        SetWindowTextW(g_hEditProgressCustom, std::to_wstring(g_customProgressValue).c_str());
        EnableWindow(g_hEditProgressCustom, g_progressBarMode == 3);
    }
    
    InvalidateRect(g_hNavBtnHome, NULL, FALSE);
    InvalidateRect(g_hNavBtnLonelith, NULL, FALSE);
    InvalidateRect(g_hNavBtnSettings, NULL, FALSE);
    InvalidateRect(g_hNavBtnInfo, NULL, FALSE);
    InvalidateRect(g_hNavBtnCustomization, NULL, FALSE);
    RECT rc = { 0, NAVBAR_HEIGHT, 900, 650 };
    InvalidateRect(g_hMainWindow, &rc, TRUE);
}

void ResetApp(HWND hWnd) {
    if (MessageBoxW(hWnd, L"Ayarlar sıfırlanacak. Program silinmeyecek.\nDevam?", L"Ayarları Sıfırla", MB_YESNO | MB_ICONWARNING) == IDYES)
    {
        StartupManager::RemoveFromStartup();
        SHDeleteKeyW(HKEY_CURRENT_USER, REG_SUBKEY);
        g_startWithWindows = false;
        g_startInTray = false;
        g_leaveGoodbyeNote = false;
        g_targetPath = GetDefaultPath();
        g_appPassword = L"145366";
        SendMessage(g_hCheckStartup, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(g_hCheckStartTray, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(g_hCheckSilent, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(g_hCheckGoodbye, BM_SETCHECK, BST_UNCHECKED, 0);
        SetWindowTextW(g_hEditDefaultPath, g_targetPath.c_str());
        SetWindowTextW(g_hEditPassword, g_appPassword.c_str());
        MessageBoxW(hWnd, L"Ayarlar varsayılana döndürüldü.", L"Başarılı", MB_OK);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        // --- 1. SİSTEM KAPATMA / YENİDEN BAŞLATMA YÖNETİMİ ---
        // Bu blok, Windows kapanırken "Bu program kapanmayı engelliyor" hatasını önler.
    case WM_QUERYENDSESSION:
    {
        // Windows'a "Kapanmaya hazırım" yanıtı veriyoruz.
        return TRUE;
    }

    case WM_ENDSESSION:
    {
        // Eğer kapanma kesinleştiyse (wParam == TRUE)
        if (wParam == TRUE)
        {
            // Tepsi ikonunu temizle
            RemoveTrayIcon();

            // Döngüleri beklemeden süreci anında sonlandır.
            // Bu sayede Windows bekleme yapmaz.
            ExitProcess(0);
        }
        return 0;
    }

    // --- 2. PENCERE KAPATMA (X BUTONU) ---
    case WM_CLOSE:
        // Programı kapatmak yerine gizle (Tepsiye küçült)
        ShowWindow(hWnd, SW_HIDE);
        if (!IsSilentMode()) SendNotification(L"Shadow Copy Gizlendi", L"Uygulama arka planda ve kilitlendi.");
        return 0;

        // --- 3. GÖRSEL ARAYÜZ ÇİZİMİ (WM_PAINT) ---
    case WM_PAINT:
    {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT rcClient; GetClientRect(hWnd, &rcClient);

        // Top Navigation Bar Background
        RECT rcNavbar = rcClient; rcNavbar.bottom = NAVBAR_HEIGHT;
        FillRect(hdc, &rcNavbar, g_hBrushSidebar);

        // İçerik Alanı Arkaplanı
        RECT rcContent = rcClient; rcContent.top = NAVBAR_HEIGHT; rcContent.bottom -= FOOTER_HEIGHT;
        FillRect(hdc, &rcContent, g_hBrushMainBg);
        
        // Footer Background
        RECT rcFooter = rcClient; rcFooter.top = rcClient.bottom - FOOTER_HEIGHT;
        FillRect(hdc, &rcFooter, g_hBrushSidebar);

        // Horizontal Divider Lines
        RECT rcLineTop = rcNavbar; rcLineTop.top = rcLineTop.bottom - 1;
        HBRUSH hBrLine = CreateSolidBrush(CLR_BORDER);
        FillRect(hdc, &rcLineTop, hBrLine);
        
        RECT rcLineBottom = rcFooter; rcLineBottom.bottom = rcLineBottom.top + 1;
        FillRect(hdc, &rcLineBottom, hBrLine);
        DeleteObject(hBrLine);
        
        // Draw version in footer (left side)
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_TEXT_MAIN);
        SelectObject(hdc, g_hFontSmall);
        RECT rcVersion = rcFooter;
        rcVersion.left = 10;
        rcVersion.top += 7;
        DrawTextW(hdc, L"v3.0", -1, &rcVersion, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        // Draw GitHub icon/link in footer (right side)
        RECT rcGitHub = rcFooter;
        rcGitHub.right -= 10;
        rcGitHub.top += 7;
        SetTextColor(hdc, CLR_ACCENT);
        DrawTextW(hdc, L"🔗 GitHub", -1, &rcGitHub, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hWnd, &ps);
        break;
    }

    // --- 4. BUTON ÇİZİMLERİ (Owner Draw) ---
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        // Nav buttons (1001-1004 and 1013)
        if ((pDIS->CtlID >= 1001 && pDIS->CtlID <= 1004) || pDIS->CtlID == IDB_NAV_CUSTOMIZATION) {
            PaintNavButton(pDIS);
            return TRUE;
        }
        break;
    }

    // --- 5. KONTROL RENKLERİ ---
    case WM_CTLCOLORSTATIC: case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, CLR_TEXT_MAIN);

        // DEĞİŞİKLİK: Metin arka planını şeffaf yapmak yerine OPAQUE (Opak) yapıyoruz.
        // Ve arka plan rengini, pencere arka plan rengiyle (CLR_BG_MAIN) aynı yapıyoruz.
        // Bu sayede metin yazılmadan önce arkasındaki eski pikseller bu renkle boyanıp temizlenir.
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, CLR_BG_MAIN);

        return (INT_PTR)g_hBrushMainBg;
    }
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam; 
        SetTextColor(hdc, CLR_TEXT_MAIN); 
        SetBkColor(hdc, CLR_INPUT_BG);
        return (INT_PTR)g_hBrushEdit;
    }

    // --- 6. KOMUT İŞLEYİCİ (Buton Tıklamaları) ---
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            // Navigasyon - New tab order: Home, Lonelith, Settings, SysInfo, Customization
        case IDB_NAV_HOME: SwitchTab(0); break;
        case IDB_NAV_LONELITH: SwitchTab(1); break;
        case IDB_NAV_SETTINGS: SwitchTab(2); break;
        case IDB_NAV_INFO: SwitchTab(3); break;
        case IDB_NAV_CUSTOMIZATION: SwitchTab(4); break;

            // İşlevler
        case IDB_SELECT_FOLDER:
            if (SelectTargetFolder()) {
                SetWindowTextW(g_hPathDisplay, g_targetPath.c_str());
                SetWindowTextW(g_hEditDefaultPath, g_targetPath.c_str());
                LogMessage(L"📁 Hedef klasör değiştirildi.");
            }
            break;
            
        case IDB_SAVE_AUTH_KEY:
        {
            wchar_t authBuf[512];
            GetWindowTextW(g_hEditAuthKey, authBuf, 512);
            g_lonelithAuthKey = authBuf;
            SaveAuthKey(g_lonelithAuthKey);
            MessageBoxW(hWnd, L"Auth key güvenli bir şekilde kaydedildi! 🔐", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            LogMessage(L"🔐 Lonelith auth key kaydedildi.");
        }
        break;
        
        case IDB_SPEED_TEST:
        {
            LogMessage(L"🚀 Hız testi başlatılıyor...");
            std::thread(TestInternetSpeed).detach();
        }
        break;
        
        case IDB_LONELITH_REFRESH:
        {
            LogMessage(L"🔄 Lonelith dosya listesi yenileniyor...");
            std::vector<std::wstring> files = GetFilesFromLonelith();
            SendMessage(g_hLonelithFileList, LB_RESETCONTENT, 0, 0);
            for (const auto& file : files) {
                SendMessage(g_hLonelithFileList, LB_ADDSTRING, 0, (LPARAM)file.c_str());
            }
            LogMessage(L"✅ Liste yenilendi.");
        }
        break;
        
        case IDB_LONELITH_UPLOAD:
        {
            // Manual upload - select a file
            OPENFILENAMEW ofn = {0};
            wchar_t szFile[MAX_PATH] = {0};
            
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"RAR Files\0*.rar\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            
            if (GetOpenFileNameW(&ofn)) {
                LogMessage(L"📤 Manuel yükleme başlatılıyor: " + std::wstring(szFile));
                if (UploadFileToLonelith(szFile)) {
                    LogMessage(L"✅ Dosya başarıyla yüklendi.");
                    MessageBoxW(hWnd, L"Dosya başarıyla yüklendi!", L"Başarılı", MB_OK | MB_ICONINFORMATION);
                } else {
                    LogMessage(L"❌ Yükleme başarısız.");
                    MessageBoxW(hWnd, L"Dosya yüklenemedi. Detaylar için günlüğe bakın.", L"Hata", MB_OK | MB_ICONERROR);
                }
            }
        }
        break;
        
        case IDB_TOGGLE_THEME:
        {
            ToggleTheme();
            LogMessage(g_isDarkMode ? L"🌙 Karanlık tema etkinleştirildi" : L"☀️ Aydınlık tema etkinleştirildi");
        }
        break;
        
        case IDB_CLEAR_LOG:
        {
            ClearLog();
        }
        break;
        
        case IDB_TEST_GITHUB:
        {
            LogMessage(L"🔍 GitHub bağlantı testi başlatılıyor...");
            std::thread(TestGitHubConnectionManual).detach();
        }
        break;
        
        case IDC_COMBO_LONELITH_URL:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int idx = SendMessage(g_hComboLonelithUrl, CB_GETCURSEL, 0, 0);
                if (idx == 2) { // "Select another" option
                    ShowWindow(g_hEditCustomUrl, SW_SHOW);
                } else {
                    ShowWindow(g_hEditCustomUrl, SW_HIDE);
                    wchar_t urlBuf[256];
                    SendMessage(g_hComboLonelithUrl, CB_GETLBTEXT, idx, (LPARAM)urlBuf);
                    g_lonelithUrl = urlBuf;
                    LogMessage(L"🌐 Lonelith URL değiştirildi: " + g_lonelithUrl);
                    // Check health of new URL
                    std::thread([]{
                        CheckLonelithUrlHealth(g_lonelithUrl);
                    }).detach();
                }
            }
        }
        break;
        
        case IDC_EDIT_CUSTOM_URL:
        {
            if (HIWORD(wParam) == EN_CHANGE) {
                wchar_t urlBuf[256];
                GetWindowTextW(g_hEditCustomUrl, urlBuf, 256);
                if (wcslen(urlBuf) > 0) {
                    g_lonelithUrl = urlBuf;
                    LogMessage(L"🌐 Özel Lonelith URL: " + g_lonelithUrl);
                    // Check health of custom URL
                    std::thread([]{
                        CheckLonelithUrlHealth(g_lonelithUrl);
                    }).detach();
                }
            }
        }
        break;

        case IDB_SAVE_SETTINGS:
            if (MessageBoxW(hWnd, L"Ayarları kaydetmek istiyor musunuz?", L"Ayarları Kaydet", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                SaveSettings();
                MessageBoxW(hWnd, L"Ayarlar başarıyla kaydedildi! ✅", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            }
            break;
        
        // Customization tab controls
        case IDC_COMBO_PROGRESS_MODE:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int idx = SendMessage(g_hComboProgressMode, CB_GETCURSEL, 0, 0);
                g_progressBarMode = idx;
                
                // Enable/disable custom value edit based on selection
                EnableWindow(g_hEditProgressCustom, idx == 3);
                
                ApplyProgressBarMode();
            }
        }
        break;
        
        case IDB_APPLY_TRAY_ICON:
        {
            int idx = SendMessage(g_hComboTrayIcon, CB_GETCURSEL, 0, 0);
            g_selectedTrayIcon = idx;
            ApplyTrayIconSelection();
        }
        break;
        
        case IDC_LONELITH_FILE_LIST:
        {
            // Handle double-click on file list
            if (HIWORD(wParam) == LBN_DBLCLK) {
                int idx = SendMessage(g_hLonelithFileList, LB_GETCURSEL, 0, 0);
                if (idx != LB_ERR) {
                    wchar_t fileName[512];
                    SendMessage(g_hLonelithFileList, LB_GETTEXT, idx, (LPARAM)fileName);
                    
                    LogMessage(L"📥 Dosya indirme başlatılıyor: " + std::wstring(fileName));
                    
                    // Download file in separate thread
                    std::thread([fileName]() {
                        ShowFileOnLonelith(fileName);
                    }).detach();
                }
            }
        }
        break;

        case IDB_RESET_APP: ResetApp(hWnd); break;

        case IDB_UNINSTALL_APP: // Kendini İmha Et Butonu
            if (MessageBoxW(hWnd, L"⚠️ DİKKAT! Programı KALICI OLARAK silmek istiyor musunuz?", L"KENDİNİ İMHA ET", MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2) == IDYES) {
                PerformSelfDestruct(false);
            }
            break;

            // Tepsi Menüsü Komutları
        case IDM_SHOW:
            if (ShowLoginDialog()) { ShowWindow(hWnd, SW_SHOW); SetForegroundWindow(hWnd); }
            break;
        case IDM_EXIT: DestroyWindow(hWnd); break;
        }
        break;

        // --- 7. BAŞLANGIÇ (USB İzlemeyi Kaydet) ---
    case WM_CREATE:
        RegisterDeviceNotifications(hWnd);
        break;

        // --- 8. USB CİHAZ ALGILAMA ---
    case WM_DEVICECHANGE:
    {
        if (wParam == DBT_DEVICEARRIVAL) // Yeni cihaz takıldı
        {
            PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
            if (lpdb && lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                DWORD dwUnitMask = lpdbv->dbcv_unitmask;

                for (char i = 0; i < 26; ++i)
                {
                    if (dwUnitMask & (1 << i))
                    {
                        // Sürücü harfini oluştur (Örn: "E:\\")
                        wchar_t driveRoot[4] = { 0 };
                        driveRoot[0] = (wchar_t)('A' + i);
                        driveRoot[1] = L':';
                        driveRoot[2] = L'\\';
                        driveRoot[3] = L'\0';

                        std::wstring sDrive(driveRoot);
                        LogMessage(L"🔌 Donanım sinyali alındı: " + sDrive);

                        // Yedekleme işlemini ayrı bir thread'de başlat
                        std::thread(SmartBackupStarter, sDrive).detach();
                    }
                }
            }
        }
    }
    break;

    // --- 9. SİSTEM TEPSİSİ (TRAY ICON) ---
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONDBLCLK) { // Çift tıkla aç
            if (ShowLoginDialog()) { ShowWindow(hWnd, SW_SHOW); SetForegroundWindow(hWnd); }
        }
        else if (lParam == WM_RBUTTONUP) { // Sağ tıkla menü aç
            ShowContextMenu(hWnd);
        }
        break;

    // --- 9.5. UPDATE LONELITH FILE LIST (ASYNC) ---
    case WM_UPDATE_LONELITH_FILES:
        if (g_hLonelithFileList) {
            SendMessage(g_hLonelithFileList, LB_RESETCONTENT, 0, 0);
            
            // Thread-safe read of cached files
            std::vector<std::wstring> filesCopy;
            {
                std::lock_guard<std::mutex> lock(g_lonelithFilesMutex);
                filesCopy = g_cachedLonelithFiles;
            }
            
            if (filesCopy.empty()) {
                SendMessage(g_hLonelithFileList, LB_ADDSTRING, 0, (LPARAM)L"📭 Dosya bulunamadı.");
            } else {
                for (const auto& file : filesCopy) {
                    SendMessage(g_hLonelithFileList, LB_ADDSTRING, 0, (LPARAM)file.c_str());
                }
            }
        }
        break;

    // --- 10. FOOTER CLICK (GitHub Link) ---
    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        
        // Check if click is in footer area (right side where GitHub link is)
        if (pt.y >= rcClient.bottom - FOOTER_HEIGHT && pt.y <= rcClient.bottom) {
            // Check if click is on right side (GitHub link area)
            if (pt.x >= rcClient.right - 100) {
                // Open repository in browser
                ShellExecuteW(NULL, L"open", L"https://github.com/prescionx/ShadowCopy", NULL, NULL, SW_SHOWNORMAL);
                LogMessage(L"🔗 GitHub repository açıldı.");
            }
        }
        break;
    }

        // --- 11. ÇIKIŞ ---
    case WM_DESTROY:
        g_isWindowAlive = false;  // Signal threads that window is being destroyed
        PostQuitMessage(0);
        break;  

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void SaveSettings() {
    g_startWithWindows = (SendMessage(g_hCheckStartup, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (g_startWithWindows) StartupManager::AddToStartup(); else StartupManager::RemoveFromStartup();
    g_startInTray = (SendMessage(g_hCheckStartTray, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DWORD trayVal = g_startInTray ? 1 : 0;
    bool silentMode = (SendMessage(g_hCheckSilent, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DWORD silentVal = silentMode ? 1 : 0;
    g_leaveGoodbyeNote = (SendMessage(g_hCheckGoodbye, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DWORD goodbyeVal = g_leaveGoodbyeNote ? 1 : 0;
    g_autoUpload = (SendMessage(g_hCheckAutoUpload, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DWORD autoUploadVal = g_autoUpload ? 1 : 0;
    wchar_t path[MAX_PATH]; GetWindowTextW(g_hEditDefaultPath, path, MAX_PATH); g_targetPath = path;
    wchar_t passBuf[128]; GetWindowTextW(g_hEditPassword, passBuf, 128); g_appPassword = passBuf;
    
    // Customization settings
    DWORD progressMode = g_progressBarMode;
    DWORD customProgress = g_customProgressValue;
    DWORD trayIcon = g_selectedTrayIcon;
    
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD pathSize = static_cast<DWORD>((g_targetPath.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"TargetPath", 0, REG_SZ, (BYTE*)g_targetPath.c_str(), pathSize);
        DWORD passSize = static_cast<DWORD>((g_appPassword.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"AppPassword", 0, REG_SZ, (BYTE*)g_appPassword.c_str(), passSize);
        RegSetValueExW(hKey, L"SilentMode", 0, REG_DWORD, (BYTE*)&silentVal, sizeof(silentVal));
        RegSetValueExW(hKey, L"StartInTray", 0, REG_DWORD, (BYTE*)&trayVal, sizeof(trayVal));
        RegSetValueExW(hKey, L"GoodbyeNote", 0, REG_DWORD, (BYTE*)&goodbyeVal, sizeof(goodbyeVal));
        RegSetValueExW(hKey, L"AutoUpload", 0, REG_DWORD, (BYTE*)&autoUploadVal, sizeof(autoUploadVal));
        DWORD urlSize = static_cast<DWORD>((g_lonelithUrl.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"LonelithUrl", 0, REG_SZ, (BYTE*)g_lonelithUrl.c_str(), urlSize);
        
        // Save customization settings
        RegSetValueExW(hKey, L"ProgressBarMode", 0, REG_DWORD, (BYTE*)&progressMode, sizeof(progressMode));
        RegSetValueExW(hKey, L"CustomProgressValue", 0, REG_DWORD, (BYTE*)&customProgress, sizeof(customProgress));
        RegSetValueExW(hKey, L"TrayIconSelection", 0, REG_DWORD, (BYTE*)&trayIcon, sizeof(trayIcon));
        
        RegCloseKey(hKey);
    }
    SetWindowTextW(g_hPathDisplay, g_targetPath.c_str());
    SetWindowTextW(g_hEditDefaultPath, g_targetPath.c_str());
}

void LoadSettings() {
    HKEY hKey;
    DWORD silentVal = 1; DWORD trayVal = 1; DWORD goodbyeVal = 0; DWORD autoUploadVal = 0; DWORD size = sizeof(DWORD);
    DWORD progressMode = 0; DWORD customProgress = 50; DWORD trayIcon = 0; DWORD darkMode = 0;
    bool isFirstRun = true;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        isFirstRun = false;
        wchar_t buffer[MAX_PATH]; DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExW(hKey, L"TargetPath", NULL, NULL, (BYTE*)buffer, &bufferSize) == ERROR_SUCCESS) g_targetPath = buffer;
        wchar_t passBuf[128]; DWORD passSize = sizeof(passBuf);
        if (RegQueryValueExW(hKey, L"AppPassword", NULL, NULL, (BYTE*)passBuf, &passSize) == ERROR_SUCCESS) g_appPassword = passBuf;
        if (RegQueryValueExW(hKey, L"SilentMode", NULL, NULL, (BYTE*)&silentVal, &size) != ERROR_SUCCESS) silentVal = 1;
        DWORD sizeTray = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"StartInTray", NULL, NULL, (BYTE*)&trayVal, &sizeTray) != ERROR_SUCCESS) trayVal = 1;
        DWORD sizeGb = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"GoodbyeNote", NULL, NULL, (BYTE*)&goodbyeVal, &sizeGb) != ERROR_SUCCESS) goodbyeVal = 0;
        DWORD sizeAuto = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"AutoUpload", NULL, NULL, (BYTE*)&autoUploadVal, &sizeAuto) != ERROR_SUCCESS) autoUploadVal = 0;
        wchar_t urlBuf[256]; DWORD urlSize = sizeof(urlBuf);
        if (RegQueryValueExW(hKey, L"LonelithUrl", NULL, NULL, (BYTE*)urlBuf, &urlSize) == ERROR_SUCCESS) g_lonelithUrl = urlBuf;
        else g_lonelithUrl = L"localhost:3000";
        
        // Load customization settings
        DWORD sizeProgress = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"ProgressBarMode", NULL, NULL, (BYTE*)&progressMode, &sizeProgress) != ERROR_SUCCESS) progressMode = 0;
        DWORD sizeCustom = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"CustomProgressValue", NULL, NULL, (BYTE*)&customProgress, &sizeCustom) != ERROR_SUCCESS) customProgress = 50;
        DWORD sizeTrayIcon = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TrayIconSelection", NULL, NULL, (BYTE*)&trayIcon, &sizeTrayIcon) != ERROR_SUCCESS) trayIcon = 0;
        DWORD sizeDark = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"DarkMode", NULL, NULL, (BYTE*)&darkMode, &sizeDark) != ERROR_SUCCESS) darkMode = 0;
        
        RegCloseKey(hKey);
        g_startInTray = (trayVal != 0); g_leaveGoodbyeNote = (goodbyeVal != 0); g_autoUpload = (autoUploadVal != 0);
        g_progressBarMode = progressMode; g_customProgressValue = customProgress; g_selectedTrayIcon = trayIcon;
        g_manualTrayIconSelection = (trayIcon != 0);  // If not default, it's manual
        g_isDarkMode = (darkMode != 0);
    }
    else { 
        g_startInTray = true; silentVal = 1; g_leaveGoodbyeNote = false; g_appPassword = L"145366"; g_autoUpload = false; 
        g_lonelithUrl = L"localhost:3000"; g_progressBarMode = 0; g_customProgressValue = 50; g_selectedTrayIcon = 0;
        g_manualTrayIconSelection = false;  // Default is auto mode
        g_isDarkMode = false;  // Default is light mode
    }
    if (g_targetPath.empty()) g_targetPath = GetDefaultPath();
    if (isFirstRun) { g_startWithWindows = true; StartupManager::AddToStartup(); }
    else { g_startWithWindows = StartupManager::IsInStartup(); }
}

void PaintNavButton(LPDRAWITEMSTRUCT pDIS)
{
    HDC hdc = pDIS->hDC; RECT rc = pDIS->rcItem; bool isPressed = (pDIS->itemState & ODS_SELECTED);
    // Updated tab order: Home=0, Lonelith=1, Settings=2, SysInfo=3, Customization=4
    bool isActive = (pDIS->CtlID == IDB_NAV_HOME && g_currentTab == 0) || 
                    (pDIS->CtlID == IDB_NAV_LONELITH && g_currentTab == 1) || 
                    (pDIS->CtlID == IDB_NAV_SETTINGS && g_currentTab == 2) ||
                    (pDIS->CtlID == IDB_NAV_INFO && g_currentTab == 3) ||
                    (pDIS->CtlID == IDB_NAV_CUSTOMIZATION && g_currentTab == 4);
    COLORREF bgCol = CLR_BG_SIDEBAR; COLORREF txtCol = RGB(80, 80, 80);
    
    if (pDIS->CtlID == IDB_UNINSTALL_APP) { 
        bgCol = isPressed ? RGB(180, 0, 0) : CLR_DANGER; 
        txtCol = RGB(255, 255, 255); 
    } else { 
        if (isActive) { 
            bgCol = RGB(220, 230, 255); 
            txtCol = CLR_ACCENT; 
        } else if (isPressed) { 
            bgCol = RGB(210, 210, 210); 
        } 
    }
    
    HBRUSH hBrush = CreateSolidBrush(bgCol); FillRect(hdc, &rc, hBrush); DeleteObject(hBrush);
    
    // Draw bottom indicator bar for active navbar buttons
    if (isActive && pDIS->CtlID != IDB_UNINSTALL_APP) { 
        RECT rcBar = rc; 
        rcBar.top = rcBar.bottom - 4; 
        HBRUSH hBar = CreateSolidBrush(CLR_ACCENT); 
        FillRect(hdc, &rcBar, hBar); 
        DeleteObject(hBar); 
    }
    
    wchar_t buf[256]; GetWindowTextW(pDIS->hwndItem, buf, 256); 
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, txtCol);
    HFONT useFont = (isActive || pDIS->CtlID == IDB_UNINSTALL_APP) ? g_hFontSubtitle : g_hFontNormal; 
    SelectObject(hdc, useFont);
    
    RECT rcText = rc; 
    if (pDIS->CtlID != IDB_UNINSTALL_APP) rcText.left += 10;
    
    UINT format = (pDIS->CtlID == IDB_UNINSTALL_APP) ? (DT_SINGLELINE | DT_VCENTER | DT_CENTER) : (DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    DrawTextW(hdc, buf, -1, &rcText, format);
}

void LogMessage(const std::wstring& message) {
    if (g_hStatusText) {
        SYSTEMTIME st; GetLocalTime(&st);
        wchar_t timeStr[64]; swprintf_s(timeStr, L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
        std::wstring fullMessage = timeStr + message + L"\r\n";
        
        // Get current line count
        int lineCount = SendMessageW(g_hStatusText, EM_GETLINECOUNT, 0, 0);
        
        // If we exceed or reach the limit, remove the oldest lines
        if (lineCount >= MAX_LOG_LINES) {
            // Delete the first LOG_CLEANUP_LINES to make room
            int firstLineIndex = SendMessageW(g_hStatusText, EM_LINEINDEX, 0, 0);
            int lineToDeleteEnd = SendMessageW(g_hStatusText, EM_LINEINDEX, LOG_CLEANUP_LINES, 0);
            SendMessageW(g_hStatusText, EM_SETSEL, firstLineIndex, lineToDeleteEnd);
            SendMessageW(g_hStatusText, EM_REPLACESEL, FALSE, (LPARAM)L"");
        }
        
        // Append new message at the end
        int len = GetWindowTextLengthW(g_hStatusText);
        SendMessageW(g_hStatusText, EM_SETSEL, len, len);
        SendMessageW(g_hStatusText, EM_REPLACESEL, FALSE, (LPARAM)fullMessage.c_str());
        
        // Auto-scroll to bottom
        SendMessageW(g_hStatusText, EM_SCROLLCARET, 0, 0);
    }
}

std::wstring FormatBytes(uintmax_t bytes) {
    const wchar_t* suffixes[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int i = 0; double dblBytes = (double)bytes;
    while (dblBytes > 1024 && i < 4) { dblBytes /= 1024; i++; }
    wchar_t buf[64]; swprintf_s(buf, L"%.2f %s", dblBytes, suffixes[i]);
    return std::wstring(buf);
}

// --- RAR KAYNAK YÖNETİMİ ---
bool ExtractRarTool(std::wstring& outPath) {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    outPath = std::wstring(tempPath) + L"internal_rar.exe";

    // Eğer zaten varsa tekrar çıkarmaya gerek yok (performans için)
    if (fs::exists(outPath)) return true;

    // Resource'dan çıkarma işlemi
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_RAR_EXE), RT_RCDATA);
    if (!hRes) {
        // Eğer RC dosyasında tanımlı değilse sistemdeki rar'ı dene
        outPath = L"winrar.exe";
        return true;
    }

    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return false;

    DWORD dataSize = SizeofResource(NULL, hRes);
    void* pData = LockResource(hData);
    if (!pData) return false;

    std::ofstream exeFile(outPath, std::ios::binary);
    if (exeFile.is_open()) {
        exeFile.write((const char*)pData, dataSize);
        exeFile.close();
        return true;
    }
    return false;
}

// --- METADATA VE YEDEKLEME ---
void StartBackupProcess(const std::wstring& sourceDrive) {
    if (g_targetPath.empty()) return;

    // Çakışmaları önlemek için her thread'e biraz gecikme verelim
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    LogMessage(L"⏳ Analiz ve şifreleme işlemi başlatılıyor: " + sourceDrive);

    try {
        // --- 1. Metadata ve Hazırlık ---
        SYSTEMTIME st; GetLocalTime(&st);
        wchar_t timestamp[64];
        swprintf_s(timestamp, L"%04d-%02d-%02d_%02d-%02d-%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        // Hedef klasörü oluştur
        if (!fs::exists(g_targetPath)) {
            fs::create_directories(g_targetPath);
        }

        std::wstring rarFileName = L"USB_Secure_" + std::wstring(timestamp) + L".rar";
        std::wstring fullRarPath = g_targetPath + L"\\" + rarFileName;

        // Geçici çalışma alanı
        wchar_t tempPath[MAX_PATH]; GetTempPathW(MAX_PATH, tempPath);
        std::wstring workDir = std::wstring(tempPath) + L"SC_Temp_" + timestamp;
        fs::create_directories(workDir);

        std::wstring infoFilePath = workDir + L"\\FlashInfo_Log.txt";

        // Flash Bilgilerini Topla
        wchar_t volumeName[MAX_PATH] = { 0 };
        wchar_t fileSysName[MAX_PATH] = { 0 };
        DWORD serialNumber = 0, maxCompLen = 0, fileSysFlags = 0;
        GetVolumeInformationW(sourceDrive.c_str(), volumeName, MAX_PATH, &serialNumber, &maxCompLen, &fileSysFlags, fileSysName, MAX_PATH);

        ULARGE_INTEGER freeBytes, totalBytes, totalFree;
        GetDiskFreeSpaceExW(sourceDrive.c_str(), &freeBytes, &totalBytes, &totalFree);

        // --- DÜZELTİLEN KISIM: GÜVENLİ DOSYA TARAMA ---
        std::wstringstream fileListSS;
        int fileCount = 0;
        uintmax_t totalSize = 0;

        // İteratör Ayarları: İzin verilmeyenleri atla + Sembolik linkleri takip etme
        auto dirOptions = fs::directory_options::skip_permission_denied;

        try {
            // try-catch bloğu iteratörün kendisi için
            for (auto& entry : fs::recursive_directory_iterator(sourceDrive, dirOptions)) {
                try {
                    // Tekil dosya hatası tüm döngüyü bozmasın
                    if (fs::is_regular_file(entry)) {
                        fileCount++;
                        uintmax_t fSize = fs::file_size(entry);
                        totalSize += fSize;
                        fileListSS << L"   - " << entry.path().filename().wstring() << L" (" << FormatBytes(fSize) << L")\n";
                    }
                }
                catch (const fs::filesystem_error& ex) {
                    // Erişim hatası alınan dosyayı loga yaz ama işlemi durdurma
                    fileListSS << L"   ! [Erişim Engellendi]: " << ex.path1().wstring() << L"\n";
                }
                catch (...) {
                    continue; // Bilinmeyen hataları atla
                }
            }
        }
        catch (const std::exception& e) {
            LogMessage(L"⚠️ Dosya listesi oluşturulurken bazı klasörlere erişilemedi (Normaldir).");
        }
        // ------------------------------------------------

        // Metadata dosyasına yaz
        std::wofstream infoFile(infoFilePath);
        infoFile.imbue(std::locale(""));
        if (infoFile.is_open()) {
            infoFile << L"=========================================\n";
            infoFile << L"        SHADOW COPY - RAPOR\n";
            infoFile << L"=========================================\n";
            infoFile << L"Tarih: " << timestamp << L"\n";
            infoFile << L"Sürücü: " << sourceDrive << L"\n";
            infoFile << L"Etiket: " << volumeName << L"\n";
            infoFile << L"Dosya Sistemi: " << fileSysName << L"\n";
            infoFile << L"-----------------------------------------\n";
            infoFile << L"Kapasite: " << FormatBytes(totalBytes.QuadPart) << L"\n";
            infoFile << L"Toplam Dosya (Erişilen): " << fileCount << L"\n";
            infoFile << L"-----------------------------------------\n";
            infoFile << L"DOSYA LİSTESİ:\n";
            infoFile << fileListSS.str();
            infoFile.close();
        }

        // 2. RAR Aracını Hazırla
        std::wstring rarExePath;
        if (!ExtractRarTool(rarExePath)) {
            LogMessage(L"❌ HATA: RAR motoru oluşturulamadı!");
            return;
        }

        // 3. RAR Komut Satırı
        // EKLENEN PARAMETRELER:
        // -dh : Open shared files (Kullanımda olan dosyaları da açmaya çalış)
        // -inul : Hata mesajlarını sustur (CMD penceresinde hata basmasın)
        // -y : Tüm sorulara 'Evet' de
        std::wstring params = L" a -r -p\"literat\" -hp -m3 -dh -y \"" + fullRarPath + L"\" \"" + sourceDrive + L"*\" \"" + infoFilePath + L"\"";

        SHELLEXECUTEINFOW shExInfo = { 0 };
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = NULL;
        shExInfo.lpVerb = L"open";
        shExInfo.lpFile = rarExePath.c_str();
        shExInfo.lpParameters = params.c_str();
        shExInfo.nShow = SW_HIDE;

        LogMessage(L"🔒 Şifreleme başlatıldı (Parola: literat)...");
        UpdateProgressBar(0, false);  // Start showing progress

        if (ShellExecuteExW(&shExInfo)) {
            // Simulate progress during RAR creation
            // In real implementation, we'd monitor the RAR process
            UpdateProgressBar(50, false);
            
            WaitForSingleObject(shExInfo.hProcess, INFINITE);
            CloseHandle(shExInfo.hProcess);
            
            UpdateProgressBar(100, false);

            if (fs::exists(fullRarPath)) {
                LogMessage(L"✅ Güvenli arşiv oluşturuldu:\n   " + rarFileName);
                wchar_t infoBuf[256];
                swprintf_s(infoBuf, L"%d dosya şifrelendi ve arşivlendi.", fileCount);
                SendNotification(L"İşlem Tamamlandı 🔒", infoBuf);
                
                // Upload to Lonelith if auto-upload enabled and internet is available
                if (g_autoUpload && g_hasInternet && !g_lonelithAuthKey.empty()) {
                    LogMessage(L"🌐 Otomatik yükleme aktif, Lonelith'e yükleme başlatılıyor...");
                    UpdateProgressBar(0, false);
                    if (UploadFileToLonelith(fullRarPath)) {
                        LogMessage(L"✅ Dosya başarıyla Lonelith'e yüklendi.");
                        UpdateProgressBar(100, false);
                    }
                    else {
                        LogMessage(L"⚠️ Lonelith yüklemesi tamamlanamadı.");
                    }
                }
                else if (g_hasInternet && !g_lonelithAuthKey.empty()) {
                    LogMessage(L"ℹ️ Otomatik yükleme devre dışı. Manuel yükle butonu ile yükleyebilirsiniz.");
                }
                else if (!g_hasInternet) {
                    LogMessage(L"ℹ️ İnternet bağlantısı yok, dosya yerel olarak saklandı.");
                }
                else if (g_lonelithAuthKey.empty()) {
                    LogMessage(L"ℹ️ Lonelith auth key ayarlanmamış, dosya sadece yerel olarak saklandı.");
                }
            }
            else {
                LogMessage(L"❌ Arşiv oluşturulamadı (İzin hatası veya boş sürücü).");
            }
        }
        else {
            LogMessage(L"❌ RAR işlemi başlatılamadı.");
        }

        // Temizlik
        try { fs::remove_all(workDir); }
        catch (...) {}
        
        // Return progress bar to marquee state
        UpdateProgressBar(0, true);

    }
    catch (const std::exception& e) {
        wchar_t errBuf[256];
        swprintf_s(errBuf, L"❌ HATA: %S", e.what());
        LogMessage(errBuf);
        UpdateProgressBar(0, true);  // Return to marquee on error
    }
}

void ToggleTheme() {
    g_isDarkMode = !g_isDarkMode;
    ApplyTheme();
    
    // Save theme preference to registry
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD isDark = g_isDarkMode ? 1 : 0;
        RegSetValueExW(hKey, L"DarkMode", 0, REG_DWORD, (BYTE*)&isDark, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void ApplyTheme() {
    // Update color scheme based on theme
    if (g_isDarkMode) {
        CLR_BG_MAIN = CLR_DARK_BG_MAIN;
        CLR_BG_SIDEBAR = CLR_DARK_BG_SIDEBAR;
        CLR_TEXT_MAIN = CLR_DARK_TEXT_MAIN;
        CLR_TEXT_SECONDARY = CLR_DARK_TEXT_SECONDARY;
        CLR_ACCENT = CLR_DARK_ACCENT;
        CLR_BORDER = CLR_DARK_BORDER;
        CLR_INPUT_BG = CLR_DARK_INPUT_BG;
    } else {
        CLR_BG_MAIN = CLR_LIGHT_BG_MAIN;
        CLR_BG_SIDEBAR = CLR_LIGHT_BG_SIDEBAR;
        CLR_TEXT_MAIN = CLR_LIGHT_TEXT_MAIN;
        CLR_TEXT_SECONDARY = CLR_LIGHT_TEXT_SECONDARY;
        CLR_ACCENT = CLR_LIGHT_ACCENT;
        CLR_BORDER = CLR_LIGHT_BORDER;
        CLR_INPUT_BG = CLR_LIGHT_INPUT_BG;
    }
    
    // Only recreate brushes if they're not stock objects
    if (!g_brushesAreStock) {
        if (g_hBrushMainBg) DeleteObject(g_hBrushMainBg);
        if (g_hBrushSidebar) DeleteObject(g_hBrushSidebar);
        if (g_hBrushEdit) DeleteObject(g_hBrushEdit);
        
        g_hBrushMainBg = CreateSolidBrush(CLR_BG_MAIN);
        g_hBrushSidebar = CreateSolidBrush(CLR_BG_SIDEBAR);
        g_hBrushEdit = CreateSolidBrush(CLR_INPUT_BG);
        
        // If recreation fails, fall back to stock objects
        if (!g_hBrushMainBg || !g_hBrushSidebar || !g_hBrushEdit) {
            if (g_hBrushMainBg) DeleteObject(g_hBrushMainBg);
            if (g_hBrushSidebar) DeleteObject(g_hBrushSidebar);
            if (g_hBrushEdit) DeleteObject(g_hBrushEdit);
            
            g_hBrushMainBg = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
            g_hBrushSidebar = (HBRUSH)GetStockObject(GRAY_BRUSH);
            g_hBrushEdit = (HBRUSH)GetStockObject(WHITE_BRUSH);
            g_brushesAreStock = true;
        }
    }
    
    // Update theme toggle button text
    if (g_hThemeToggleBtn) {
        SetWindowTextW(g_hThemeToggleBtn, g_isDarkMode ? L"☀️ Aydınlık Moda Geç" : L"🌙 Karanlık Moda Geç");
    }
    
    // Force complete window redraw
    InvalidateRect(g_hMainWindow, NULL, TRUE);
    
    // Redraw all tabs
    for (int i = 0; i < TAB_COUNT; i++) {
        for (HWND hCtrl : g_tabControls[i]) {
            InvalidateRect(hCtrl, NULL, TRUE);
        }
    }
}

void CreateTrayIcon() {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hMainWindow;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(g_hInst, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L" ");
    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() { Shell_NotifyIcon(NIM_DELETE, &g_nid); }

void ShowContextMenu(HWND hWnd) {
    POINT pt; GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_SHOW, L"🔒");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"❌");
    SetForegroundWindow(hWnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
    if (cmd == IDM_SHOW) SendMessage(hWnd, WM_COMMAND, IDM_SHOW, 0);
    else if (cmd == IDM_EXIT) DestroyWindow(hWnd);
    DestroyMenu(hMenu);
}

std::wstring GetDefaultPath() {
    wchar_t localAppData[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH) > 0) {
        std::wstring defaultPath = std::wstring(localAppData) + L"\\USBBackups";
        try { fs::create_directories(defaultPath); }
        catch (...) {}
        return defaultPath;
    }
    return L"C:\\USBBackups";
}

bool SelectTargetFolder() {
    IFileDialog* pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions; pfd->GetOptions(&dwOptions);
        pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
        if (SUCCEEDED(pfd->Show(g_hMainWindow))) {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath = NULL;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    g_targetPath = pszPath;
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return !g_targetPath.empty();
}