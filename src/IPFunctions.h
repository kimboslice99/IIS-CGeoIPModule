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
#include <string>
#include <httpserv.h>
#include <vector>
#include <cstdlib> // For atoi
#include <stdexcept> // For std::stoi
#include "RulesStruct.h"
#include <chrono>
#include <cstring>
#include <WS2tcpip.h>

#pragma comment(lib, "WS2_32")

/// <summary>
/// provides methods for handling addresses
/// </summary>
class IPFunctions
{
public:
    static BOOL IsLocalAddress(PSOCKADDR pSockAddr);

    static BOOL isIpInExceptionRules(PSOCKADDR pSockAddr, const std::vector<ExceptionRules>& rules, BOOL* pAllowed);

    static HRESULT GetIpVersion(IN PCSTR ipAddress, OUT INT* pFamily);

    static HRESULT StringToPSOCK(IN IHttpContext* pHttpContext, IN PCSTR string, IN INT family, OUT PSOCKADDR* ppOutAddr);

private:
    static VOID GenerateIpv6Mask(int prefixLength, struct in6_addr* mask);

    static BOOL IsIpv6InSubnet(struct in6_addr* addr, struct in6_addr* subnet, struct in6_addr* mask);

    static BOOL IsIpv4InSubnet(DWORD ip, DWORD subnet, DWORD mask);
};