/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_DATA_BACKUP_H
#define GAMEDIARY_DATA_BACKUP_H

/**
 * @file data_backup.h
 * @brief Exports and Imports the database to/from a standard JSON format.
 */

/**
 * @brief Exports current in-memory database to ms0:/PSP/COMMON/GameDiary/backup.json
 * @return 0 on success, negative on error.
 */
int data_backup_export(void);

/**
 * @brief Imports from ms0:/PSP/COMMON/GameDiary/backup.json
 *        Overwrites existing games.dat and sessions.dat.
 * @return 0 on success, negative on error.
 */
int data_backup_import(void);

#endif // GAMEDIARY_DATA_BACKUP_H
