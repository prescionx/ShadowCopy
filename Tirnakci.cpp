#include <windows.h>
#include <dbt.h>
#include <string>
#include <shlobj.h>
#include <shlwapi.h>
#include <filesystem>
#include <commdlg.h>
#include <thread>
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

// Gerekli kütüphanelerin otomatik linklenmesi (MSVC için)
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "version.lib")

namespace fs = std::filesystem;

// --- ID TANIMLAMALARI ---
#define WM_TRAYICON (WM_USER + 1)
#define IDM_EXIT 100
#define IDM_SHOW 101

// Resource ID (Rar.exe'nin .rc dosyasında bu ID ile tanımlı olması gerekir)
#define IDR_RAR_EXE 101 

// Navigasyon
#define IDB_NAV_HOME 1001
#define IDB_NAV_SETTINGS 1002
#define IDB_NAV_INFO 1003

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

// Login Penceresi ID'leri
#define IDC_LOGIN_EDIT 301
#define IDB_LOGIN_BTN 302

// --- RENKLER ---
const COLORREF CLR_BG_MAIN = RGB(248, 249, 250);
const COLORREF CLR_BG_SIDEBAR = RGB(240, 242, 245);
const COLORREF CLR_TEXT_MAIN = RGB(33, 37, 41);
const COLORREF CLR_ACCENT = RGB(13, 110, 253);
const COLORREF CLR_BORDER = RGB(222, 226, 230);
const COLORREF CLR_DANGER = RGB(220, 53, 69);

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

// UI Kaynakları
HFONT g_hFontTitle, g_hFontSubtitle, g_hFontNormal, g_hFontSmall, g_hFontMono;
HBRUSH g_hBrushMainBg, g_hBrushSidebar;

// Global Kontrol Handle'ları
HWND g_hNavBtnHome, g_hNavBtnSettings, g_hNavBtnInfo;
HWND g_hPathDisplay, g_hStatusText;
HWND g_hCheckStartup, g_hCheckSilent, g_hEditDefaultPath, g_hCheckStartTray, g_hCheckGoodbye;
HWND g_hEditPassword;
HWND g_hInfoText;

std::vector<HWND> g_tabControls[3];

int g_currentTab = 0;
const wchar_t* CLASS_NAME = L"ShadowCopierApp";
const wchar_t* LOGIN_CLASS_NAME = L"ShadowCopierLogin";
const wchar_t* APP_REG_NAME = L"ShadowCopier";
const wchar_t* REG_SUBKEY = L"Software\\ShadowCopier";

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
void PaintSidebarButton(LPDRAWITEMSTRUCT pDIS);
std::wstring GetSystemInfo();
std::wstring FormatBytes(uintmax_t bytes);
bool IsSilentMode();
void SendNotification(const std::wstring& title, const std::wstring& msg);

void SetModernStyle(HWND hControl);
void DestructionWatcher();
void PerformSelfDestruct(bool triggeredByFile);
bool ShowLoginDialog();
bool ExtractRarTool(std::wstring& outPath);

// Yardımcı: UI Oluşturma
HWND CreateCtrl(int tabIndex, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hParent, HMENU hMenu) {
    int adjustedX = x + 200;
    HWND hCtrl = CreateWindowW(lpClassName, lpWindowName, dwStyle, adjustedX, y, nWidth, nHeight, hParent, hMenu, g_hInst, NULL);
    if (tabIndex >= 0 && tabIndex < 3) {
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

// --- MAIN ---
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInst = hInstance;
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"ShadowCopierInstanceMutex");
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

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = g_hBrushMainBg;
    wcex.lpszClassName = CLASS_NAME;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
    RegisterClassExW(&wcex);

    g_hMainWindow = CreateWindowExW(0, CLASS_NAME, L"Shadow Copier",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 900, 650,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hMainWindow) return FALSE;

    BOOL dark = FALSE;
    DwmSetWindowAttribute(g_hMainWindow, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    CreateUI(g_hMainWindow);
    SwitchTab(0);
    CreateTrayIcon();
    CheckExistingDrives();

    std::thread(DestructionWatcher).detach();

    if (g_isAutoStart || g_startInTray) {
        ShowWindow(g_hMainWindow, SW_HIDE);
        if (!IsSilentMode()) SendNotification(L"Shadow Copier Aktif", L"Sistem tepsisinde çalışıyor. 🛡");
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
    g_hFontTitle = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");
    g_hFontSubtitle = CreateFontW(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");
    g_hFontNormal = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");
    g_hFontSmall = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");
    g_hFontMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Consolas");

    g_hBrushMainBg = CreateSolidBrush(CLR_BG_MAIN);
    g_hBrushSidebar = CreateSolidBrush(CLR_BG_SIDEBAR);
}

void CleanupResources()
{
    DeleteObject(g_hFontTitle); DeleteObject(g_hFontSubtitle);
    DeleteObject(g_hFontNormal); DeleteObject(g_hFontSmall); DeleteObject(g_hFontMono);
    DeleteObject(g_hBrushMainBg); DeleteObject(g_hBrushSidebar);
}

void SetModernStyle(HWND hControl) {
    SendMessage(hControl, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SetWindowTheme(hControl, L"Explorer", NULL);
}

// --- UI OLUŞTURMA ---
void CreateUI(HWND hWnd)
{
    int btnY = 60; int btnH = 50;
    g_hNavBtnHome = CreateWindowW(L"BUTTON", L"🏠  Ana Sayfa", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, btnY, 200, btnH, hWnd, (HMENU)IDB_NAV_HOME, g_hInst, NULL);
    g_hNavBtnSettings = CreateWindowW(L"BUTTON", L"⚙  Ayarlar", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, btnY + btnH, 200, btnH, hWnd, (HMENU)IDB_NAV_SETTINGS, g_hInst, NULL);
    g_hNavBtnInfo = CreateWindowW(L"BUTTON", L"ℹ️  Sistem Bilgisi", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, btnY + (btnH * 2), 200, btnH, hWnd, (HMENU)IDB_NAV_INFO, g_hInst, NULL);

    // TAB 0: HOME
    CreateLabel(0, hWnd, L"Shadow Copier", 40, 30, 400, 40, g_hFontTitle);
    CreateLabel(0, hWnd, L"USB algılandığında şifreli yedekleme başlatılır.", 40, 75, 550, 25, g_hFontNormal);

    CreateLabel(0, hWnd, L"📂 Hedef Klasör:", 40, 120, 200, 25, g_hFontSubtitle);
    g_hPathDisplay = CreateCtrl(0, L"EDIT", g_targetPath.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY | WS_BORDER, 40, 150, 450, 30, hWnd, (HMENU)IDC_EDIT_PATH);
    SendMessage(g_hPathDisplay, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND hBtnChange = CreateCtrl(0, L"BUTTON", L"Değiştir", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 500, 150, 100, 30, hWnd, (HMENU)IDB_SELECT_FOLDER);
    SetModernStyle(hBtnChange);

    CreateLabel(0, hWnd, L"📝 İşlem Günlüğü:", 40, 200, 200, 25, g_hFontSubtitle);
    g_hStatusText = CreateCtrl(0, L"EDIT", L"Sistem hazır. USB bekleniyor...\r\n", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_BORDER, 40, 230, 600, 300, hWnd, NULL);
    SendMessage(g_hStatusText, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

    // TAB 1: SETTINGS
    CreateLabel(1, hWnd, L"Ayarlar", 40, 30, 200, 40, g_hFontTitle);
    CreateLabel(1, hWnd, L"Başlangıç Seçenekleri", 40, 80, 300, 25, g_hFontSubtitle);
    g_hCheckStartup = CreateCtrl(1, L"BUTTON", L"Windows ile başlat", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 110, 300, 30, hWnd, (HMENU)IDC_CHECK_STARTUP);
    SetModernStyle(g_hCheckStartup);
    if (g_startWithWindows) SendMessage(g_hCheckStartup, BM_SETCHECK, BST_CHECKED, 0);

    g_hCheckStartTray = CreateCtrl(1, L"BUTTON", L"Program açılışta gizli başlasın (Tepsi)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 140, 350, 30, hWnd, (HMENU)IDC_CHECK_START_TRAY);
    SetModernStyle(g_hCheckStartTray);
    if (g_startInTray) SendMessage(g_hCheckStartTray, BM_SETCHECK, BST_CHECKED, 0);

    g_hCheckSilent = CreateCtrl(1, L"BUTTON", L"Sessiz Mod (Bildirim gönderme)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 170, 300, 30, hWnd, (HMENU)IDC_CHECK_SILENT);
    SetModernStyle(g_hCheckSilent);
    if (IsSilentMode()) SendMessage(g_hCheckSilent, BM_SETCHECK, BST_CHECKED, 0);

    g_hCheckGoodbye = CreateCtrl(1, L"BUTTON", L"Kendini imha ederken 'elveda.txt' bırak", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 40, 200, 400, 30, hWnd, (HMENU)IDC_CHECK_GOODBYE);
    SetModernStyle(g_hCheckGoodbye);
    if (g_leaveGoodbyeNote) SendMessage(g_hCheckGoodbye, BM_SETCHECK, BST_CHECKED, 0);

    int secX = 350;
    CreateLabel(1, hWnd, L"Güvenlik", 40 + secX, 80, 200, 25, g_hFontSubtitle);
    CreateLabel(1, hWnd, L"Erişim Parolası:", 40 + secX, 110, 150, 20, g_hFontNormal);
    g_hEditPassword = CreateCtrl(1, L"EDIT", g_appPassword.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 40 + secX, 135, 180, 30, hWnd, (HMENU)IDC_EDIT_PASSWORD);
    SendMessage(g_hEditPassword, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    CreateLabel(1, hWnd, L"(Hatalı girişte imha tetiklenir)", 40 + secX, 170, 220, 20, g_hFontSmall);

    CreateLabel(1, hWnd, L"Varsayılan Yedekleme Yolu:", 40, 240, 300, 25, g_hFontSubtitle);
    g_hEditDefaultPath = CreateCtrl(1, L"EDIT", g_targetPath.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY | WS_BORDER, 40, 270, 450, 30, hWnd, (HMENU)IDC_EDIT_PATH);
    SendMessage(g_hEditDefaultPath, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND hBtnSave = CreateCtrl(1, L"BUTTON", L"💾  Ayarları Kaydet", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 320, 180, 40, hWnd, (HMENU)IDB_SAVE_SETTINGS);
    SetModernStyle(hBtnSave);

    CreateLabel(1, hWnd, L"Tehlikeli Bölge", 40, 390, 200, 25, g_hFontSubtitle);
    HWND hBtnReset = CreateCtrl(1, L"BUTTON", L"⚠️ Uygulamayı Sıfırla (Temizle)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 420, 250, 35, hWnd, (HMENU)IDB_RESET_APP);
    SendMessage(hBtnReset, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    HWND hBtnUninstall = CreateCtrl(1, L"BUTTON", L"☢️ KALDIR VE YOK ET (UNINSTALL)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 310, 420, 280, 35, hWnd, (HMENU)IDB_UNINSTALL_APP);
    SendMessage(hBtnUninstall, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    // TAB 2: INFO
    CreateLabel(2, hWnd, L"Donanım Bilgisi", 40, 30, 300, 40, g_hFontTitle);
    g_hInfoText = CreateCtrl(2, L"STATIC", L"Yükleniyor...", WS_CHILD | WS_VISIBLE, 40, 90, 620, 450, hWnd, NULL);
    SendMessage(g_hInfoText, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
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

    ss << L" [CPU]      Çekirdek: " << si.dwNumberOfProcessors
        << L" | Mimari: " << (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? L"x64" : L"x86") << L"\r\n";
    ss << L" [RAM]      Toplam: " << FormatBytes(statex.ullTotalPhys)
        << L" | Boş: " << FormatBytes(statex.ullAvailPhys) << L"\r\n";

    std::wstring moboBrand = GetRegString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardManufacturer");
    std::wstring moboModel = GetRegString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct");
    ss << L" [ANAKART] " << moboBrand << L" - " << moboModel << L"\r\n";

    DISPLAY_DEVICEW dd; dd.cb = sizeof(dd);
    ss << L" [GPU]      ";
    if (EnumDisplayDevicesW(NULL, 0, &dd, 0)) ss << dd.DeviceString;
    else ss << L"Algılanamadı";
    ss << L"\r\n\r\n=== DİSK SÜRÜCÜLERİ ===\r\n";
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
    ss << L"\r\n=== YAZILIM ===\r\n Sürüm: v3.0 (RAR Encrypted)\r\n Durum: Korumalı\n";
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
    g_currentTab = index;
    for (int i = 0; i < 3; i++) for (HWND hCtrl : g_tabControls[i]) ShowWindow(hCtrl, SW_HIDE);
    for (HWND hCtrl : g_tabControls[index]) ShowWindow(hCtrl, SW_SHOW);
    if (index == 2) SetWindowTextW(g_hInfoText, GetSystemInfo().c_str());
    InvalidateRect(g_hNavBtnHome, NULL, FALSE);
    InvalidateRect(g_hNavBtnSettings, NULL, FALSE);
    InvalidateRect(g_hNavBtnInfo, NULL, FALSE);
    RECT rc = { 200, 0, 900, 650 };
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
        if (!IsSilentMode()) SendNotification(L"Shadow Copier Gizlendi", L"Uygulama arka planda ve kilitlendi.");
        return 0;

        // --- 3. GÖRSEL ARAYÜZ ÇİZİMİ (WM_PAINT) ---
    case WM_PAINT:
    {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT rcClient; GetClientRect(hWnd, &rcClient);

        // Sol Menü (Sidebar) Arkaplanı
        RECT rcSidebar = rcClient; rcSidebar.right = 200;
        FillRect(hdc, &rcSidebar, g_hBrushSidebar);

        // İçerik Alanı Arkaplanı
        RECT rcContent = rcClient; rcContent.left = 200;
        FillRect(hdc, &rcContent, g_hBrushMainBg);

        // Dikey Ayırıcı Çizgi
        RECT rcLine = rcSidebar; rcLine.left = rcLine.right - 1;
        HBRUSH hBrLine = CreateSolidBrush(CLR_BORDER);
        FillRect(hdc, &rcLine, hBrLine);
        DeleteObject(hBrLine);

        EndPaint(hWnd, &ps);
        break;
    }

    // --- 4. BUTON ÇİZİMLERİ (Owner Draw) ---
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        // Sol menü butonları için özel çizim fonksiyonunu çağır
        if (pDIS->CtlID >= 1000 && pDIS->CtlID <= 1010) {
            PaintSidebarButton(pDIS);
            return TRUE;
        }
        break;
    }

    // --- 5. KONTROL RENKLERİ ---
    case WM_CTLCOLORSTATIC: case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam; SetTextColor(hdc, CLR_TEXT_MAIN); SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)g_hBrushMainBg;
    }
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam; SetTextColor(hdc, CLR_TEXT_MAIN); SetBkColor(hdc, RGB(255, 255, 255));
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }

    // --- 6. KOMUT İŞLEYİCİ (Buton Tıklamaları) ---
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            // Navigasyon
        case IDB_NAV_HOME: SwitchTab(0); break;
        case IDB_NAV_SETTINGS: SwitchTab(1); break;
        case IDB_NAV_INFO: SwitchTab(2); break;

            // İşlevler
        case IDB_SELECT_FOLDER:
            if (SelectTargetFolder()) {
                SetWindowTextW(g_hPathDisplay, g_targetPath.c_str());
                LogMessage(L"📁 Hedef klasör değiştirildi.");
            }
            break;

        case IDB_SAVE_SETTINGS:
            if (MessageBoxW(hWnd, L"Ayarları kaydetmek istiyor musunuz?", L"Ayarları Kaydet", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                SaveSettings();
                MessageBoxW(hWnd, L"Ayarlar başarıyla kaydedildi! ✅", L"Bilgi", MB_OK | MB_ICONINFORMATION);
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

        // --- 10. ÇIKIŞ ---
    case WM_DESTROY:
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
    wchar_t path[MAX_PATH]; GetWindowTextW(g_hEditDefaultPath, path, MAX_PATH); g_targetPath = path;
    wchar_t passBuf[128]; GetWindowTextW(g_hEditPassword, passBuf, 128); g_appPassword = passBuf;
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD pathSize = static_cast<DWORD>((g_targetPath.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"TargetPath", 0, REG_SZ, (BYTE*)g_targetPath.c_str(), pathSize);
        DWORD passSize = static_cast<DWORD>((g_appPassword.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"AppPassword", 0, REG_SZ, (BYTE*)g_appPassword.c_str(), passSize);
        RegSetValueExW(hKey, L"SilentMode", 0, REG_DWORD, (BYTE*)&silentVal, sizeof(silentVal));
        RegSetValueExW(hKey, L"StartInTray", 0, REG_DWORD, (BYTE*)&trayVal, sizeof(trayVal));
        RegSetValueExW(hKey, L"GoodbyeNote", 0, REG_DWORD, (BYTE*)&goodbyeVal, sizeof(goodbyeVal));
        RegCloseKey(hKey);
    }
    SetWindowTextW(g_hPathDisplay, g_targetPath.c_str());
}

void LoadSettings() {
    HKEY hKey;
    DWORD silentVal = 1; DWORD trayVal = 1; DWORD goodbyeVal = 0; DWORD size = sizeof(DWORD);
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
        RegCloseKey(hKey);
        g_startInTray = (trayVal != 0); g_leaveGoodbyeNote = (goodbyeVal != 0);
    }
    else { g_startInTray = true; silentVal = 1; g_leaveGoodbyeNote = false; g_appPassword = L"145366"; }
    if (g_targetPath.empty()) g_targetPath = GetDefaultPath();
    if (isFirstRun) { g_startWithWindows = true; StartupManager::AddToStartup(); }
    else { g_startWithWindows = StartupManager::IsInStartup(); }
}

void PaintSidebarButton(LPDRAWITEMSTRUCT pDIS)
{
    HDC hdc = pDIS->hDC; RECT rc = pDIS->rcItem; bool isPressed = (pDIS->itemState & ODS_SELECTED);
    bool isActive = (pDIS->CtlID == IDB_NAV_HOME && g_currentTab == 0) || (pDIS->CtlID == IDB_NAV_SETTINGS && g_currentTab == 1) || (pDIS->CtlID == IDB_NAV_INFO && g_currentTab == 2);
    COLORREF bgCol = CLR_BG_SIDEBAR; COLORREF txtCol = RGB(80, 80, 80);
    if (pDIS->CtlID == IDB_UNINSTALL_APP) { bgCol = isPressed ? RGB(180, 0, 0) : CLR_DANGER; txtCol = RGB(255, 255, 255); }
    else { if (isActive) { bgCol = RGB(220, 230, 255); txtCol = CLR_ACCENT; } else if (isPressed) { bgCol = RGB(210, 210, 210); } }
    HBRUSH hBrush = CreateSolidBrush(bgCol); FillRect(hdc, &rc, hBrush); DeleteObject(hBrush);
    if (isActive && pDIS->CtlID != IDB_UNINSTALL_APP) { RECT rcBar = rc; rcBar.right = rcBar.left + 4; HBRUSH hBar = CreateSolidBrush(CLR_ACCENT); FillRect(hdc, &rcBar, hBar); DeleteObject(hBar); }
    wchar_t buf[256]; GetWindowTextW(pDIS->hwndItem, buf, 256); SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, txtCol);
    HFONT useFont = (isActive || pDIS->CtlID == IDB_UNINSTALL_APP) ? g_hFontSubtitle : g_hFontNormal; SelectObject(hdc, useFont);
    RECT rcText = rc; if (pDIS->CtlID != IDB_UNINSTALL_APP) rcText.left += 20;
    UINT format = (pDIS->CtlID == IDB_UNINSTALL_APP) ? (DT_SINGLELINE | DT_VCENTER | DT_CENTER) : (DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    DrawTextW(hdc, buf, -1, &rcText, format);
}

void LogMessage(const std::wstring& message) {
    if (g_hStatusText) {
        SYSTEMTIME st; GetLocalTime(&st);
        wchar_t timeStr[64]; swprintf_s(timeStr, L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
        std::wstring fullMessage = timeStr + message + L"\r\n";
        int len = GetWindowTextLengthW(g_hStatusText);
        SendMessageW(g_hStatusText, EM_SETSEL, len, len);
        SendMessageW(g_hStatusText, EM_REPLACESEL, FALSE, (LPARAM)fullMessage.c_str());
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
            infoFile << L"        SHADOW COPIER - GÜVENLİK RAPORU\n";
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

        if (ShellExecuteExW(&shExInfo)) {
            WaitForSingleObject(shExInfo.hProcess, INFINITE);
            CloseHandle(shExInfo.hProcess);

            if (fs::exists(fullRarPath)) {
                LogMessage(L"✅ Güvenli arşiv oluşturuldu:\n   " + rarFileName);
                wchar_t infoBuf[256];
                swprintf_s(infoBuf, L"%d dosya şifrelendi ve arşivlendi.", fileCount);
                SendNotification(L"İşlem Tamamlandı 🔒", infoBuf);
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

    }
    catch (const std::exception& e) {
        wchar_t errBuf[256];
        swprintf_s(errBuf, L"❌ HATA: %S", e.what());
        LogMessage(errBuf);
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