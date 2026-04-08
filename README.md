# CGeoIPModule
 _Geoblocking module for IIS written in native C++_

 Response options include
 - Close Connection (Politely close the connection without sending a response) *
 - Reset Connection (Hang up rudely without sending a response) *  
 _* These options are connection level, HTTP clients will see this as an error.  
 This is the least expensive type of denial, however_
 - Not Found (404)
 - Forbidden (403)
 - Unauthorized (401)
 - Gone (410)
 - I'm a Teapot (418)
 - Enhance Your Calm (420)
 - Unavailable For Legal Reasons (451)

## Installation

Grab the latest installer from the releases page

Server variable `GEOIP_COUNTRY` will contain the iso country code on successful lookup, `ZZ` for local addresses as defined by the `IsLocalAddress()` function or `--` for unsuccessful lookups

<figure>
  <figcaption>Configuration page</figcaption>
  <img alt="Configuration page" src="https://github.com/kimboslice99/IIS-CGeoIPModule/blob/dev/cgeoipmodule-ui.png" width="240">
</figure>

## Bombardier results
1.39 KB static file for testing (small to emphasize transactional throughput)
***
No module enabled
```
Statistics        Avg      Stdev        Max
  Reqs/sec     13327.21    6434.57   31616.04
  Latency        9.37ms     2.08ms    53.62ms
  HTTP codes:
    1xx - 0, 2xx - 133322, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:    22.34MB/s
```
***
C# module
```
Statistics        Avg      Stdev        Max
  Reqs/sec       842.83     144.49    1301.78
  Latency      147.69ms    10.07ms   252.26ms
  HTTP codes:
    1xx - 0, 2xx - 8518, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:     1.41MB/s
```
***
C++ module  
<sup><sub>_This is probably even faster now_</sub></sup>
```
Statistics        Avg      Stdev        Max
  Reqs/sec      9776.10     200.54   10107.25
  Latency       12.77ms     0.95ms    94.00ms
  HTTP codes:
    1xx - 0, 2xx - 97873, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:    16.84MB/s
```

## Building

- Get libmaxminddb
```cmd
# Clone the libmaxminddb repository with submodules
git clone --recursive https://github.com/maxmind/libmaxminddb

# Navigate to the libmaxminddb directory
cd libmaxminddb

# Create build directories for both x64 and x86 architectures
mkdir build/x64 build/x86

# Configure and build the x64 version
cmake -Bbuild/x64 -S. -DMSVC_STATIC_RUNTIME=ON -DBUILD_SHARED_LIBS=OFF -A x64
cmake --build build/x64 --config Release --target install

# Configure and build the x86 version
cmake -Bbuild/x86 -S. -DMSVC_STATIC_RUNTIME=ON -DBUILD_SHARED_LIBS=OFF -A Win32
cmake --build build/x86 --config Release --target install
```

- Download and install Windows SDK.

- Get [Wix](https://github.com/wixtoolset/wix3/releases)

- Get [Wix Extension for VS2022](https://marketplace.visualstudio.com/items?itemName=WixToolset.WixToolsetVisualStudio2022Extension)

- Build project with Visual Studio

## Reason for this modules existence
I have been using the [GeoIP2blockModule](https://github.com/RvdHout/IIS-GeoIP2block-Module) for several years, and while its a well written solution, it is a bit slower (probably due to the introduction of asp.net into the request pipeline)

This module has much lower impact on IIS's performance, making it more suitable to sites that see heavy load.
