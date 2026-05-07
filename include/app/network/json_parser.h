/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_JSON_PARSER_H
#define GAMEDIARY_JSON_PARSER_H

#include <stddef.h>

/**
 * @file json_parser.h
 * @brief Lightweight JSON parsing strictly for reading versions.
 */

typedef struct {
    char version[32];
    char release_date[32];
    char url[256];
    int mandatory;
} NetworkVersionInfo;

/**
 * @brief Parses version, date, url, and mandatory flags from a JSON payload.
 * 
 * @param json_buf The null-terminated JSON buffer.
 * @param out_info Pointer to the struct holding the parsed info.
 * @return 0 on success, -1 on failure (e.g. "version" field missing).
 */
int json_parse_version_info(const char* json_buf, NetworkVersionInfo* out_info);

#endif // GAMEDIARY_JSON_PARSER_H
