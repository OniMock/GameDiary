/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _PLUGIN_OVERLAY_NOTIFICATION_H_
#define _PLUGIN_OVERLAY_NOTIFICATION_H_

#include <psptypes.h>

void overlay_notification_init(void);
void overlay_notification_shutdown(void);

typedef enum {
  OVERLAY_NOTIFY_NEUTRAL = 0,
  OVERLAY_NOTIFY_TRACKER_ON,
  OVERLAY_NOTIFY_TRACKER_OFF
} OverlayNotifyStyle;

void overlay_notification_show(const char *line1, const char *line2,
                               OverlayNotifyStyle style);

/** @brief True while the on-screen message should be shown. */
int overlay_notification_is_visible(u32 now_ms);

/** @brief Redraw text on the current framebuffer (call once per vblank). */
void overlay_notification_draw(void);

#endif /* _PLUGIN_OVERLAY_NOTIFICATION_H_ */
