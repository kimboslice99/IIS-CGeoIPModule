/**
 *
 *   _____                _           _   _                    _    _           _               _ _          ___   ___
 *  / ____|              | |         | | | |             ____ | |  (_)         | |             | (_)        / _ \ / _ \
 * | |     _ __ ___  __ _| |_ ___  __| | | |__  _   _   / __ \| | ___ _ __ ___ | |__   ___  ___| |_  ___ __| (_) | (_) |
 * | |    | '__/ _ \/ _` | __/ _ \/ _` | | '_ \| | | | / / _` | |/ / | '_ ` _ \| '_ \ / _ \/ __| | |/ __/ _ \__, |\__, |
 * | |____| | |  __/ (_| | ||  __/ (_| | | |_) | |_| || | (_| |   <| | | | | | | |_) | (_) \__ \ | | (_|  __/ / /   / /
 *  \_____|_|  \___|\__,_|\__\___|\__,_| |_.__/ \__, | \ \__,_|_|\_\_|_| |_| |_|_.__/ \___/|___/_|_|\___\___|/_/   /_/
 *                                               __/ |  \____/
 *                                              |___/
 */
#include "pch.h"
#include "Functions.h"
#include <Windows.h>

#define ELEMENT L"system.webServer/CGeoIPModule"

HRESULT Functions::GetConfig(IN IHttpContext* pHttpContext, OUT IAppHostElement** ppElement)
{
    HRESULT hr = S_OK;
    IAppHostAdminManager* pAdminManager = NULL;
    IAppHostElement* pSessionTrackingElement = NULL;
    IAppHostPropertyException* pPropertyException = NULL;

    // Get the IAppHostAdminManager instance
    pAdminManager = g_pHttpServer->GetAdminManager();
    if (pAdminManager == NULL)
    {
        // Admin manager not available, cannot proceed
        return E_UNEXPECTED;
    }

    // Get the configuration path
    PCWSTR pszConfigPath = pHttpContext->GetMetadata()->GetMetaPath();
    BSTR bstrConfigPath = SysAllocString(pszConfigPath);
    BSTR bstrSectionName = SysAllocString(ELEMENT);

    hr = pAdminManager->GetAdminSection(bstrSectionName, bstrConfigPath, &pSessionTrackingElement);
    SysFreeString(bstrConfigPath);
    SysFreeString(bstrSectionName);
    if (FAILED(hr) || pSessionTrackingElement == NULL)
    {
        // Failed to retrieve the section, or the section is not found
        return hr;
    }

    // Return the retrieved element
    *ppElement = pSessionTrackingElement;

    return hr;
}

HRESULT Functions::GetStringPropertyValueFromElement(IAppHostElement* pElement, BSTR pszElementName, BSTR* pStringValue)
{
    HRESULT hr = S_OK;
    IAppHostProperty* pProperty = NULL;
    VARIANT vPropertyValue;

    if (
        (pElement == NULL) ||
        (pszElementName == NULL) ||
        (pStringValue == NULL)
        )
    {
        return E_INVALIDARG;
    }

    // Get the property object for the attribute within the specified element:
    hr = pElement->GetPropertyByName(
        pszElementName,
        &pProperty);

    if (FAILED(hr))
    {
        return hr;
    }

    if (pProperty == NULL)
    {
        return E_UNEXPECTED;
    }

    // Get the attribute value:
    VariantInit(&vPropertyValue);

    hr = pProperty->get_Value(&vPropertyValue);

    if (FAILED(hr))
    {
        pProperty->Release();
        return hr;
    }

    // Check if the value is a string
    if (vPropertyValue.vt != VT_BSTR)
    {
        VariantClear(&vPropertyValue);
        pProperty->Release();
        return E_FAIL; // Value is not a string
    }

    // Allocate memory for the string value and copy the value
    *pStringValue = SysAllocString(vPropertyValue.bstrVal);

    // Release resources
    VariantClear(&vPropertyValue);
    pProperty->Release();

    if (*pStringValue == NULL)
    {
        return E_OUTOFMEMORY; // Memory allocation failed
    }

    return hr;
}

HRESULT Functions::GetBooleanPropertyValueFromElement(IAppHostElement* pElement, BSTR pszElementName, BOOL* pBoolValue)
{
    HRESULT hr = S_OK;
    IAppHostProperty* pProperty = NULL;
    VARIANT vPropertyValue;

    if (
        (pElement == NULL) ||
        (pszElementName == NULL) ||
        (pBoolValue == NULL)
        )
    {
        return E_INVALIDARG;
    }

    hr = pElement->GetPropertyByName(
        pszElementName,
        &pProperty);

    if (FAILED(hr))
    {
        return hr;
    }

    if (pProperty == NULL)
    {
        return E_UNEXPECTED;
    }

    VariantInit(&vPropertyValue);

    hr = pProperty->get_Value(&vPropertyValue);

    if (FAILED(hr))
    {
        VariantClear(&vPropertyValue);
        pProperty->Release();
        return hr;
    }

    if (vPropertyValue.vt != VT_BOOL)
    {
        VariantClear(&vPropertyValue);
        pProperty->Release();
        return E_FAIL; // Value is not a bool
    }

    // Finally, get the value:
    *pBoolValue = (vPropertyValue.boolVal == VARIANT_TRUE) ? TRUE : FALSE;

    VariantClear(&vPropertyValue);
    pProperty->Release();

    return hr;
}

BOOL Functions::IsCountryCodeListed(IN IHttpContext* pHttpContext, IN BSTR CountryCode, IN IAppHostElement* pModuleElement)
{
    BOOL Listed = FALSE;

    IAppHostElement* pCountryCodesElement = NULL;
    BSTR bstr = SysAllocString(L"countryCodes");
    HRESULT hr = pModuleElement->GetElementByName(bstr, &pCountryCodesElement);
    SysFreeString(bstr);
    if (FAILED(hr) || pCountryCodesElement == NULL)
    {
#ifdef _DEBUG
        WriteFileLogMessage("[Functions::IsCountryCodeListed]: GetElementByName failed");
        _com_error err(hr);
        WriteFileLogMessage(CStringA(err.ErrorMessage()));
#endif
        return Listed;
    }

    IAppHostElementCollection* pCollection = NULL;
    hr = pCountryCodesElement->get_Collection(&pCollection);
    pCountryCodesElement->Release();
    if (FAILED(hr) || pCollection == NULL)
    {
#ifdef _DEBUG
        WriteFileLogMessage("[Functions::IsCountryCodeListed]: get_Collection failed");
        _com_error err(hr);
        WriteFileLogMessage(CStringA(err.ErrorMessage()));
#endif
        return Listed;
    }

    DWORD count = 0;
    hr = pCollection->get_Count(&count);
    if (FAILED(hr))
    {
#ifdef _DEBUG
        WriteFileLogMessage("[Functions::IsCountryCodeListed]: get_Count failed");
        _com_error err(hr);
        WriteFileLogMessage(CStringA(err.ErrorMessage()));
#endif
        pCollection->Release();
        return Listed;
    }

    for (DWORD i = 0; i < count; ++i)
    {
        VARIANT varIndex;
        VariantInit(&varIndex);
        varIndex.vt = VT_I4;
        varIndex.lVal = i;

        IAppHostElement* pElement = NULL;
        hr = pCollection->get_Item(varIndex, &pElement);

        if (FAILED(hr) || pElement == NULL)
        {
#ifdef _DEBUG
            WriteFileLogMessage("[Functions::IsCountryCodeListed]: get_Item failed");
            _com_error err(hr);
            WriteFileLogMessage(CStringA(err.ErrorMessage()));
#endif
            continue;
        }

        BSTR bstrElementName = NULL;
        hr = pElement->get_Name(&bstrElementName);
        if (FAILED(hr) || bstrElementName == NULL)
        {
#ifdef _DEBUG
            WriteFileLogMessage("[Functions::IsCountryCodeListed]: get_Name failed");
            _com_error err(hr);
            WriteFileLogMessage(CStringA(err.ErrorMessage()));
#endif
            pElement->Release(); // Release element before continuing
            continue;
        }

        if (_wcsicmp(bstrElementName, L"add") == 0)
        {
            BSTR bstrCountryCode = NULL;
            BSTR bstr = SysAllocString(L"code");
            hr = GetStringPropertyValueFromElement(pElement, bstr, &bstrCountryCode);
            SysFreeString(bstr);

            if (SUCCEEDED(hr) && bstrCountryCode != NULL)
            {
                if (wcscmp(CountryCode, bstrCountryCode) == 0)
                {
#ifdef _DEBUG
                    WriteFileLogMessage("Found country code in config");
#endif
                    Listed = TRUE;
                    SysFreeString(bstrCountryCode);
                    SysFreeString(bstrElementName);
                    pElement->Release();
                    break;
                }
                SysFreeString(bstrCountryCode);
            }
        }

        SysFreeString(bstrElementName);
        pElement->Release();
    }

    pCollection->Release();
    return Listed;
}

/// <summary>
/// Check country code returned by GetCountryCode
/// </summary>
/// <param name="pHttpContext">Context</param>
/// <param name="COUNTRYCODE">The country code of an IP</param>
/// <param name="MODE">mode switch</param>
/// <returns></returns>
BOOL Functions::CheckCountryCode(IN IHttpContext* pHttpContext, IN CHAR* COUNTRYCODE, IN BOOL MODE, IN IAppHostElement* pModuleElement)
{
#ifdef _DEBUG
    if (MODE == FALSE) {
        WriteFileLogMessage("mode=block listed");
    }
    else {
        WriteFileLogMessage("mode=allow listed");
    }
#endif
    INT wslen = MultiByteToWideChar(CP_ACP, 0, COUNTRYCODE, (INT)strlen(COUNTRYCODE), 0, 0);
    BSTR bstrCountryCode = SysAllocStringLen(0, wslen);
    MultiByteToWideChar(CP_ACP, 0, COUNTRYCODE, (INT)strlen(COUNTRYCODE), bstrCountryCode, wslen);

    BOOL result = MODE ? FALSE : TRUE;
    if (IsCountryCodeListed(pHttpContext, bstrCountryCode, pModuleElement)) {
        result = MODE ? TRUE : FALSE;
    }

    SysFreeString(bstrCountryCode);

    return result;
}

/// <summary>
/// true to allow only listed country codes
/// </summary>
/// <param name="pHttpContext"></param>
/// <returns></returns>
BOOL Functions::GetAllowMode(IN IAppHostElement* pModuleElement)
{
    BOOL mode = FALSE;
    BSTR bstrEnabled = SysAllocString(L"allowListed");
    HRESULT hr = GetBooleanPropertyValueFromElement(pModuleElement, bstrEnabled, &mode);
    SysFreeString(bstrEnabled);

    return mode;
}

BOOL Functions::GetIsEnabled(IN IAppHostElement* pModuleElement)
{
    BOOL isEnabled = FALSE;

    BSTR bstrEnabled = SysAllocString(L"enabled");
    if (bstrEnabled == NULL)
    {
        return FALSE;
    }

    HRESULT hr = GetBooleanPropertyValueFromElement(pModuleElement, bstrEnabled, &isEnabled);
    SysFreeString(bstrEnabled);
    if (FAILED(hr))
    {
#ifdef _DEBUG
        WriteFileLogMessage("[Functions::GetIsEnabled]: GetBooleanPropertyValueFromElement failed");
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        WriteFileLogMessage(CStringA(errMsg));
#endif
    }
    return isEnabled;
}

HRESULT Functions::GetSiteId(IN IHttpContext* pHttpContext, OUT PCWSTR* str) {
    if (!pHttpContext || !str) {
        return E_POINTER;
    }

    IHttpApplication* pApp = pHttpContext->GetApplication();
    if (NULL == pApp)
    {
        return E_FAIL;
    }
    PCWSTR appId = pApp->GetApplicationId();
    if (NULL == appId)
    {
        return E_FAIL;
    }

    size_t appIdLen = wcslen(appId);
    LPWSTR modifiedAppId = (LPWSTR)pHttpContext->AllocateRequestMemory((appIdLen + 1) * sizeof(WCHAR));
    if (!modifiedAppId) {
        return E_OUTOFMEMORY;
    }

    wcscpy_s(modifiedAppId, appIdLen + 1, appId);

    // Replace all slashes
    for (size_t i = 0; i < appIdLen; i++) {
        if (modifiedAppId[i] == L'/') {
            modifiedAppId[i] = L'_';
        }
    }

    *str = modifiedAppId;

    return S_OK;
}

/// <summary>
/// Get flag from config
/// </summary>
/// <param name="pHttpContext"></param>
/// <returns>true if we should parse REMOTE_ADDR</returns>
BOOL Functions::CheckRemoteAddr(IN IAppHostElement* pModuleElement)
{
    BOOL checkRemoteAddr = FALSE;

    BSTR bstrRemoteAddr = SysAllocString(L"remoteAddr");
    if (bstrRemoteAddr == NULL)
    {
        return FALSE;
    }

    HRESULT hr = GetBooleanPropertyValueFromElement(pModuleElement, bstrRemoteAddr, &checkRemoteAddr);
    SysFreeString(bstrRemoteAddr);
    if (FAILED(hr))
    {
#ifdef _DEBUG
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        CHAR message[256];
        sprintf_s(message, sizeof(message), "[Functions::CheckRemoteAddr]: GetBooleanPropertyValueFromElement failed %s", CStringA(errMsg).GetString());
        WriteFileLogMessage(message);
#endif
        return FALSE;
    }

    return checkRemoteAddr;
}

/// <summary>
/// pick deny response, default to Close on failure
/// </summary>
/// <param name="pHttpContext"></param>
/// <returns></returns>
VOID Functions::DenyAction(IN IHttpContext* pHttpContext, IN IAppHostElement* pModuleElement)
{
    WCHAR mode[13] = L"Close";

    if (pModuleElement != NULL) {
        BSTR bstrAction = SysAllocString(L"action");
        if (bstrAction != NULL)
        {
            BSTR modeBstr = NULL;
            HRESULT hr = GetStringPropertyValueFromElement(pModuleElement, bstrAction, &modeBstr);
            SysFreeString(bstrAction);

            if (SUCCEEDED(hr) && modeBstr != NULL)
            {
                wcsncpy_s(mode, ARRAYSIZE(mode), modeBstr, _TRUNCATE);
                SysFreeString(modeBstr);
            }
#ifdef _DEBUG
            else
            {
                WriteFileLogMessage("[Functions::DenyAction]: Failed to retrieve 'action' property");
                _com_error err(hr);
                LPCTSTR errMsg = err.ErrorMessage();
                WriteFileLogMessage(CStringA(errMsg));
            }
#endif
        }
    }

    // Respond based on the mode
    IHttpResponse* pHttpResponse = pHttpContext->GetResponse();

    if (NULL == pHttpResponse)
    {
        return;
    }

    if (wcscmp(mode, L"Close") == 0)
    {
        pHttpResponse->CloseConnection();
    }
    else if (wcscmp(mode, L"NotFound") == 0)
    {
        pHttpResponse->SetStatus(404, "Not Found");
    }
    else if (wcscmp(mode, L"Forbidden") == 0)
    {
        pHttpResponse->SetStatus(403, "Forbidden");
    }
    else if (wcscmp(mode, L"Unauthorized") == 0)
    {
        pHttpResponse->SetStatus(401, "Unauthorized");
    }
    else if (wcscmp(mode, L"Reset") == 0)
    {
        pHttpResponse->ResetConnection();
    }
    else if (wcscmp(mode, L"Teapot") == 0)
    {
        pHttpResponse->SetStatus(418, "I'm a teapot");
    }
    else if (wcscmp(mode, L"Gone") == 0)
    {
        pHttpResponse->SetStatus(410, "Gone");
    }
    else
    {
        pHttpResponse->CloseConnection();
#ifdef _DEBUG
        WriteFileLogMessage("[Functions::DenyAction]: Action not recognized, defaulting to 'Close'");
#endif
    }
}

std::vector<ExceptionRules> Functions::exceptionRules(IN IHttpContext* pHttpContext, IN IAppHostElement* pModuleElement) {
    std::vector<ExceptionRules> rules;

    IAppHostElement* pAddressElement = NULL;
    BSTR bstr = SysAllocString(L"exceptionRules");
    HRESULT hr = pModuleElement->GetElementByName(bstr, &pAddressElement);
    SysFreeString(bstr);

    if (FAILED(hr) || pAddressElement == NULL) {
#ifdef _DEBUG
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        CHAR message[128];
        sprintf_s(message, sizeof(message), "[Functions::exceptionRules]: GetElementByName failed %s", CStringA(errMsg).GetString());
        WriteFileLogMessage(message);
#endif
        return rules;
    }

    IAppHostElementCollection* pCollection = NULL;
    hr = pAddressElement->get_Collection(&pCollection);
    pAddressElement->Release();

    if (FAILED(hr) || pCollection == NULL) {
#ifdef _DEBUG
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        CHAR message[128];
        sprintf_s(message, sizeof(message), "[Functions::exceptionRules]: get_Collection failed %s", CStringA(errMsg).GetString());
        WriteFileLogMessage(message);
#endif
        return rules;
    }

    DWORD count = 0;
    hr = pCollection->get_Count(&count);
    if (FAILED(hr)) {
#ifdef _DEBUG
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        CHAR message[128];
        sprintf_s(message, sizeof(message), "[Functions::exceptionRules]: get_Count failed %s", CStringA(errMsg).GetString());
        WriteFileLogMessage(message);
#endif
        pCollection->Release();
        return rules;
    }

    for (DWORD i = 0; i < count; ++i) {
        VARIANT varIndex;
        VariantInit(&varIndex);
        varIndex.vt = VT_I4;
        varIndex.lVal = i;

        IAppHostElement* pElement = NULL;
        hr = pCollection->get_Item(varIndex, &pElement);
        if (FAILED(hr) || pElement == NULL) {
#ifdef _DEBUG
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            CHAR message[128];
            sprintf_s(message, sizeof(message), "[Functions::exceptionRules]: Get_Item failed %s", CStringA(errMsg).GetString());
            WriteFileLogMessage(message);
#endif
            continue;
        }

        BSTR bstrElementName = NULL;
        hr = pElement->get_Name(&bstrElementName);
        if (FAILED(hr) || bstrElementName == NULL) {
#ifdef _DEBUG
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            CHAR message[128];
            sprintf_s(message, sizeof(message), "[Functions::exceptionRules]: get_Name failed %s", CStringA(errMsg).GetString());
            WriteFileLogMessage(message);
#endif
            pElement->Release();
            continue;
        }

        if (_wcsicmp(bstrElementName, L"add") == 0) {
            SysFreeString(bstrElementName);

            BSTR bstrFamily = NULL;
            BSTR family = SysAllocString(L"family");
            hr = GetStringPropertyValueFromElement(pElement, family, &bstrFamily);
            SysFreeString(family);

            if (FAILED(hr) || bstrFamily == NULL) {
#ifdef _DEBUG
                CHAR msg[512];
                _com_error err(hr);
                LPCTSTR errMsg = err.ErrorMessage();
                CStringA errMsgA(errMsg);
                sprintf_s(msg, sizeof(msg), "[Functions::exceptionRules]: GetStringPropertyValueFromElement failed (family) %s", (LPCSTR)errMsgA);
                WriteFileLogMessage(msg);
#endif
                pElement->Release();
                continue;
            }

            BOOL allowed = FALSE;
            BSTR allow = SysAllocString(L"allow");
            hr = GetBooleanPropertyValueFromElement(pElement, allow, &allowed);
            SysFreeString(allow);
            if (FAILED(hr)) {
#ifdef _DEBUG
                CHAR msg[512];
                _com_error err(hr);
                LPCTSTR errMsg = err.ErrorMessage();
                CStringA errMsgA(errMsg);
                sprintf_s(msg, sizeof(msg), "[Functions::exceptionRules]: GetStringPropertyValueFromElement failed (family) %s", (LPCSTR)errMsgA);
                WriteFileLogMessage(msg);
#endif
                SysFreeString(bstrFamily);
                pElement->Release();
                continue;
            }

            BSTR bstrMask = NULL;
            BSTR mask = SysAllocString(L"mask");
            hr = GetStringPropertyValueFromElement(pElement, mask, &bstrMask);
            SysFreeString(mask);

            if (FAILED(hr) || bstrMask == NULL) {
#ifdef _DEBUG
                WriteFileLogMessage("[Functions::exceptionRules]: GetStringPropertyValueFromElement failed (mask)");
#endif
                SysFreeString(bstrFamily);
                pElement->Release();
                continue;
            }

            BSTR bstrAddress = NULL;
            BSTR address = SysAllocString(L"address");
            hr = GetStringPropertyValueFromElement(pElement, address, &bstrAddress);
            SysFreeString(address);

            if (FAILED(hr) || bstrAddress == NULL) {
#ifdef _DEBUG
                WriteFileLogMessage("[Functions::exceptionRules]: GetStringPropertyValueFromElement failed (address)");
#endif
                SysFreeString(bstrFamily);
                SysFreeString(bstrMask);
                pElement->Release();
                continue;
            }

            // bstrAddress, bstrMask, bstrFamily, and mode contains config elements
            PCSTR pcstrAddress = BSTRToCharArray(pHttpContext, bstrAddress);
            PCSTR pcstrMask = BSTRToCharArray(pHttpContext, bstrMask);
            PCSTR pcstrFamily = BSTRToCharArray(pHttpContext, bstrFamily);
            SysFreeString(bstrFamily);
            SysFreeString(bstrMask);
            SysFreeString(bstrAddress);

            rules.push_back({ pcstrFamily, pcstrAddress, pcstrMask, allowed });
        }
        else {
            SysFreeString(bstrElementName);
        }

        pElement->Release();
    }

    pCollection->Release();
    return rules;
}

PWSTR Functions::charToWString(IN IHttpContext* pHttpContext, IN LPCSTR pcharArray, IN INT length)
{
    if (!pHttpContext || !pcharArray || length <= 0) {
        return nullptr;
    }

    PWSTR wString = (PWSTR)pHttpContext->AllocateRequestMemory(sizeof(WCHAR) * length);
    if (!wString) {
        return nullptr;
    }

    MultiByteToWideChar(CP_ACP, 0, pcharArray, -1, wString, length);
    return wString;
}

LPSTR Functions::BSTRToCharArray(IN IHttpContext* pHttpContext, IN BSTR bstr)
{
    if (!pHttpContext || !bstr) {
        return nullptr;
    }

    int length = WideCharToMultiByte(CP_UTF8, 0, bstr, -1, NULL, 0, NULL, NULL);
    if (length <= 0) {
        return nullptr;
    }

    LPSTR charArray = (LPSTR)pHttpContext->AllocateRequestMemory(length);
    if (!charArray) {
        return nullptr;
    }

    WideCharToMultiByte(CP_UTF8, 0, bstr, -1, charArray, length, NULL, NULL);

    return charArray;
}

#ifdef _DEBUG

LPSTR Functions::PSOCKADDRtoString(IN PSOCKADDR pSockAddr)
{
    CHAR* string = nullptr;
    if (pSockAddr->sa_family == AF_INET) {
        CHAR s[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in*)pSockAddr)->sin_addr), s, INET_ADDRSTRLEN);
        string = new CHAR[strlen(s) + 1];
        strcpy_s(string, strlen(s) + 1, s);
    }
    else if (pSockAddr->sa_family == AF_INET6) {
        CHAR s[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(((struct sockaddr_in6*)pSockAddr)->sin6_addr), s, INET6_ADDRSTRLEN);
        string = new CHAR[strlen(s) + 1];
        strcpy_s(string, strlen(s) + 1, s);
    }
    else {
        string = new CHAR[1];
        string[0] = '\0';
    }
    return string;
}

LPSTR Functions::FormatStringPSOCKADDR(IN LPCSTR message, IN PSOCKADDR pSockAddr)
{
    CHAR* ipstring = PSOCKADDRtoString(pSockAddr);
    size_t len = strlen(message) + strlen(ipstring) + 2; // 1 for space and 1 for null terminator
    LPSTR result = new CHAR[len];
    sprintf_s(result, len, "%s %s", message, ipstring);
    delete[] ipstring;
    return result;
}

/**
 *
 * Debug logging function
 *
 */

VOID Functions::WriteFileLogMessage(IN LPCSTR szMsg)
{
    CHAR msg[256];
    sprintf_s(msg, sizeof(msg), "[CGeoIPModule]: %s", szMsg);
    OutputDebugStringA(msg);
    // Get system drive letter
    LPSTR sysDrive = nullptr;
    size_t len = 0;
    if (_dupenv_s(&sysDrive, &len, "SystemDrive") != 0 || sysDrive == nullptr) {
		OutputDebugStringA("[CGeoIPModule]: [Functions::WriteFileLogMessage] Failed to get SystemDrive environment variable");
        return;
    }

    CHAR path[MAX_PATH] = { 0 };
    snprintf(path, sizeof(path), "%s/inetpub/logs/CGeoIPDebugLog/Module.log", sysDrive);

    free(sysDrive);

    // Open file for writing
    HANDLE hFile = CreateFileA(
        path,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
		sprintf_s(msg, sizeof(msg), "[CGeoIPModule]: [Functions::WriteFileLogMessage] Failed to open log file at path: %s", path);
		OutputDebugStringA(msg);
        return;
    }

    // date
    std::time_t currentTime = std::time(nullptr);
    CHAR buffer[80];
    std::tm localTime;
    localtime_s(&localTime, &currentTime);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);

    DWORD dwWritten;
    SetFilePointer(hFile, 0, NULL, FILE_END);
    CHAR finalMsg[512];

    sprintf_s(finalMsg, sizeof(finalMsg),
        "%s %s\r\n",
        buffer,  // timestamp
        szMsg
    );
    WriteFile(hFile, finalMsg, (DWORD)strlen(finalMsg), &dwWritten, NULL);
    CloseHandle(hFile);
    return;
}

#endif
