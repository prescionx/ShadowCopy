# ShadowCopy

ShadowCopy is a secure USB backup application that automatically encrypts and backs up USB drive contents when connected to the system.

## Features

### Core Functionality
- Automatic USB drive detection and monitoring
- Encrypted RAR archive creation with password protection
- Secure backup to configurable target directory
- Self-destruct mechanism with password protection
- System tray integration

### New Features (Latest Release)

#### WinRAR Status Monitoring
- Automatically checks for WinRAR installation on startup
- Displays a **Red X** tray icon if WinRAR is not found on the system
- Ensures backup functionality requires WinRAR to be installed

#### Internet Connection Monitoring
- Monitors internet connection status every 15 seconds
- Updates tray icon based on connection status:
  - **Green Checkmark**: Active internet connection detected
  - **World icon with exclamation mark**: No internet connection
  - **Red X**: WinRAR not installed (takes priority)

#### Cloud Upload Integration (Lonelith)
- Automatically uploads encrypted backup files when stable internet connection is available
- Uses encrypted storage for authentication key (Windows DPAPI)
- Auth key is stored securely in Windows registry with encryption
- Upload only occurs when both WinRAR is installed and internet is available

## Configuration

### Auth Key Setup
The Lonelith authentication key is stored encrypted in the Windows registry at:
`HKEY_CURRENT_USER\Software\ShadowCopier\LonelithAuthKey`

To set the auth key, it must be configured through the application settings or registry.

### Icon States
The system tray icon changes to reflect the current status:
1. **Red X** - WinRAR not found (critical issue)
2. **World with !** - No internet connection (warning)
3. **Green Checkmark** - All systems operational and connected

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

- Custom icons should be created for:
  - `tray_no_winrar.ico` - Red X icon
  - `tray_no_internet.ico` - World icon with exclamation mark
  - `tray_connected.ico` - Green checkmark icon
- Currently using placeholder icons (copies of small.ico)
- Lonelith API integration requires access to the Lonelith repository for full implementation

## License

Copyright (c) 2025
