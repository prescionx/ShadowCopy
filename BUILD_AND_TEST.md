# Build and Test Guide

## Quick Start

### Prerequisites
- Windows 10 or later
- Visual Studio 2019 or later
- Windows SDK
- WinRAR (for backup functionality)

### Build Steps

1. **Open Project**
   ```
   Open ShadowCopy.vcxproj in Visual Studio
   ```

2. **Select Configuration**
   - Configuration: Release
   - Platform: x64

3. **Build**
   ```
   Build > Build Solution (Ctrl+Shift+B)
   ```

4. **Output**
   ```
   Executable: x64/Release/ShadowCopy.exe
   ```

---

## Testing Checklist

### Phase 1: Basic Functionality ✓
- [ ] Application launches without errors
- [ ] Login screen appears (if not in tray mode)
- [ ] Password entry works
- [ ] Password shake animation on wrong input
- [ ] Successful login opens main window
- [ ] Window icon appears in titlebar

### Phase 2: UI Layout ✓
- [ ] Top navbar displays correctly (60px height)
- [ ] 4 navigation buttons visible: Home, Settings, Info, Lonelith
- [ ] Footer displays correctly (30px height)
- [ ] Progress bar visible in footer (4px height, marquee mode)
- [ ] Active tab indicator shows (blue bar at bottom of active button)
- [ ] Content area between navbar and footer

### Phase 3: Navigation ✓
- [ ] Click Home button - switches to Tab 0
- [ ] Click Settings button - switches to Tab 1
- [ ] Click Info button - switches to Tab 2
- [ ] Click Lonelith button - switches to Tab 3
- [ ] Active button highlights correctly
- [ ] Content changes for each tab
- [ ] Info tab shows system information

### Phase 4: Home Tab Features ✓
- [ ] Target folder path displays
- [ ] Change button opens folder picker
- [ ] Selected folder updates path
- [ ] Log window shows messages
- [ ] Timestamp format: [HH:MM:SS]

### Phase 5: Settings Tab Features ✓
- [ ] Windows startup checkbox works
- [ ] Start in tray checkbox works
- [ ] Silent mode checkbox works
- [ ] Leave goodbye note checkbox works
- [ ] Password field shows/hides text
- [ ] Default path displays
- [ ] Save Settings button works
- [ ] Settings persist after restart
- [ ] Reset button clears settings
- [ ] Uninstall button triggers confirmation

### Phase 6: Lonelith Tab Features ✓
- [ ] Auth key input field visible
- [ ] Auth key save button works
- [ ] Auth key persists (encrypted)
- [ ] Auto-upload checkbox works
- [ ] Auto-upload setting persists
- [ ] GitHub connection status displays
- [ ] Lonelith server health displays
- [ ] Speed test result displays
- [ ] Speed test button works
- [ ] File list (empty or populated)
- [ ] Refresh button works
- [ ] Manual upload button opens file picker

### Phase 7: Progress Bar ✓
- [ ] Marquee animation on startup
- [ ] Progress shows during RAR creation
- [ ] Progress shows during upload
- [ ] Progress shows during speed test
- [ ] Returns to marquee after operations
- [ ] Returns to marquee on error

### Phase 8: Network Features ✓
- [ ] Internet connection detected on startup
- [ ] Speed test runs automatically (2s delay)
- [ ] Speed displays in Mbps format
- [ ] Manual speed test works
- [ ] GitHub connection check works
- [ ] Status shows: ✅ Connected or ❌ Failed

### Phase 9: Tray Icon ✓
- [ ] Tray icon appears on startup
- [ ] Icon changes based on status:
  - Red X (no WinRAR)
  - World ! (no internet)
  - Green checkmark (all OK)
- [ ] Double-click shows login
- [ ] Right-click shows menu
- [ ] Menu has Show and Exit options

### Phase 10: USB Backup ✓
- [ ] Insert USB drive
- [ ] Drive detected (log message)
- [ ] Backup process starts
- [ ] Progress bar shows activity
- [ ] RAR file created in target folder
- [ ] Log shows completion
- [ ] Notification appears (if not silent)

### Phase 11: Auto-Upload ✓
- [ ] Enable auto-upload in Lonelith tab
- [ ] Set auth key
- [ ] Insert USB drive
- [ ] Backup creates RAR file
- [ ] Upload starts automatically (if internet)
- [ ] Progress bar shows upload
- [ ] Log shows upload status

### Phase 12: Manual Upload ✓
- [ ] Click Manual Upload button
- [ ] File picker opens
- [ ] Select RAR file
- [ ] Upload starts (placeholder)
- [ ] Progress bar shows activity
- [ ] Log shows result

---

## Common Issues

### Build Errors

**Error: Cannot open include file**
```
Solution: Install Windows SDK via Visual Studio Installer
```

**Error: LNK1104: cannot open 'wininet.lib'**
```
Solution: Verify Windows SDK installation
```

**Error: Unresolved external symbol**
```
Solution: Check #pragma comment(lib, ...) directives
```

### Runtime Errors

**Application crashes on startup**
```
Check: Windows version (requires Windows 10+)
Check: Visual C++ Redistributables installed
```

**Login window doesn't appear**
```
Check: Registry settings (may be in tray-start mode)
Check: Look for tray icon
```

**No tray icon visible**
```
Check: Notification area settings
Check: "Always show all icons" enabled
```

**USB not detected**
```
Check: Drive is properly connected
Check: Drive appears in File Explorer
Check: Log window for error messages
```

**Progress bar not animating**
```
Check: Windows version supports PBS_MARQUEE
Check: Common Controls library version
```

---

## Performance Testing

### Speed Test Validation
1. Run speed test multiple times
2. Compare with speedtest.net or fast.com
3. Should be within ±20% of reference
4. Test on different network speeds

### Progress Bar Timing
1. Create large RAR (>100MB)
2. Verify progress updates
3. Check for smooth transitions
4. Verify return to marquee

### Internet Monitoring
1. Start app with internet
2. Wait 15+ seconds
3. Disconnect internet
4. Verify icon changes within 15s
5. Reconnect internet
6. Verify icon changes back

---

## Security Testing

### Password Protection
1. Enter wrong password 3 times
2. Verify shake animation each time
3. On 3rd attempt, app should self-destruct
4. Check that:
   - Registry cleaned
   - Backup folder deleted (if configured)
   - Application exits

### Auth Key Encryption
1. Save auth key
2. Open Registry Editor
3. Navigate to: `HKCU\Software\ShadowCopier`
4. Check `LonelithAuthKey` value
5. Should be encrypted hex string
6. Not readable as plain text

### Self-Destruct Trigger
1. Create file on desktop: `sil321.txt`
2. Wait 15 seconds
3. Application should self-destruct
4. All data deleted

---

## Debugging Tips

### Enable Verbose Logging
The log window shows all operations. Watch for:
- `⏳` - Operation starting
- `✅` - Success
- `❌` - Error
- `⚠️` - Warning
- `ℹ️` - Information

### Check Registry
```
Path: HKEY_CURRENT_USER\Software\ShadowCopier
Values:
- TargetPath
- AppPassword
- SilentMode
- StartInTray
- GoodbyeNote
- AutoUpload
- LonelithAuthKey (encrypted)
```

### Monitor Tray Icon
Icon changes indicate state:
1. Red X → No WinRAR
2. World ! → No internet
3. Green ✓ → All OK

### Progress Bar States
```cpp
// Check in code:
g_isMarquee    // true = marquee, false = progress
g_progressValue // 0-100 when in progress mode
```

---

## Known Limitations

### Lonelith API
- Upload, get files, show functions are placeholders
- Will return false/empty until API is implemented
- Logs show "API Not Available" messages

### Progress Monitoring
- RAR creation progress is simulated (0→50→100)
- Actual monitoring would require:
  - RAR process polling
  - File size tracking
  - Time estimation

### Icon Design
- Using placeholder icons
- Custom icons recommended for production

### Animation
- Page transitions use instant switch
- Framework ready for slide/fade effects
- Can be enhanced with SetLayeredWindowAttributes

---

## Production Deployment

### Before Release
1. **Build Release version** (not Debug)
2. **Test on clean Windows install**
3. **Verify WinRAR detection**
4. **Test with/without internet**
5. **Verify tray icon changes**
6. **Test all tabs and features**
7. **Check for memory leaks**
8. **Run for extended period**

### Distribution
1. Create installer (optional)
2. Include documentation
3. Provide WinRAR installation link
4. Include Lonelith setup guide
5. Security warnings about self-destruct

### User Setup
1. Install WinRAR
2. Run ShadowCopy.exe
3. Login with default password: `145366`
4. Go to Settings:
   - Set new password
   - Configure target folder
   - Enable auto-upload (optional)
5. Go to Lonelith tab:
   - Enter auth key
   - Enable auto-upload if desired
   - Test speed

---

## Support & Troubleshooting

### Log Files
All operations logged to UI log window
Copy-paste for support requests

### Registry Backup
Before testing self-destruct:
```
Export: HKEY_CURRENT_USER\Software\ShadowCopier
```

### Recovery
If locked out:
1. Delete registry key
2. Restart application
3. Default settings restored

### Contact
See repository issues for support

---

**Document Version**: 1.0  
**Last Updated**: December 17, 2025  
**Platform**: Windows 10+  
**Build Tool**: Visual Studio 2019+
