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
#include "IPFunctions.h"
#include <Windows.h>

BOOL IPFunctions::IsIpv4InSubnet(DWORD ip, DWORD subnet, DWORD mask)
{
    return (ip & mask) == (subnet & mask);
}

BOOL IPFunctions::IsIpv6InSubnet(struct in6_addr* addr, struct in6_addr* subnet, struct in6_addr* mask)
{
    for (int i = 0; i < 16; ++i) {
        if ((addr->s6_addr[i] & mask->s6_addr[i]) != (subnet->s6_addr[i] & mask->s6_addr[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

// Function to generate an IPv6 mask based on the prefix length
VOID IPFunctions::GenerateIpv6Mask(int prefixLength, struct in6_addr* mask)
{
    std::memset(mask, 0, sizeof(struct in6_addr)); // Initialize the mask to all zeros

    for (int i = 0; i < prefixLength / 8; ++i) {
        mask->s6_addr[i] = 0xFF;
    }
    if (prefixLength % 8 != 0) {
        mask->s6_addr[prefixLength / 8] = static_cast<uint8_t>(0xFF << (8 - (prefixLength % 8)));
    }
}

/// <summary>
/// Local addresses
/// 192.168.0.0/16
/// 10.0.0.0/8
/// 0.0.0.0/8
/// 172.16.0.0/12
/// 127.0.0.0/8
/// fe80::/10
/// fc00::/7
/// ::1/128
/// </summary>
/// <param name="pSockAddr"></param>
/// <returns>true if local</returns>
BOOL IPFunctions::IsLocalAddress(PSOCKADDR pSockAddr) {
    if (pSockAddr->sa_family == AF_INET) {
        SOCKADDR_IN* sockaddr_in = (struct sockaddr_in*)pSockAddr;
        DWORD clientIp = sockaddr_in->sin_addr.S_un.S_addr;

        // List of IPv4 local subnets and masks
        struct {
            const char* subnet;
            const char* mask;
        } ipv4_local_subnets[] = {
            {"192.168.0.0", "255.255.0.0"},
            {"10.0.0.0", "255.0.0.0"},
            {"0.0.0.0", "255.0.0.0"},
            {"172.16.0.0", "255.240.0.0"},
            {"127.0.0.0", "255.0.0.0"}
        };

        // Check each local subnets
        for (const auto& net : ipv4_local_subnets) {
            struct in_addr ipv4_subnet = { 0 }, ipv4_mask = { 0 };
            inet_pton(AF_INET, net.subnet, &ipv4_subnet);
            inet_pton(AF_INET, net.mask, &ipv4_mask);

            DWORD subnet = ipv4_subnet.S_un.S_addr;
            DWORD mask = ipv4_mask.S_un.S_addr;

            if (IsIpv4InSubnet(clientIp, subnet, mask)) {
                return TRUE;
            }
        }
    }
    else if (pSockAddr->sa_family == AF_INET6) {
        SOCKADDR_IN6* sockaddr_in6 = (struct sockaddr_in6*)pSockAddr;
        struct in6_addr ipv6_subnet = {}, ipv6_mask = {};

        // Check for link-local address (fe80::/10)
        inet_pton(AF_INET6, "fe80::", &ipv6_subnet);
        GenerateIpv6Mask(10, &ipv6_mask);
        if (IsIpv6InSubnet(&sockaddr_in6->sin6_addr, &ipv6_subnet, &ipv6_mask)) {
            return TRUE;
        }

        ipv6_subnet = {}, ipv6_mask = {};
        // Check for unique local address (fc00::/7)
        inet_pton(AF_INET6, "fc00::", &ipv6_subnet);
        GenerateIpv6Mask(7, &ipv6_mask);
        if (IsIpv6InSubnet(&sockaddr_in6->sin6_addr, &ipv6_subnet, &ipv6_mask)) {
            return TRUE;
        }

        // Check for IPv6 localhost (::1)
        struct in6_addr ipv6_localhost = {};
        inet_pton(AF_INET6, "::1", &ipv6_localhost);
        if (memcmp(&sockaddr_in6->sin6_addr, &ipv6_localhost, sizeof(struct in6_addr)) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL IPFunctions::isIpInExceptionRules(PSOCKADDR pSockAddr, const std::vector<ExceptionRules>& rules, BOOL* pAllowed)
{
    for (const auto& rule : rules) {
        PCSTR family = rule.family;
        PCSTR address = rule.address;
        PCSTR mask = rule.mask;
        *pAllowed = rule.mode;

        if (pSockAddr->sa_family == AF_INET && strcmp(family, "ipv4") == 0) {
            SOCKADDR_IN* sockaddr_in = (struct sockaddr_in*)pSockAddr;
            DWORD clientIp = sockaddr_in->sin_addr.S_un.S_addr;

            struct in_addr ipv4_subnet = { 0 }, ipv4_mask = { 0 };
            inet_pton(AF_INET, address, &ipv4_subnet);
            inet_pton(AF_INET, mask, &ipv4_mask);

            DWORD subnet = ipv4_subnet.S_un.S_addr;
            DWORD mask = ipv4_mask.S_un.S_addr;

            if (IsIpv4InSubnet(clientIp, subnet, mask)) {
                return TRUE;
            }
        }

        if (pSockAddr->sa_family == AF_INET6 && strcmp(family, "ipv6") == 0) {
            // Convert mask string to an integer
            int maskInt = 0;
            std::exception e;
            try {
                maskInt = std::stoi(mask);
            }
            catch (const std::invalid_argument&) {
                continue;
            }
            catch (const std::out_of_range&) {
                continue;
            }

            SOCKADDR_IN6* sockaddr_in6 = (struct sockaddr_in6*)pSockAddr;
            struct in6_addr ipv6_subnet = {}, ipv6_mask = {};

            inet_pton(AF_INET6, address, &ipv6_subnet);
            GenerateIpv6Mask(maskInt, &ipv6_mask);
            if (IsIpv6InSubnet(&sockaddr_in6->sin6_addr, &ipv6_subnet, &ipv6_mask)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

HRESULT IPFunctions::GetIpVersion(IN PCSTR ipAddress, OUT INT* pFamily)
{
    struct sockaddr_in sa;

    if (inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) == 1) {
        *pFamily = AF_INET;
        return S_OK;
    }

    struct sockaddr_in6 sa6;

    if (inet_pton(AF_INET6, ipAddress, &(sa6.sin6_addr)) == 1) {
        *pFamily = AF_INET6;
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT IPFunctions::StringToPSOCK(IN IHttpContext* pHttpContext, IN PCSTR string, IN INT family, OUT PSOCKADDR* ppOutAddr)
{
    *ppOutAddr = nullptr;

    if (string == nullptr || pHttpContext == nullptr || ppOutAddr == nullptr) {
        return E_INVALIDARG;
    }

    if (family == AF_INET) {
        PSOCKADDR pSockAddr = (PSOCKADDR)pHttpContext->AllocateRequestMemory(sizeof(struct sockaddr_in));

        if (pSockAddr == nullptr) {
            return E_OUTOFMEMORY;
        }

        // Zero out the structure.
        ZeroMemory(pSockAddr, sizeof(struct sockaddr_in));

        ((struct sockaddr_in*)pSockAddr)->sin_family = AF_INET;

        // Convert the IP address string into the sockaddr_in structure
        if (inet_pton(AF_INET, string, &(((struct sockaddr_in*)pSockAddr)->sin_addr)) != 1) {
            return E_FAIL;
        }
        *ppOutAddr = pSockAddr;
        return S_OK;
    }
    else if (family == AF_INET6) {
        PSOCKADDR pSockAddr = (PSOCKADDR)pHttpContext->AllocateRequestMemory(sizeof(struct sockaddr_in6));
        if (pSockAddr == nullptr) {
            return E_OUTOFMEMORY;
        }

        ZeroMemory(pSockAddr, sizeof(struct sockaddr_in6));
        ((struct sockaddr_in6*)pSockAddr)->sin6_family = AF_INET6;

        if (inet_pton(AF_INET6, string, &(((struct sockaddr_in6*)pSockAddr)->sin6_addr)) != 1) {
            return E_FAIL;
        }
        *ppOutAddr = pSockAddr;
        return S_OK;
    }

    return E_INVALIDARG;
}