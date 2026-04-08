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
#include "pch.h"
#include <sal.h>
#include <httpserv.h>
#include <windows.h>
#include <atomic>
#include <shared_mutex>
#include "RulesStruct.h"
#include "Functions.h"
#include "IPFunctions.h"
#include "GeoFunctions.h"

IHttpServer* g_pHttpServer = NULL;
PVOID g_pModuleContext = NULL;
MMDB_s g_mmdb;
GeoFunctions geoFunctions;
std::atomic<bool> g_reloadNeeded = false;
std::atomic<bool> isInitialized = false;
std::shared_mutex initMutex;

// Create the module class.
class CGeoIPModule : public CHttpModule
{
public:

    REQUEST_NOTIFICATION_STATUS
        OnBeginRequest(
            IN IHttpContext* pHttpContext,
            IN IHttpEventProvider* pProvider
        )
    {
        IHttpRequest* pHttpRequest = pHttpContext->GetRequest();
        PSOCKADDR pSockAddr = pHttpRequest->GetRemoteAddress();
        Functions myFunctions;

        IAppHostElement* pModuleElement = nullptr;
        HRESULT hr = myFunctions.GetConfig(pHttpContext, &pModuleElement);
        if (FAILED(hr)) {
#ifdef _DEBUG
            CHAR message[256];
            _com_error err(hr);
            sprintf_s(message, sizeof(message), "Failed to retreive configuration. The following error occured. %s", CStringA(err.ErrorMessage()).GetString());
            myFunctions.WriteFileLogMessage(message);
#endif
            pProvider->SetErrorStatus(hr);
            myFunctions.DenyAction(pHttpContext, NULL);
            return RQ_NOTIFICATION_FINISH_REQUEST;
        }

        if (!myFunctions.GetIsEnabled(pModuleElement)) {
#ifdef _DEBUG
            myFunctions.WriteFileLogMessage("Module disabled");
#endif
            pModuleElement->Release();
            return RQ_NOTIFICATION_CONTINUE;
        }

        // Parse REMOTE_ADDR to sockaddr.
        if (myFunctions.CheckRemoteAddr(pModuleElement)) {
            DWORD val;
            PCWSTR remoteAddr;
            hr = pHttpContext->GetServerVariable("REMOTE_ADDR", &remoteAddr, &val);
            if (FAILED(hr)) {
#ifdef _DEBUG
                myFunctions.WriteFileLogMessage("Failed to retreive remote address");
#endif
                pProvider->SetErrorStatus(hr);
                pModuleElement->Release();
                return RQ_NOTIFICATION_FINISH_REQUEST;
            }

            _bstr_t b(remoteAddr);
            PCSTR pcstrRemoteAddr = b;
#ifdef _DEBUG
            CHAR message[128];
            sprintf_s(message, sizeof(message), "Parsing REMOTE_ADDR %s", pcstrRemoteAddr);
            myFunctions.WriteFileLogMessage(message);
#endif
            INT family;
            hr = IPFunctions::GetIpVersion(pcstrRemoteAddr, &family);
            if (FAILED(hr)) {
                pProvider->SetErrorStatus(hr);
                pModuleElement->Release();
                return RQ_NOTIFICATION_FINISH_REQUEST;
            }

            hr = IPFunctions::StringToPSOCK(pHttpContext, pcstrRemoteAddr, family, &pSockAddr);
            if (FAILED(hr)) {
                pProvider->SetErrorStatus(hr);
                pModuleElement->Release();
                return RQ_NOTIFICATION_FINISH_REQUEST;
            }

        }

        if (!isInitialized || g_reloadNeeded) {
            std::lock_guard<std::shared_mutex> lock(initMutex);
            if (!isInitialized || g_reloadNeeded) {
                // Load configuration from pHttpContext
                HRESULT hr = geoFunctions.LoadMMDB(pHttpContext, pModuleElement);
                if (FAILED(hr)) {
                    pProvider->SetErrorStatus(hr);
                    pModuleElement->Release();
                    return RQ_NOTIFICATION_FINISH_REQUEST;
                }
                isInitialized = true;
            }
        }

        // check exception rules
        BOOL allowed = FALSE;
        std::vector<ExceptionRules> rules;
        rules = myFunctions.exceptionRules(pHttpContext, pModuleElement);
        if (IPFunctions::isIpInExceptionRules(pSockAddr, rules, &allowed)) {
            if (TRUE == allowed) {
#ifdef _DEBUG
                myFunctions.WriteFileLogMessage("IP allowed by exception rule");
#endif
                pModuleElement->Release();
                return RQ_NOTIFICATION_CONTINUE;
            } else {
#ifdef _DEBUG
                myFunctions.WriteFileLogMessage("IP denied by exception rule");
#endif
                myFunctions.DenyAction(pHttpContext, pModuleElement);
                pModuleElement->Release();
                return RQ_NOTIFICATION_FINISH_REQUEST;
            }
        }

        CHAR countryCode[3] = { '\0' }; // Buffer to store the country code (2 characters + null terminator)

        if (IPFunctions::IsLocalAddress(pSockAddr)) {
            strcpy_s(countryCode, 3, "ZZ");
        }

        BOOL mode = myFunctions.GetAllowMode(pModuleElement);
        REQUEST_NOTIFICATION_STATUS reqStatus;

        // Get country code for this address, if it has not been set
        if (countryCode[0] == '\0') {
            geoFunctions.GetCountryCode(pSockAddr, countryCode);
        }
        LPCWSTR wCountryCode = myFunctions.charToWString(pHttpContext, countryCode, 3);
        pHttpContext->SetServerVariable("GEOIP_COUNTRY", wCountryCode);

        // check the retrieved country code
        if (myFunctions.CheckCountryCode(pHttpContext, countryCode, mode, pModuleElement)) {
#ifdef _DEBUG
            myFunctions.WriteFileLogMessage("CountryCode allowed");
#endif
            reqStatus = RQ_NOTIFICATION_CONTINUE;
        } else {
#ifdef _DEBUG
            myFunctions.WriteFileLogMessage("CountryCode denied");
#endif
            myFunctions.DenyAction(pHttpContext, pModuleElement);
            reqStatus = RQ_NOTIFICATION_FINISH_REQUEST;
        }

        pModuleElement->Release();
        return reqStatus;
    }
};

class MyHttpModuleFactory : public IHttpModuleFactory
{
public:
    HRESULT
        GetHttpModule(
            OUT CHttpModule** ppModule,
            IN IModuleAllocator* pAllocator
        )
    {
        UNREFERENCED_PARAMETER(pAllocator);
        CGeoIPModule* pModule = new CGeoIPModule;

        if (!pModule) {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        } else {
            *ppModule = pModule;
            pModule = NULL;
            return S_OK;
        }
    }

    void Terminate()
    {
        delete this;
        geoFunctions.UnloadMMDB();
    }
};

// Create the module's exported registration function.
HRESULT WINAPI RegisterModule(
    DWORD dwServerVersion,
    IHttpModuleRegistrationInfo* pModuleInfo,
    IHttpServer* pGlobalInfo
)
{
    UNREFERENCED_PARAMETER(dwServerVersion);

    g_pModuleContext = pModuleInfo->GetId();
    g_pHttpServer = pGlobalInfo;

    // Set the request notifications and exit.
    return pModuleInfo->SetRequestNotifications(
        // Specify the class factory.
        new MyHttpModuleFactory,
        // Specify the event notifications.
        RQ_BEGIN_REQUEST,
        // Specify the post-event notifications.
        0
    );
}