#ifndef READERTHEME_H
#define READERTHEME_H


#include "guicast.h"
#include "reader.inc"

class ReaderTheme : public BC_Theme
{
public:
    ReaderTheme();
    void initialize();
    void fill_box(VFrame *dst, uint32_t color);

    int margin;
// button graphics for each color
    VFrame **top_colors[TOTAL_COLORS];
    VFrame **bottom_colors[TOTAL_COLORS];
};





#endif

