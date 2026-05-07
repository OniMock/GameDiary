/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_HTTP_CLIENT_H
#define GAMEDIARY_HTTP_CLIENT_H

#include <stddef.h>

/**
 * @file http_client.h
 * @brief Simple HTTP client fetching for version checking.
 *
 * Includes timeout limits and cancellation flags using purely `sceHttp`.
 */

// The IP is injected via Makefile to avoid hardcoding secrets.
// Use: make VERSION_URL="\"http://your-ip/version.json\""
#ifndef VERSION_ENDPOINT
#define VERSION_ENDPOINT "http://127.0.0.1/version.json"
#endif

// Set true to cancel any ongoing reading/establishing connection
extern volatile int g_http_cancel;

/**
 * @brief Fetches the version string from the endpoint.
 *
 * @param out_buf Buffer to store the response text.
 * @param buf_size Maximum capacity of the buffer.
 * @return 0 on success, negative value on error.
 */
int http_client_fetch_version(char* out_buf, size_t buf_size);

#endif // GAMEDIARY_HTTP_CLIENT_H
