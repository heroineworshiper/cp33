#ifndef READER_H
#define READER_H



#include "reader.inc"





class LoadFileWindow : public BC_FileBox
{
public:
	LoadFileWindow(ManeWindow *mwindow, int x, int y, char *path);

	ManeWindow *mwindow;
};


class LoadFileThread : public BC_DialogThread
{
public:
	LoadFileThread(ManeWindow *mwindow);
    BC_Window* new_gui();
    void handle_done_event(int result);
	ManeWindow *mwindow;
    LoadFileWindow *gui;
};

class LoadFile : public BC_Button
{
public:
    LoadFile(ManeWindow *mwindow, int x, int y);
    int handle_event();
    ManeWindow *mwindow;
};

class MenuWindow : public BC_Window
{
public:
    MenuWindow(ManeWindow *mwindow);
    void create_objects();
    int close_event();


    ManeWindow *mwindow;
};



class MenuThread: public Thread
{
public:
    MenuThread(ManeWindow *mwindow);
    void create_objects();
    void run();
    ManeWindow *mwindow;
    MenuWindow *gui;
};

class ManeWindow : public BC_Window
{
public:
    ManeWindow();
    
    void create_objects();
    void show_page(int number);
    int keypress_event();
    int button_release_event();
    void save_defaults();
    void show_error(char *text);

    int root_w;
    int root_h;
    MenuThread *menu;
    LoadFileThread *load;
    ReaderTheme *theme;
    BC_Hash *defaults;
};




#endif






