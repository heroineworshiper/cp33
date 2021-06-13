#ifndef READERTHEME_H
#define READERTHEME_H


#include "guicast.h"


class ReaderTheme : public BC_Theme
{
public:
    ReaderTheme();
    void initialize();
    
    int margin;
};





#endif

