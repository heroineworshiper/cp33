/*
 * MUSIC READER
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */


// show music on 2 LCD panels

// step 1: import the text using the pdftoreader program
// ./pdftoreader /home/archive/scherzo4b.pdf 1 99 135,0 2292,3567 scherzo4
// ./pdftoreader /home/archive/Chopin_Scherzo_No.1\,_Op.20_Joseffy.pdf 1 99 135,0 2292,4000 scherzo1

// step 2: run this program as a controller
// export DISPLAY=:0
// ./reader scherzo4

// run this program as a client
// ./reader

// compile it on a raspberry:
// apt install xorg
// apt install libx11-dev
// apt install libxext-dev
// apt install libxv-dev
// apt install wiringpi
// make -f Makefile.reader
// scp reader pi2:

#include "clip.h"
#include "guicast.h"
#include "cursors.h"
#include "keys.h"
#include <stdlib.h>
#include <string.h>
#ifndef USE_WINDOW
#include <wiringPi.h>
#endif
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>


#include "reader.h"
#include "readermenu.h"
#include "readertheme.h"
typedef struct
{
    int accum;
    int time;
    int next_time;
    int value;
    int got_it;
} button_state_t;
button_state_t up_button;
button_state_t down_button;

// network
uint8_t packet[PACKET_SIZE];
uint8_t command[PACKET_SIZE];
int command_size;
int command_result;
sem_t command_ready;
sem_t command_done;
int client_mode = 0;


// framebuffer I/O
int fb_fd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int screensize = 0;
char *fbp = 0;




MWindow* MWindow::mwindow = 0;
const uint32_t MWindow::top_colors[TOTAL_COLORS] = 
{
    0x000000, // black
    0x808080, // grey
    0xff0000, // red
    0xcc44cc, // purple
    0x00ff00, // green
    0x0000ff, // blue
    0x664400, // brown
};

const uint32_t MWindow::bottom_colors[TOTAL_COLORS] = 
{
    0xeeee77, // yellow
    0xbbbbbb, // light grey
    0xff7777, // light red
    0xaaffee, // cyan
    0xaaff66, // light green
    0x0088ff, // light blue
    0xdd8855, // orange
};


class Page
{
public:
    Page()
    {
        x1 = y1 = x2 = y2 = 0;
        image = 0;
    }
    
    ~Page()
    {
        if(image)
        {
            delete image;
        }
    }
    
    int x1, y1, x2, y2;
    string path;
    VFrame *image;
};

ArrayList<Page*> pages;





MWindow::MWindow() : BC_Window()
{
    current_page = 0;

//#ifndef USE_WINDOW
//    set_border(0);
//#endif
};

    
void MWindow::create_objects()
{
	defaults = new BC_Hash("~/.readerrc");
	defaults->load();
    BC_WindowBase::load_defaults(defaults);
    
    is_top = defaults->get("IS_TOP", 0);
    is_hollow = defaults->get("IS_HOLLOW", 0);
    top_color = defaults->get("TOP_COLOR", 0);
    bottom_color = defaults->get("BOTTOM_COLOR", 0);
    brush_size = defaults->get("BRUSH_SIZE", 1);
    CLAMP(top_color, 0, TOTAL_COLORS - 1);
    CLAMP(bottom_color, 0, TOTAL_COLORS - 1);
    CLAMP(brush_size, 1, MAX_BRUSH);

    
    if(get_resources()->filebox_w > ROOT_W)
    {
        get_resources()->filebox_w = ROOT_W;
    }
    if(get_resources()->filebox_h > ROOT_H)
    {
        get_resources()->filebox_h = ROOT_H;
    }
    sprintf(get_resources()->filebox_filter, "*.reader");

    theme = new ReaderTheme;
    theme->initialize();
    get_resources()->bg_color = WHITE;

    create_window("Reader", // title
        0, // x
        0, // y
        ROOT_W, // w
        ROOT_H, // h
        -1, // min_w
        -1, // min_h
        0, // allow_resize
        1, // private_color
        1, // hide
        WHITE, // bg_color
        "", // display_name
        0); // group_it
    root_w = get_w();
    root_h = get_h();
    if(client_mode)
    {

        set_cursor(TRANSPARENT_CURSOR, 0, 0);
    }
// draw bitmaps to the foreground/win instead of the back buffer/pixmap
#ifndef BG_PIXMAP

    start_video();
#endif



    show_window(1);

    menu = new MenuThread;
    menu->create_objects();
    menu->start();
    
    load = new LoadFileThread;
//    printf("MWindow::create_objects %d color_model=%d\n", __LINE__, get_color_model());
}

void MWindow::save_defaults()
{
    BC_WindowBase::save_defaults(defaults);
    defaults->update("IS_TOP", is_top);
    defaults->update("IS_HOLLOW", is_hollow);
    defaults->update("TOP_COLOR", (int32_t)top_color);
    defaults->update("BOTTOM_COLOR", (int32_t)bottom_color);
    defaults->update("BRUSH_SIZE", (int32_t)brush_size);
    defaults->save();
}

void MWindow::show_error(char *text)
{
    lock_window();
    clear_box(0, 0, root_w, root_h, 0);
    set_color(BLACK);
    draw_text(theme->margin, 
        theme->margin + get_text_height(MEDIUMFONT), 
        text);
    flash();
    unlock_window();
}
void MWindow::show_page(int number)
{
    if(number >= pages.size())
    {
        lock_window();
        clear_box(0, 0, root_w, root_h, 0);
        flash();
        unlock_window();
        return;
    }


    Page *page = pages.get(number);
    lock_window();
    BC_Bitmap *bitmap = get_temp_bitmap(root_w,
        root_h,
        get_color_model());
//printf("MWindow::show_page %d color_model=%d\n", __LINE__, get_color_model());

// convert to display color model
    switch(get_color_model())
    {
        case BC_BGR8888:
            for(int i = 0; i < root_h; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i];
                uint8_t *dst_row = bitmap->get_row_pointers()[i];
                for(int j = 0; j < root_w; j++)
                {
                    uint8_t src_value = *src_row++;
                    uint8_t r = 0;
                    uint8_t g = 0;
                    uint8_t b = 0;
    // bit mask to RGB
                    if((src_value & 0x1))
                    {
                        r = 0xff;
                    }

                    if((src_value & 0x2))
                    {
                        g = 0xff;
                    }

                    if((src_value & 0x4))
                    {
                        b = 0xff;
                    }

                    dst_row[0] = b;
                    dst_row[1] = g;
                    dst_row[2] = r;
                    dst_row += 4;
                }
            }
            break;

        case BC_RGB565:
        {
            const uint16_t table[] =
            {
                0b0000000000000000,
                0b1111100000000000,
                0b0000011111100000,
                0b1111111111100000,
                0b0000000000011111,
                0b1111100000011111,
                0b0000011111111111,
                0b1111111111111111
            };
            for(int i = 0; i < root_h; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i];
                uint16_t *dst_row = (uint16_t*)bitmap->get_row_pointers()[i];
                for(int j = 0; j < root_w; j++)
                {
// bit mask to RGB
                    *dst_row++ = table[*src_row++];
                }
            }
            break;
        }

        case BC_RGB8:
        {
            const uint8_t table[] =
            {
                0b00000000,
                0b11000000,
                0b00111000,
                0b11111000,
                0b00000111,
                0b11000111,
                0b00111111,
                0b11111111
            };
            for(int i = 0; i < root_h; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i];
                uint8_t *dst_row = bitmap->get_row_pointers()[i];
                for(int j = 0; j < root_w; j++)
                {
// bit mask to RGB
                    *dst_row++ = table[*src_row++];
                }
            }
            break;
        }

        default:
            if(get_color_model() != BC_BGR8888)
            {
                printf("MWindow::show_page %d unknown color model %d\n",
                    __LINE__,
                    get_color_model());
            }
            break;
    }

    draw_bitmap(bitmap, 0);
#ifdef BG_PIXMAP
    flash();
#endif
    unlock_window();
}


int MWindow::keypress_event()
{
    switch(get_keypress())
    {
        case LEFT:
            prev_page();
            break;

        case RIGHT:
            next_page();
            break;

        case ESC:
            set_done(1);
            break;
    }

    return 1;
}

int MWindow::button_release_event()
{
//    set_cursor(ARROW_CURSOR, 0, 1);
    int x = get_abs_cursor_x(0);
    int y = get_abs_cursor_y(0);
    unlock_window();

    MenuWindow::menu_window->lock_window();
    if(x + MenuWindow::menu_window->get_w() > root_w)
    {
        x = root_w - MenuWindow::menu_window->get_w();
    }
    if(y + MenuWindow::menu_window->get_h() > root_h)
    {
        y = root_h - MenuWindow::menu_window->get_h();
    }
    if(x < 0)
    {
        x = 0;
    }

    if(y < 0)
    {
        y = 0;
    }
    MenuWindow::menu_window->unlock_window();


    if(load->is_running())
    {
        load->lock_gui("MWindow::button_release_event");
        load->gui->raise_window(1);
        load->unlock_gui();
    }
    else
    if(menu->gui->get_hidden())
    {
// show the menu
//printf("MWindow::button_release_event %d %d %d\n", __LINE__, x, y);
        
        menu->gui->lock_window();
        menu->gui->show_window();
// have to rehide buttons after show_window
        menu->gui->update_buttons();
// have to reposition after showing
        menu->gui->reposition_window(x, y);
        menu->gui->raise_window(1);
        menu->gui->unlock_window();
    }
    else
    {
//printf("MWindow::button_release_event %d\n", __LINE__);
// close all the windows
        MenuWindow::menu_window->lock_window();
        MenuWindow::menu_window->hide_windows(0);
//        MenuWindow::menu_window->reposition_window(x, y);
//        MenuWindow::menu_window->raise_window(1);
        MenuWindow::menu_window->unlock_window();

    }

    lock_window();
    return 1;
}


void next_page()
{
//    printf("next_page %d\n", __LINE__);
#ifdef TWO_PAGE
    if(MWindow::mwindow->current_page < pages.size() - 2)
    {
        MWindow::mwindow->current_page += 2;
        int temp = MWindow::mwindow->current_page + 1;
        send_command(SHOW_PAGE, (uint8_t*)&temp, sizeof(int));
        MWindow::mwindow->show_page(MWindow::mwindow->current_page);
        wait_command();
    }
#else
    if(MWindow::mwindow->current_page < pages.size() - 1)
    {
        MWindow::mwindow->current_page++;
        MWindow::mwindow->show_page(MWindow::mwindow->current_page);
    }
#endif
}

void prev_page()
{
//    printf("prev_page %d\n", __LINE__);
    if(MWindow::mwindow->current_page > 0)
    {
#ifdef TWO_PAGE
        MWindow::mwindow->current_page -= 2;
        int temp = MWindow::mwindow->current_page + 1;
        send_command(SHOW_PAGE, (uint8_t*)&temp, sizeof(int));
        MWindow::mwindow->show_page(MWindow::mwindow->current_page);
        wait_command();
#else
        MWindow::mwindow->current_page--;
        MWindow::mwindow->show_page(MWindow::mwindow->current_page);
#endif
    }
}

void update_button_state(button_state_t *ptr, int value)
{
    ptr->got_it = 0;
// button down
    if(value)
    {
        if(ptr->accum < DEBOUNCE)
        {
// wait
            ptr->accum++;
            ptr->time = 0;
            ptr->next_time = REPEAT1;
        }
        else
        if(!ptr->value)
        {
// propagate change
            ptr->value = 1;
// signal a change
            ptr->got_it = 1;
            ptr->time = 0;
            ptr->next_time = REPEAT1;
        }
    }
    else
    {
// button up
        if(ptr->accum > 0)
        {
// wait
            ptr->accum--;
            ptr->time = 0;
            ptr->next_time = REPEAT1;
        }
        else
        if(ptr->value)
        {
// propagate change
            ptr->value = 0;
// signal a change
            ptr->got_it = 1;
            ptr->time = 0;
            ptr->next_time = REPEAT1;
        }
    }

// advance timer
    if(ptr->accum >= DEBOUNCE)
    {
        ptr->time++;
    }
    else
    if(ptr->accum <= 0)
    {
        ptr->time++;
    }
}

#ifndef USE_WINDOW
void* button_thread(void *ptr)
{
    int ticks = 0;
    while(1)
    {
        usleep(1000000 / BUTTON_HZ);
        update_button_state(&up_button, !digitalRead(UP_GPIO));
        update_button_state(&down_button, !digitalRead(DOWN_GPIO));

//         printf("button_thread %d %d %d\n", 
//             __LINE__,
//             digitalRead(UP),
//             digitalRead(DOWN));
//         if(up_button.got_it)
//         {
//             printf("UP\n");
//         }
//         if(down_button.got_it)
//         {
//             printf("DOWN\n");
//         }
//         if(!(ticks % BUTTON_HZ))
//         {
//             printf("button_thread %d %d %d\n", 
//                 __LINE__,
//                 up_button.time,
//                 down_button.time);
//         }

        if(up_button.value)
        {
            if(up_button.got_it || up_button.time >= up_button.next_time)
            {
                if(up_button.time >= up_button.next_time)
                {
                    up_button.next_time = up_button.time + REPEAT2;
                }
                next_page();
            }
        }
        else
        if(down_button.value)
        {
            if(down_button.got_it || down_button.time >= down_button.next_time)
            {
                if(down_button.time >= down_button.next_time)
                {
                    down_button.next_time = down_button.time + REPEAT2;
                }
                prev_page();
            }
        }
        

        ticks++;
    }
}
#endif // !USE_WINDOW


void* client_thread(void *ptr)
{
// create read socket
    int i;
	int read_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in read_addr;
	read_addr.sin_family = AF_INET;
	read_addr.sin_port = htons((unsigned short)CLIENT_PORT);
	read_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(read_socket, 
		(struct sockaddr*)&read_addr, 
		sizeof(read_addr)) < 0)
	{
		perror("client_thread: bind");
        exit(1);
	}

    
    while(1)
    {
        socklen_t peer_addr_len = sizeof(struct sockaddr_in);
        struct sockaddr_in peer_addr;
        int bytes_read = recvfrom(read_socket,
            packet, 
            PACKET_SIZE, 
            0,
            (struct sockaddr *) &peer_addr, 
            &peer_addr_len);
        if(bytes_read < 0)
        {
            perror("client_thread: recvfrom");
            exit(1);
        }

//         printf("client_thread %d: got ", __LINE__);
//         for(i = 0; i < bytes_read; i++)
//         {
//             printf("%02x ", packet[i]);
//         }
//         printf("\n");

        int result = 0;

// send result
        int write_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        struct sockaddr_in write_addr;
        write_addr.sin_family = AF_INET;
        write_addr.sin_port = htons((unsigned short)CLIENT_PORT);
        write_addr.sin_addr.s_addr = peer_addr.sin_addr.s_addr;
        
        if(connect(write_socket, 
		    (struct sockaddr*)&write_addr, 
		    sizeof(write_addr)) < 0)
	    {
		    perror("client_thread: connect");
            close(write_socket);
            continue;
	    }

//         printf("client_thread %d: ACK to %x\n", 
//             __LINE__,
//             peer_addr.sin_addr.s_addr);

// ACK value
        uint8_t ack[8];
        ack[0] = 0;
        result = write(write_socket, ack, 1);
        close(write_socket);

// run command
        int id = packet[0];
        switch(id)
        {
            case LOAD_FILE:
                result = load_file((char*)(packet + 1));
                break;
            case SHOW_PAGE:
                MWindow::mwindow->current_page = (uint32_t)packet[1] |
                    (((uint32_t)packet[2]) << 8) |
                    (((uint32_t)packet[3]) << 16) |
                    (((uint32_t)packet[4]) << 24);
                MWindow::mwindow->show_page(MWindow::mwindow->current_page);
                result = 0;
                break;
        }
    }
}

void* server_thread(void *ptr)
{
    int read_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in read_addr;
    read_addr.sin_family = AF_INET;
    read_addr.sin_port = htons((unsigned short)CLIENT_PORT);
    read_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(read_socket, 
		(struct sockaddr*)&read_addr, 
		sizeof(read_addr)) < 0)
	{
		perror("server_thread: bind");
        exit(1);
	}

    int write_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in write_addr;
	struct hostent *hostinfo;
	write_addr.sin_family = AF_INET;
	write_addr.sin_port = htons((unsigned short)CLIENT_PORT);
    hostinfo = gethostbyname(CLIENT_ADDRESS);

	if(hostinfo == NULL)
    {
    	fprintf (stderr, 
            "server_thread %d: unknown host %s.\n", 
            __LINE__, 
            CLIENT_ADDRESS);
    	exit(1);
    }

    write_addr.sin_addr = *(struct in_addr *)hostinfo->h_addr;

	if(connect(write_socket, 
		(struct sockaddr*)&write_addr, 
		sizeof(write_addr)) < 0)
	{
		perror("server_thread: connect");
        exit(1);
	}




    while(1)
    {
        sem_wait(&command_ready);
        

        int retries = 0;
        int ack_result = 1;
        while(retries < RETRIES)
        {
            retries++;
            int result = write(write_socket, command, command_size);

// wait for ACK with a timeout
	        fd_set read_set;
	        struct timeval tv;
	        FD_ZERO(&read_set);
	        FD_SET(read_socket, &read_set);
	        tv.tv_sec = TIMEOUT / 1000;
	        tv.tv_usec = (TIMEOUT % 1000) * 1000;

           result = select(read_socket + 1, &read_set, 0, 0, &tv);
//printf("server_thread %d result=%d\n", __LINE__, result);
           if(result <= 0)
           {
               printf("server_thread %d: ACK timed out\n", __LINE__);
               continue;
           }

//printf("server_thread %d\n", __LINE__);
            socklen_t peer_addr_len = sizeof(struct sockaddr_in);
            struct sockaddr_in peer_addr;
            result = recvfrom(read_socket,
                packet, 
                PACKET_SIZE, 
                0,
                (struct sockaddr *) &peer_addr, 
                &peer_addr_len);
//printf("server_thread %d result=%d\n", __LINE__, result);
            if(result > 0)
            {
                ack_result = packet[0];
//printf("server_thread %d ack_result=%d\n", __LINE__, ack_result);
                break;
            }
            else
            {
                printf("server_thread %d: read %d bytes\n", __LINE__, result);
            }
        }
        command_result = ack_result;
        
        
        sem_post(&command_done);
    }
}

int send_command(int id, uint8_t *value, int value_size)
{
    command[0] = id;
    memcpy(command + 1, value, value_size);
    command_size = 1 + value_size;
    command_result = -1;
    sem_post(&command_ready);
}

int wait_command()
{
    sem_wait(&command_done);
    return command_result;
}

// Faster page turns may be possible by writing to the framebuffer directly, 
// but 25fps was good enough.
// https://gist.github.com/FredEckert/3425429
void init_fb()
{
    fb_fd = open("/dev/fb0", O_RDWR);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    printf("init_fb %d xres=%d yres=%d bpp=%d\n", 
        __LINE__, 
        vinfo.xres, 
        vinfo.yres, 
        vinfo.bits_per_pixel);
    // Map the device to memory
    fbp = (char *)mmap(0, 
        screensize, 
        PROT_READ | PROT_WRITE, 
        MAP_SHARED, 
        fb_fd, 
        0);
}

int load_file(char *path)
{
    int i;
    printf("load_file %d: loading %s\n", __LINE__, path);
    pages.remove_all_objects();
    if(client_mode)
    {
        MWindow::mwindow->current_page = 1;
    }
    else
    {
        MWindow::mwindow->current_page = 0;
    }

    FILE *src_fd = fopen(path, "r");
    if(!src_fd)
    {
        char string[BCTEXTLEN];
        sprintf(string, "Couldn't load:\n%s", path);
        printf("load_file %d: couldn't open image file %s\n", __LINE__, path);
        MWindow::mwindow->show_error(string);
        return 1;
    }

    int src_w;
    int src_h;
    int page_count;
    int _ = fread(&src_w, 1, 4, src_fd);
    _ = fread(&src_h, 1, 4, src_fd);
    _ = fread(&page_count, 1, 4, src_fd);
    printf("Pages: %d\n", page_count);


// reject the content if the size differs
    if(src_w != MWindow::mwindow->root_w ||
        src_h != MWindow::mwindow->root_h)
    {
        printf("main %d Source & screen have different dimensions\n", __LINE__);
        printf("source=%dx%d screen=%dx%d\n", 
            src_w, 
            src_h, 
            MWindow::mwindow->root_w, 
            MWindow::mwindow->root_h);
        fclose(src_fd);
        return 1;
    }

    for(i = 0; i < page_count; i++)
    {
        Page *page = new Page;
        pages.append(page);
        page->image = new VFrame;
        page->image->set_use_shm(0);
        page->image->reallocate(
		    0,   // Data if shared
		    -1,             // shmid if IPC  -1 if not
		    0,         // plane offsets if shared YUV
		    0,
		    0,
		    src_w, 
		    src_h, 
		    BC_A8, 
		    -1);   // -1 if unused
        int _ = fread(page->image->get_data(), 1, src_w * src_h, src_fd);
    }
    fclose(src_fd);

// show page 1
    MWindow::mwindow->show_page(MWindow::mwindow->current_page);
    return 0;
}

void load_file_entry(char *path)
{
#ifdef TWO_PAGE
    send_command(LOAD_FILE, 
        (uint8_t*)path, 
        strlen(path) + 1);
    load_file(path);
    wait_command();
#else
    ::load_file(path);
#endif
}


int main(int argc, char *argv[])
{
    int i, j, k;



	pthread_t tid;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);

    char *path = 0;
// load the image file
    if(argc > 1)
    {
        if(!strcmp(argv[1], "-c"))
        {
// start in client mode without loading a file
            client_mode = 1;
        }
        else
        {
            path = argv[1];
        }
    }

//    init_fb();
    BC_Resources::dpi = BASE_DPI;
    BC_Resources::override_dpi = 1;
    MWindow::mwindow = new MWindow;
    MWindow::mwindow->create_objects();

    if(!client_mode)
    {
        printf("Starting network server\n");
        sem_init(&command_ready, 0, 0);
        sem_init(&command_done, 0, 0);
        pthread_create(&tid, &attr, server_thread, 0);

        if(path)
        {
            load_file_entry(path);
        }

// enable the GPIOs
#ifndef USE_WINDOW
        wiringPiSetup() ;
        pinMode(UP_GPIO, INPUT);
        pinMode(DOWN_GPIO, INPUT);
        pullUpDnControl(UP_GPIO, PUD_UP);
        pullUpDnControl(DOWN_GPIO, PUD_UP);
        pthread_create(&tid, &attr, button_thread, 0);
#endif // !USE_WINDOW
    }
    else
// start the client
    {
        printf("Starting network client\n");
        pthread_create(&tid, &attr, client_thread, 0);
    }


    
// DEBUG
// benchmark the page turn speed
//     struct timeval current_time;
//     struct timeval new_time;
//     gettimeofday(&current_time, 0);
//     i = 0;
//     int page_count = 0;
//     while(1)
//     {
// #ifdef TWO_PAGE
//         int temp = i + 1;
//         send_command(SHOW_PAGE, (uint8_t*)&temp, sizeof(int));
//         MWindow::mwindow->show_page(i);
//         wait_command();
// #else
//         MWindow::mwindow->show_page(i);
// #endif
//         i++;
//         page_count++;
//         i %= pages.size();
//         gettimeofday(&new_time, 0);
//         int elapsed = (new_time.tv_usec - current_time.tv_usec) / 1000 +
//             (new_time.tv_sec - current_time.tv_sec) * 1000;
//         if(elapsed >= 1000)
//         {
//             current_time = new_time;
//             printf("main %d %d per sec\n", __LINE__, page_count);
//             page_count = 0;
//         }
//     }

    MWindow::mwindow->run_window();
    MWindow::mwindow->save_defaults();
    return 0;
}








