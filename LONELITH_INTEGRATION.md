# Lonelith API Integration Notes

## Overview
This document describes the Lonelith file upload integration that needs to be completed.

## Current Status
The ShadowCopy application includes a placeholder implementation for uploading encrypted backup files to the Lonelith server. The actual API implementation requires access to the Lonelith repository.

## Required Information from Lonelith Repository
The following information is needed from https://github.com/prescionx/Lonelith:

1. **API Endpoint URL**: The base URL for the Lonelith file upload service
2. **Authentication Method**: How the auth key should be sent (Header, Query param, etc.)
3. **Request Format**: The HTTP method and content type (likely POST with multipart/form-data)
4. **Request Headers**: Required headers including auth key format
5. **Response Format**: Expected response structure for success/failure
6. **File Upload Field Name**: The form field name for the file upload
7. **Additional Parameters**: Any other required parameters (metadata, tags, etc.)

## C# Client Example Reference
According to the problem statement, the C# client example at:
https://github.com/prescionx/Lonelith?tab=readme-ov-file#c-client-example

This example should show:
- How to authenticate with the auth key
- How to upload files
- How to handle responses

## Implementation Placeholder
The current implementation in `ShadowCopy.cpp` includes:

```cpp
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
    
    // TODO: Implement actual Lonelith API upload
    // Placeholder for real implementation
    
    return false;
}
```

## Recommended C++ Implementation Approach

Once the API details are known, the implementation should:

1. Use WinHTTP or WinINet for HTTP requests
2. Read file content into memory or stream
3. Create multipart/form-data POST request
4. Add authentication header with auth key
5. Send file data
6. Parse response
7. Handle errors appropriately
8. Log success/failure

## Example C++ HTTP Upload Pattern

```cpp
// Pseudo-code example (to be adapted based on Lonelith API specs)
HINTERNET hSession = WinHttpOpen(L"ShadowCopy/1.0", ...);
HINTERNET hConnect = WinHttpConnect(hSession, serverName, port, 0);
HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", endpoint, ...);

// Add auth header
WinHttpAddRequestHeaders(hRequest, authHeader, ...);

// Send file data
WinHttpSendRequest(hRequest, headers, headersLen, fileData, fileLen, totalLen, 0);

// Receive response
WinHttpReceiveResponse(hRequest, NULL);

// Read response data and parse result
```

## Auth Key Management
The auth key is currently:
- Encrypted using Windows DPAPI (CryptProtectData)
- Stored in registry at: `HKEY_CURRENT_USER\Software\ShadowCopier\LonelithAuthKey`
- Loaded on application startup
- Used for authentication when uploading files

## Testing
To test the upload functionality:
1. Set a valid Lonelith auth key in the registry (encrypted format)
2. Ensure internet connection is active
3. Trigger a USB backup
4. Monitor logs for upload status
5. Verify file appears on Lonelith server

## Next Steps
1. Access the Lonelith repository to get API documentation
2. Review the C# client example
3. Implement the HTTP upload using WinHTTP or similar
4. Test with actual Lonelith server
5. Handle edge cases and errors
