#ifndef _SFO_PARSER_H_
#define _SFO_PARSER_H_

#include <psptypes.h>

/**
 * @brief Reads a key string from a standalone .SFO file.
 */
int sfo_read_string(const char *path, const char *target_key, char *out,
                    int out_max);

/**
 * @brief Reads a key string from an .SFO embedded within a .PBP file.
 */
int pbp_read_sfo_string(const char *path, const char *target_key, char *out,
                        int out_max);

#endif // _SFO_PARSER_H_
