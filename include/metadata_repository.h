#ifndef _METADATA_REPOSITORY_H_
#define _METADATA_REPOSITORY_H_

#include "models.h"

/**
 * @brief Repository that abstracts metadata retrieval for different game types.
 */

/**
 * @brief Fills the GameMetadata struct for the current environment.
 * @param metadata Pointer to the struct to fill.
 * @return int 1 if successful, 0 otherwise.
 */
int metadata_fetch(GameMetadata *metadata);

/**
 * @brief Fills the GameMetadata struct from a UMD disc (late fetch).
 * @param metadata Pointer to the struct to fill.
 * @return int 1 if successful, 0 otherwise.
 */
int metadata_fetch_from_umd(GameMetadata *metadata);

#endif // _METADATA_REPOSITORY_H_
