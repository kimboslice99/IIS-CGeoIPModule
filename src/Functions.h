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
#define WIN32_LEAN_AND_MEAN
#pragma once
#include <httpserv.h>
#include <vector>
#include <string>
#include <chrono>
#include <comdef.h> // for _com_error and _bstr_t
#include <atlbase.h>  // For CComPtr
#ifdef _DEBUG
#include <atlstr.h>
#endif
#include "RulesStruct.h"

extern IHttpServer* g_pHttpServer;

/// <summary>
/// This class provides methods for retreiving configuration items
/// </summary>
class Functions
{
public:
    static HRESULT GetConfig(IN IHttpContext* pHttpContext, OUT IAppHostElement** ppElement);

    HRESULT GetStringPropertyValueFromElement(IAppHostElement* pElement, BSTR pszElementName, BSTR* pStringValue);

    HRESULT GetBooleanPropertyValueFromElement(IAppHostElement* pElement, BSTR pszElementName, BOOL* pBoolValue);

    BOOL IsCountryCodeListed(IN IHttpContext* pHttpContext, IN BSTR CountryCode, IN IAppHostElement* pModuleElement);

    BOOL GetIsEnabled(IN IAppHostElement* pModuleElement);

    HRESULT GetSiteId(IN IHttpContext* pHttpContext, OUT PCWSTR* str);

    BOOL CheckRemoteAddr(IN IAppHostElement* pModuleElement);

    BOOL CheckCountryCode(IN IHttpContext* pHttpContext, IN CHAR* COUNTRYCODE, IN BOOL MODE, IN IAppHostElement* pModuleElement);

    VOID DenyAction(IN IHttpContext* pHttpContext, IN IAppHostElement* pModuleElement);

    BOOL GetAllowMode(IN IAppHostElement* pModuleElement);

    std::vector<ExceptionRules> exceptionRules(IN IHttpContext* pHttpContext, IN IAppHostElement* pModuleElement);

    LPWSTR charToWString(IN IHttpContext* pHttpContext, IN LPCSTR charArray, IN INT length);

    LPSTR BSTRToCharArray(IN IHttpContext* pHttpContext, IN BSTR bstr);

#ifdef _DEBUG
    LPSTR PSOCKADDRtoString(IN PSOCKADDR pSockAddr);

    LPSTR FormatStringPSOCKADDR(IN LPCSTR string, IN PSOCKADDR pSockAddr);

    static VOID WriteFileLogMessage(IN LPCSTR szMsg);
#endif

};
