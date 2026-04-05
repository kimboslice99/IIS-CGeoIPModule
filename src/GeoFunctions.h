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
#include "Functions.h"
#include <httpserv.h>
#include <comdef.h> // for _com_error and _bstr_t
#include <maxminddb.h>
#include <atomic>

#pragma comment(lib, "maxminddb.lib")

extern std::atomic<bool> g_reloadNeeded;
extern MMDB_s g_mmdb;

/// <summary>
/// This class provides methods for interacting with the GeoIP database.
/// </summary>
class GeoFunctions
{
public:
    CHAR* GetMMDBPath(IN IHttpContext* pHttpContext, IN IAppHostElement* pModuleElement);

    HRESULT LoadMMDB(IN IHttpContext* pHttpContext, IN IAppHostElement* pModuleElement);

    VOID UnloadMMDB();

    HRESULT GetCountryCode(IN PSOCKADDR IP, OUT CHAR* COUNTRYCODE);
};
