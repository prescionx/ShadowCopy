# Testing Checklist for ShadowCopy Improvements

This document outlines all changes made and how to test them.

## 1. Application Renaming ✅

### Changes Made:
- Renamed from "ShadowCopier" to "ShadowCopy"
- Updated registry path from `Software\ShadowCopier` to `Software\ShadowCopy`
- Updated window class names
- Updated mutex name

### How to Test:
1. Build and run the application
2. Check window title shows "Shadow Copy" (not "Shadow Copier")
3. Open Registry Editor (regedit)
4. Navigate to `HKEY_CURRENT_USER\Software\`
5. Verify key is named `ShadowCopy` (not `ShadowCopier`)
6. Try running two instances - should only allow one (mutex test)

**Expected Result:** Application uses "ShadowCopy" consistently throughout.

---

## 2. Async Lonelith Tab Loading ✅

### Changes Made:
- Made file list fetching asynchronous
- Added loading indicator
- Implemented thread-safe caching
- Added window lifetime checks

### How to Test:
1. Launch application and login
2. Click on "☁️ Lonelith" tab
3. **IMPORTANT:** UI should remain responsive immediately
4. File list should show "📥 Yükleniyor..." while loading
5. After a few seconds, list updates with files or "📭 Dosya bulunamadı"

**Before:** UI would freeze for 2-3 seconds when clicking Lonelith tab
**After:** UI stays responsive, shows loading indicator

### Stress Test:
1. Rapidly switch between tabs
2. Click Lonelith tab multiple times quickly
3. Application should not crash
4. Each click should start a new load (showing loading indicator)

---

## 3. LogBox Text Overlap Fix ✅

### Changes Made:
- Added 500-line limit
- Auto-cleanup of oldest 100 lines when limit reached
- Auto-scroll to bottom
- Added horizontal scrollbar

### How to Test:
1. Go to Home tab (📝 İşlem Günlüğü)
2. Generate many log entries by:
   - Inserting/removing USB drives multiple times
   - Running speed tests
   - Switching tabs
3. Verify after 500 lines, oldest entries are removed
4. Verify log auto-scrolls to bottom
5. Test horizontal scroll with long log messages

**Expected Result:** 
- No text overlap
- Smooth scrolling
- Old logs automatically cleaned up
- Always shows most recent entries

---

## 4. System Info Text Overlap Fix ✅

### Changes Made:
- Added horizontal scrollbar
- Improved text rendering

### How to Test:
1. Click on "ℹ️ Sistem Bilgisi" tab
2. Check all system information displays correctly
3. Test horizontal scrolling for long lines
4. Switch away and back - info should refresh cleanly

**Expected Result:** 
- All text readable
- No overlapping sections
- Horizontal scroll available for long lines

---

## 5. Updated TODO List ✅

### Changes Made:
- Changed from static "future features" to dynamic status
- Added completion indicators (✅ and 🔄)
- Renamed section to "📋 Özellik Durumu:"

### How to Test:
1. Go to Home tab
2. Scroll down to "📋 Özellik Durumu:" section
3. Verify shows completed features with ✅
4. Verify shows ongoing features with 🔄

**Expected Result:** 
Features shown as completed:
- ✅ Lonelith API temel entegrasyonu
- ✅ Dosya yükleme özelliği
- ✅ Dosya indirme özelliği
- ✅ Bulut dosya listesi görüntüleme
- ✅ Tray ikon özelleştirme
- ✅ Gelişmiş ilerleme çubuğu seçenekleri
- ✅ Otomatik internet bağlantı kontrolü
- ✅ Hız testi (indirme ve yükleme)

---

## 6. Tray Icon Selector Fix ✅

### Changes Made:
- Added manual selection flag
- Default mode (0) = Auto mode (follows system status)
- Fixed modes (1-3) = Manual selection (stays fixed)
- Saves manual selection state

### How to Test:

#### Test Auto Mode (Default):
1. Go to "🎨 Özelleştirme" tab
2. Set "Aktif ikon" to "Varsayılan"
3. Click "🎨 Uygula"
4. Check system tray icon
5. Disconnect internet
6. Wait 15 seconds
7. **Icon should automatically change** to "No Internet" icon
8. Reconnect internet
9. **Icon should automatically change back**

#### Test Manual Mode:
1. Go to "🎨 Özelleştirme" tab
2. Set "Aktif ikon" to "Bağlantılı (✅)"
3. Click "🎨 Uygula"
4. Check system tray icon shows connected icon
5. Disconnect internet
6. Wait 30 seconds
7. **Icon should NOT change** (stays as connected)
8. Reconnect internet
9. Icon should still be connected

#### Test Persistence:
1. Set manual icon selection
2. Close application
3. Reopen application
4. Check tray icon matches your selection

**Expected Result:** 
- Default mode = Auto-updates based on system status
- Manual modes = Stay fixed regardless of system status
- Selection persists across restarts

---

## 7. Progress Bar Verification ✅

### Changes Made:
- Verified all modes work correctly
- Proper enable/disable logic for custom value

### How to Test:

#### Mode 1: Marquee (Animasyonlu)
1. Go to "🎨 Özelleştirme" tab
2. Select "Marquee (Animasyonlu)"
3. Check footer - progress bar should show animated marquee

#### Mode 2: Full (100%)
1. Select "Full (100%)"
2. Progress bar should show 100% filled

#### Mode 3: Hide (Gizli)
1. Select "Hide (Gizli)"
2. Progress bar should disappear from footer

#### Mode 4: Custom (Özel Yüzde)
1. Select "Custom (Özel Yüzde)"
2. "Özel yüzde değeri" field should become enabled
3. Enter value: 75
4. Progress bar should show 75%
5. Try invalid values (e.g., "abc") - should default to 50
6. Try values > 100 - should cap at 100
7. Try values < 0 - should cap at 0

**Expected Result:** All modes work correctly with proper validation

---

## 8. Navbar Icons and Text ✅

### Changes Made:
- Verified all buttons have icons and text

### How to Test:
1. Check navigation bar at top
2. Verify all buttons show:
   - 🏠 Ana Sayfa
   - ☁️ Lonelith
   - ⚙ Ayarlar
   - ℹ️ Sistem Bilgisi
   - 🎨 Özelleştirme

**Expected Result:** All buttons have both icon and text

---

## Thread Safety & Stability Testing

### How to Test:
1. Rapidly switch between all tabs
2. Click Lonelith tab 10 times quickly
3. Close application while Lonelith is loading
4. Insert USB while on different tabs
5. Run speed test and immediately switch tabs

**Expected Result:** 
- No crashes
- No freezes
- No error messages
- Clean shutdown even during background operations

---

## Regression Testing

Ensure existing functionality still works:

1. **USB Detection:** Insert USB, verify backup starts
2. **Password Protection:** Test login with correct/wrong password
3. **Settings:** Change settings, verify they save
4. **Startup:** Enable "Start with Windows", verify it works
5. **Silent Mode:** Enable, verify no notifications
6. **GitHub Link:** Click footer link, opens browser

---

## Performance Testing

1. **Memory Usage:** Monitor with Task Manager
2. **CPU Usage:** Should be minimal when idle
3. **Log Performance:** Add 1000+ entries, should cleanup smoothly
4. **Thread Cleanup:** Check threads are properly cleaned up

---

## Known Limitations

1. **Lonelith API:** May show "Dosya bulunamadı" if server not accessible
2. **Internet Monitoring:** Updates every 15 seconds
3. **Progress Bar:** Marquee mode may appear static on some Windows versions

---

## Build Instructions

```cmd
1. Open ShadowCopy.vcxproj in Visual Studio 2019+
2. Select Configuration: Release
3. Select Platform: x64
4. Build > Build Solution (Ctrl+Shift+B)
5. Executable: x64/Release/ShadowCopy.exe
```

---

## Rollback Instructions

If issues are found:

```cmd
git checkout c140604  # Commit before changes
```

To see what changed:

```cmd
git diff c140604 39f3441 ShadowCopy.cpp
```

---

**Testing completed by:** _____________  
**Date:** _____________  
**Build version:** _____________  
**Issues found:** _____________
