#ifndef GAMEDIARY_UI_LAYOUT_H
#define GAMEDIARY_UI_LAYOUT_H

typedef struct {
    int x, y, w, h;
} Rect;

/**
 * Creates a Rect with padding.
 */
Rect rect_padding(Rect r, int p);

/**
 * Creates a Rect centered within a parent Rect.
 */
Rect rect_center(Rect parent, int w, int h);

/**
 * Splits a Rect into a vertical column at a specific index.
 */
Rect rect_column(Rect parent, int index, int total, int gap);

/**
 * Splits a Rect into a horizontal row at a specific index.
 */
Rect rect_row(Rect parent, int index, int total, int gap);

#endif // GAMEDIARY_UI_LAYOUT_H
