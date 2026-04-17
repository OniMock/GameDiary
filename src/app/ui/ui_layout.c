/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

/**
 * @file ui_layout.c
 * @brief UI layout implementation.
 */

#include "app/ui/ui_layout.h"

Rect rect_padding(Rect r, int p) {
    Rect res = {r.x + p, r.y + p, r.w - 2 * p, r.h - 2 * p};
    return res;
}

Rect rect_center(Rect parent, int w, int h) {
    Rect res = {
        parent.x + (parent.w - w) / 2,
        parent.y + (parent.h - h) / 2,
        w, h
    };
    return res;
}

Rect rect_column(Rect parent, int index, int total, int gap) {
    int h = (parent.h - (total - 1) * gap) / total;
    Rect res = {
        parent.x,
        parent.y + index * (h + gap),
        parent.w,
        h
    };
    return res;
}

Rect rect_row(Rect parent, int index, int total, int gap) {
    int w = (parent.w - (total - 1) * gap) / total;
    Rect res = {
        parent.x + index * (w + gap),
        parent.y,
        w,
        parent.h
    };
    return res;
}
