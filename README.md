# ShadowCopy

ShadowCopy is a secure USB backup application that automatically encrypts and backs up USB drive contents when connected to the system, with cloud upload capabilities.

## Features

### Core Functionality
- Automatic USB drive detection and monitoring
- Encrypted RAR archive creation with password protection
- Secure backup to configurable target directory
- Self-destruct mechanism with password protection
- System tray integration with dynamic status icons

### Cloud Integration (Lonelith)
- **NEW**: Dedicated cloud management interface
- Encrypted authentication key storage (Windows DPAPI)
- Auto-upload option for immediate cloud backup when internet is available
- Manual upload button for on-demand cloud backup
- File list management (when API is available)
- Connection health monitoring

### UI/UX Improvements (Latest Release)
- **NEW**: Top horizontal navigation bar (replaces left sidebar)
- **NEW**: 4 tabs: Home, Settings, System Info, Lonelith
- **NEW**: Footer progress bar with marquee and actual progress modes
- **NEW**: Password entry shake animation on error
- **NEW**: Window titlebar icon
- **NEW**: Page transition framework (ready for animations)

### Network Features
- **NEW**: Internet speed tester (automatic on first connection)
- **NEW**: Manual speed test button
- **NEW**: GitHub connection status check
- **NEW**: Lonelith server health monitoring
- Internet connection monitoring (every 15 seconds)
- Dynamic tray icon based on status

### Progress Indicators
- **NEW**: Footer progress bar
- **Marquee mode**: When idle or waiting
- **Progress mode**: During RAR creation, uploads, speed tests
- Automatic return to marquee after operations

### WinRAR Status Monitoring
- Automatically checks for WinRAR installation on startup
- Displays **Red X** tray icon if WinRAR is not found
- Ensures backup functionality requires WinRAR

### Tray Icon States
The system tray icon dynamically changes to reflect current status:
1. **Red X** - WinRAR not found (critical issue)
2. **World with !** - No internet connection (warning)
3. **Green Checkmark** - All systems operational and connected

## Configuration

### UI Layout
- **Top Navigation**: 60px high horizontal navbar with 4 tabs
- **Footer**: 30px high with 4px progress bar
- **Content Area**: Between navbar and footer
- **Progress States**: Marquee (idle) or Actual (0-100%)

### Lonelith Cloud Settings
1. Navigate to **Lonelith** tab (☁️ icon)
2. Enter your Lonelith authentication key
3. Click **💾 Kaydet** to save (encrypted with Windows DPAPI)
4. Enable **Auto-Upload** checkbox for automatic uploads
5. Use **🚀 Hız Testi Yap** to test internet speed
6. Use **⬆️ Manuel Yükle** to upload files on-demand

### Auth Key Storage
The Lonelith authentication key is stored encrypted in the Windows registry at:
`HKEY_CURRENT_USER\Software\ShadowCopier\LonelithAuthKey`

**Encryption**: Windows Data Protection API (DPAPI) - user-specific encryption

### Settings Persistence
All settings are saved to registry:
- Target backup path
- Application password
- Silent mode
- Start in tray
- Leave goodbye note
- Auto-upload option

### Network Features
- **Speed Test**: Automatically runs on first internet connection
- **Connection Checks**: Tests GitHub connection.txt and Lonelith health
- **Status Display**: Shows connection status in Lonelith tab

## Requirements

- Windows 10 or later
- WinRAR installed (for backup functionality)
- Internet connection (optional, for cloud upload)
- Visual Studio 2019 or later for building

## Building

1. Open `ShadowCopy.vcxproj` in Visual Studio
2. Build the solution (Release x64 recommended)
3. The executable will be in the Release/x64 folder

## Security Features

- Password-protected login on application launch
- Encrypted RAR archives with password "literat"
- Auth key encryption using Windows DPAPI
- Self-destruct mechanism with configurable options
- Secure registry storage

## Notes

### UI Animations
- Password entry: Shake animation on incorrect login
- Page transitions: Framework in place for future enhancement
- Progress bar: Smooth marquee animation when idle

### Progress Bar States
- **Marquee**: Active when no operations running (default)
- **Progress**: Shows 0-100% during:
  - RAR file creation
  - File upload to Lonelith
  - Internet speed test
- **Auto-return**: Returns to marquee after completion

### Lonelith Integration Status
- **Framework**: ✅ Complete and ready
- **API Implementation**: ⚠️ Pending (requires Lonelith repository access)
- **Placeholder Functions**:
  - Upload file
  - Get file list
  - Show/download file
  - Health check

These functions will be completed once the Lonelith API specifications are available from the repository.

### Icon Files
Custom icons should be created for:
- `tray_no_winrar.ico` - Red X icon
- `tray_no_internet.ico` - World icon with exclamation mark
- `tray_connected.ico` - Green checkmark icon

Currently using placeholder icons (copies of small.ico).

### Security Features
- Password-protected login with shake animation on error
- Limited login attempts (3 max) before self-destruct
- Encrypted RAR archives with password "literat"
- Auth key encryption using Windows DPAPI (user-specific)
- Self-destruct mechanism with configurable options
- Secure registry storage for all sensitive data

### Documentation
- `FEATURE_IMPLEMENTATION.md` - Complete feature documentation
- `LONELITH_INTEGRATION.md` - API integration guide
- `STATUS.md` - Previous implementation status
- `IMPLEMENTATION_SUMMARY.md` - Technical details

## License

Copyright (c) 2025
