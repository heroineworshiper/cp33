#include "guicast.h"
#include "reader.inc"





class LoadFileWindow : public BC_FileBox
{
public:
	LoadFileWindow(int x, int y, char *path);
};


class LoadFileThread : public BC_DialogThread
{
public:
	LoadFileThread();
    BC_Window* new_gui();
    void handle_done_event(int result);
    LoadFileWindow *gui;
};


class PaletteButton : public BC_Button
{
public:
    PaletteButton(int x, 
        int y, 
        int is_top, 
        int color, 
        VFrame **data);
    int handle_event();
// index of color in table
    int color;
    int is_top;
};


class PaletteWindow : public BC_Popup
{
public:
	PaletteWindow(int x, 
        int y, 
        int w, 
        int h, 
        BC_Pixmap *bg_pixmap);
    ~PaletteWindow();
    void create_objects(int is_top);
    int button_press_event();
    int button_release_event();
    int cursor_motion_event();
    
    static PaletteWindow *palette_window;
};


class PaletteThread : public BC_DialogThread
{
public:
	PaletteThread();
    BC_Window* new_gui();
    void handle_done_event(int result);
    PaletteWindow *gui;
};

class LoadFile : public BC_Button
{
public:
    LoadFile(int x, int y);
    int handle_event();
};

class Save : public BC_Button
{
public:
    Save(int x, int y);
    int handle_event();
};

class Undo : public BC_Button
{
public:
    Undo(int x, int y);
    int handle_event();
};


class Redo : public BC_Button
{
public:
    Redo(int x, int y);
    int handle_event();
};



class PrevPage : public BC_Button
{
public:
    PrevPage(int x, int y);
    int handle_event();
};


class NextPage : public BC_Button
{
public:
    NextPage(int x, int y);
    int handle_event();
};


class Draw : public BC_Toggle
{
public:
    Draw(int x, int y);
    int handle_event();
};

class Erase : public BC_Toggle
{
public:
    Erase(int x, int y);
    int handle_event();
};

class Hollow : public BC_Button
{
public:
    Hollow(int x, int y);
    int handle_event();
};

class Filled : public BC_Button
{
public:
    Filled(int x, int y);
    int handle_event();
};

class Top : public BC_Button
{
public:
    Top(int x, int y);
    int handle_event();
};

class Bottom : public BC_Button
{
public:
    Bottom(int x, int y);
    int handle_event();
};

class TopColor : public BC_Button
{
public:
    TopColor(int x, int y);
    int handle_event();
};

class BottomColor : public BC_Button
{
public:
    BottomColor(int x, int y);
    int handle_event();
};


class BrushSize : public BC_TumbleTextBox
{
public:
    BrushSize(int x, int y, int w);
    int handle_event();
};


class MenuWindow : public BC_Window
{
public:
    MenuWindow();
    ~MenuWindow();

    void create_objects();
    int close_event();
    void show_palette(int top);
    void hide_palette();
// hide all the child windows of this window
    void hide_windows(int lock_it);
    void update_color();
    void update_colors();
    void update_buttons();

    Draw *draw;
    Erase *erase;
    Hollow *hollow;
    Filled *filled;
    Top *top;
    Bottom *bottom;
    BrushSize *size;
    TopColor *top_color;
    BottomColor *bottom_color;
// background for palette
    BC_Pixmap *bg_pixmap;
    static MenuWindow *menu_window;
};



class MenuThread: public Thread
{
public:
    MenuThread();
    void create_objects();
    void run();
    MenuWindow *gui;
};




