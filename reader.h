#ifndef READER_H
#define READER_H


#include "guicast.h"
#include "reader.inc"



// enable 2 page mode using the client
#ifndef USE_WINDOW
#define TWO_PAGE
#endif // USE_WINDOW

#define ROOT_W 768
#define ROOT_H 1366


// use double buffering
#define BG_PIXMAP

#define CLIENT_ADDRESS "pi2"
#define CLIENT_PORT 1234
#define PACKET_SIZE 1024
// timeout in ms
//#define TIMEOUT 1000
#define TIMEOUT 1000
#define RETRIES 5

// command IDs
#define LOAD_FILE 0
#define SHOW_PAGE 1

// button GPIOs
#define UP_GPIO 14
#define DOWN_GPIO 13
#define DEBOUNCE 1
#define BUTTON_HZ 60
#define REPEAT1 (BUTTON_HZ / 2)
// as fast as the drawing routine
#define REPEAT2 (1)



int load_file(char *path);
void load_file_entry(char *path);
int send_command(int id, uint8_t *value, int value_size);
int wait_command();
void prev_page();
void next_page();

class MWindow : public BC_Window
{
public:
    MWindow();
    
    void create_objects();
    void show_page(int number);
    int keypress_event();
    int button_release_event();
    void save_defaults();
    void show_error(char *text);

    int root_w;
    int root_h;
    int current_page;
// the current layer
    int is_top;
// index of color in table
    uint32_t top_color;
    uint32_t bottom_color;
    int brush_size;
    int is_hollow;
    
    
    MenuThread *menu;
    LoadFileThread *load;
    ReaderTheme *theme;
    BC_Hash *defaults;
    static MWindow *mwindow;
    
    static const uint32_t top_colors[TOTAL_COLORS];
    static const uint32_t bottom_colors[TOTAL_COLORS];
};




#endif






