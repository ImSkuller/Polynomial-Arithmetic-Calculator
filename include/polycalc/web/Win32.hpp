#pragma once

// Single choke point for every Windows/Winsock header this app needs.
// winsock2.h must be included before windows.h (or windows.h drags in the
// obsolete winsock.h and the two collide) - every other header in web/
// includes this instead of windows.h directly so that ordering never has to
// be re-derived at each call site.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
