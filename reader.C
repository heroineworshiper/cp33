/*
 * MUSIC READER
 * Copyright (C) 2021-2024 Adam Williams <broadcast at earthling dot net>
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

// step 1: import the text using the pdftoreader2 program

// step 2: run this program as a controller
// export DISPLAY=:0
// ./reader [client address:port] [server port]

// run this program as a client
// ./reader -c <client port> <server port>

// OBSOLETE
// export an image for each page with names output01.png, output02.png...
// Edit reader.html & load in a browser to view the images.
// ./reader <input> -e <output>

// compile it on a raspberry:
// apt install xorg
// apt install libx11-dev
// apt install libxext-dev
// apt install libxv-dev
// apt install wiringpi
// make -f Makefile.reader
// scp reader pi2:

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
#include <sys/stat.h>


#include "capture.h"
#include "capturemenu.h"
#include "capturemidi.h"
#include "mwindow.h"
#include "reader.h"
//#include "readerbrushes.h"
#include "readermenu.h"
#include "readertheme.h"
#include "sema.h"
#include "score.h"







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

// received command
uint8_t packet[PACKET_SIZE];

// next commandto send
uint8_t command[PACKET_SIZE];
int command_counter = 0;
int command_size;

// command currently being sent
uint8_t command2[PACKET_SIZE];
int command_size2;
int command_counter2 = 0;

int command_result;
Sema command_ready;
Sema command_done;
int client_mode = 0;
const char *export_path = 0;


// framebuffer I/O
int fb_fd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int screensize = 0;
char *fbp = 0;

// the current reader file
char reader_path[BCTEXTLEN];
int file_changed = 0;
int current_page = 0;

int mode = READER_MODE;



char client_address[BCTEXTLEN];
int client_port;
int server_port;

Page::Page()
{
    x1 = y1 = x2 = y2 = 0;
    image = 0;
    annotations = 0;
}

Page::~Page()
{
    if(image)
    {
        delete image;
    }
    
    if(annotations)
    {
        delete annotations;
    }
}

ArrayList<Page*> pages;






void next_page(int step, int lock_it)
{
//    printf("next_page %d\n", __LINE__);
#ifdef TWO_PAGE
    if(current_page < pages.size() - step)
    {
        MWindow::instance->reset_undo();
        current_page += step;
        int temp = current_page + 1;
        send_command(SHOW_PAGE, (uint8_t*)&temp, sizeof(int));
        MWindow::instance->show_page(current_page, lock_it);
        wait_command();
    }
#else
    if(current_page < pages.size() - 1)
    {
        MWindow::instance->reset_undo();
        current_page++;
        MWindow::instance->show_page(current_page, lock_it);
    }
#endif
}

void prev_page(int step, int lock_it)
{
//    printf("prev_page %d\n", __LINE__);
    if(current_page > 0)
    {
#ifdef TWO_PAGE
        MWindow::instance->reset_undo();
        current_page -= step;
        if(current_page < 0)
        {
            current_page = 0;
        }
        int temp = current_page + 1;
        send_command(SHOW_PAGE, (uint8_t*)&temp, sizeof(int));
        MWindow::instance->show_page(current_page, lock_it);
        wait_command();
#else
        MWindow::instance->reset_undo();
        current_page--;
        MWindow::instance->show_page(current_page, lock_it);
#endif
    }
}

// called from MenuWindow
void do_capture()
{
    mode = CAPTURE_MODE;
    MWindow::instance->current_operation = IDLE;

    MenuWindow::instance->hide_windows(0);
    CaptureMenu::instance->show();
    Capture::instance->draw_score();
}

void do_reader()
{
    mode = READER_MODE;
    MWindow::instance->current_operation = IDLE;

    CaptureMenu::instance->hide_window();
    MenuWindow::instance->show();
    MWindow::instance->show_page(current_page, 1);
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
                next_page(2, 1);
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
                prev_page(2, 1);
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
	read_addr.sin_port = htons((unsigned short)client_port);
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
        write_addr.sin_port = htons((unsigned short)server_port);
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
            case LOAD_ANNOTATIONS:
                result = load_annotations();
                if(!result)
                {
                    MWindow::instance->show_page(current_page, 1);
                }
                break;
            case SHOW_PAGE:
                current_page = (uint32_t)packet[1] |
                    (((uint32_t)packet[2]) << 8) |
                    (((uint32_t)packet[3]) << 16) |
                    (((uint32_t)packet[4]) << 24);
                MWindow::instance->show_page(current_page, 1);
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
    read_addr.sin_port = htons((unsigned short)server_port);
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
	write_addr.sin_port = htons((unsigned short)client_port);
    hostinfo = gethostbyname(client_address);

	if(hostinfo == NULL)
    {
    	fprintf (stderr, 
            "server_thread %d: unknown host %s.\n", 
            __LINE__, 
            client_address);
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
        command_ready.lock();
        

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
        
        
        command_done.unlock();
    }
}

int send_command(int id, uint8_t *value, int value_size)
{
    command[0] = id;
//    command[1] = command_counter++;
    memcpy(command + 1, value, value_size);
    command_size = 1 + value_size;
    command_result = -1;
    command_ready.unlock();
}

int wait_command()
{
    command_done.lock();
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

// convert the path to an annotation path
void get_annotation_path(char *dst, char *src)
{
    strcpy(dst, src);
    char *ptr = strstr(dst, READER_SUFFIX);
    if(ptr)
    {
        sprintf(ptr, ANNOTATION_SUFFIX);
    }
    else
    {
// don't overwrite the src if failure
        dst[0] = 0;
    }
}

int save_annotations()
{
    char path[BCTEXTLEN];
    if(reader_path[0] && pages.size() > 0)
    {
        get_annotation_path(path, reader_path);
        
        char command[BCTEXTLEN];
        sprintf(command, "gzip -1 > %s", path);
        FILE *fd = popen(command, "w");
        if(!fd)
        {
            printf("save_annotations %d: error writing to %s\n",
                __LINE__,
                path);
            return 1;
        }
        
        
        for(int i = 0; i < pages.size(); i++)
        {
            VFrame *frame = pages.get(i)->annotations;
            fwrite(frame->get_rows()[0],
                1,
                frame->get_w() * frame->get_h(),
                fd);
        }
        
        fclose(fd);
        file_changed = 0;
        return 0;
    }
    return 1;
}

int save_annotations_entry()
{
#ifdef TWO_PAGE
    int result = save_annotations();
    send_command(LOAD_ANNOTATIONS, 
        0, 
        0);
    wait_command();
#else
    int result = save_annotations();
#endif
    return result;
}


int load_annotations()
{
    char path[BCTEXTLEN];
    if(reader_path[0] && pages.size() > 0)
    {
        get_annotation_path(path, reader_path);
        
// test file's existence
        struct stat statbuf;
        if(stat(path, &statbuf))
        {
// doesn't exist
            printf("load_annotations %d: no annotation file %s\n",
                __LINE__,
                path);
            return 1;
        }
        
        char command[BCTEXTLEN];
        sprintf(command, "gunzip -c %s", path);
        FILE *fd = popen(command, "r");
        if(!fd)
        {
            printf("load_annotations %d: error reading %s\n",
                __LINE__,
                path);
            return 1;
        }
        
        
        for(int i = 0; i < pages.size(); i++)
        {
            VFrame *frame = pages.get(i)->annotations;
            int bytes_read = fread(frame->get_rows()[0],
                1,
                frame->get_w() * frame->get_h(),
                fd);
            if(bytes_read < frame->get_w() * frame->get_h())
            {
                printf("load_annotations %d: error reading %s page %d\n",
                    __LINE__,
                    path,
                    i);
            }
        }
        
        fclose(fd);
        return 0;
    }
}

int load_file(const char *path)
{
    int i;
    printf("load_file %d: loading %s\n", __LINE__, path);
    pages.remove_all_objects();
    if(client_mode)
    {
        current_page = 1;
    }
    else
    {
        current_page = 0;
        MWindow::instance->reset_undo();
        MWindow::instance->zoom_x = 0;
        MWindow::instance->zoom_y = 0;
        MWindow::instance->zoom_factor = 1;
    }

    strcpy(reader_path, path);
    file_changed = 0;

    char command[BCTEXTLEN];
    sprintf(command, "gunzip -c %s", path);
    FILE *src_fd = popen(command, "r");
    if(!src_fd)
    {
        char string[BCTEXTLEN];
        sprintf(string, "Couldn't load:\n%s", path);
        printf("load_file %d: couldn't open image file %s\n", __LINE__, path);
        MWindow::instance->show_error(string);
        return 1;
    }

    int src_w;
    int src_h;
    int page_count;
    int _ = fread(&src_w, 1, 4, src_fd);
    _ = fread(&src_h, 1, 4, src_fd);
    _ = fread(&page_count, 1, 4, src_fd);
    printf("Pages: %d w=%d h=%d\n", page_count, src_w, src_h);


// reject the content if the size differs
    if(src_w != MWindow::instance->root_w ||
        src_h != MWindow::instance->root_h)
    {
        printf("main %d: Source & screen have different dimensions\n", __LINE__);
        printf("source=%dx%d screen=%dx%d\n", 
            src_w, 
            src_h, 
            MWindow::instance->root_w, 
            MWindow::instance->root_h);
        fclose(src_fd);
        return 1;
    }

    for(i = 0; i < page_count; i++)
    {
        Page *page = new Page;
        pages.append(page);
        page->image = new VFrame(src_w, 
		    src_h, 
		    BC_A8);
        page->annotations = new VFrame(src_w, 
		    src_h, 
		    BC_A8);
        int _ = fread(page->image->get_data(), 1, src_w * src_h, src_fd);
    }
    fclose(src_fd);


    load_annotations();

// export all the pages
    if(export_path)
    {
        for(i = 0; i < page_count; i++)
        {
            if(MWindow::instance->export_page(export_path, i)) break;
        }
        exit(0);
    }
    else
// show page 1
    {
        MWindow::instance->show_page(current_page, 1);
    }

    return 0;
}

void load_file_entry(const char *path)
{
    if(file_changed)
    {
        save_annotations();
    }

    MenuWindow::instance->lock_window();
    MenuWindow::instance->start_hourglass();
    MenuWindow::instance->unlock_window();
    MWindow::instance->lock_window();
    MWindow::instance->start_hourglass();
    MWindow::instance->unlock_window();

#ifdef TWO_PAGE
    send_command(LOAD_FILE, 
        (uint8_t*)path, 
        strlen(path) + 1);
    ::load_file(path);
    wait_command();
#else
    ::load_file(path);
#endif

    MenuWindow::instance->lock_window();
    MenuWindow::instance->save->set_images(MWindow::instance->theme->get_image_set("save"));
    MenuWindow::instance->save->draw_face(1);
    MenuWindow::instance->stop_hourglass();
    MenuWindow::instance->unlock_window();
    MWindow::instance->lock_window();
    MWindow::instance->stop_hourglass();
    MWindow::instance->unlock_window();

}


int main(int argc, char *argv[])
{
    int i, j, k;



	pthread_t tid;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);

//    const char *path = 0;
    client_address[0] = 0;
    client_port = -1;
    server_port = -1;

// load the image file
    if(argc > 1)
    {
        for(i = 1; i < argc; i++)
        {
            if(!strcmp(argv[i], "-c"))
            {
// start in client mode without loading a file
                client_mode = 1;
                if(i >= argc)
                {
                    printf("-c needs a client port\n");
                    exit(1);
                }
                else
                {
                    i++;
                    client_port = atoi(argv[i]);
                }
                
                if(i >= argc)
                {
                    printf("-c needs a server port\n");
                    exit(1);
                }
                else
                {
                    i++;
                    server_port = atoi(argv[i]);
                }
            }
            else
            {
// enter server mode
// split the client address
#ifdef TWO_PAGE
                if(client_address[0] == 0)
                {
                    char *ptr = strchr(argv[i], ':');
                    if(ptr)
                    {
                        strcpy(client_address, argv[i]);
                        client_port = atoi(ptr + 1);
                        ptr = strchr(client_address, ':');
                        *ptr = 0;
                    }
                    else
                    {
                        printf("Need a client & port of the form address:port\n");
                        exit(1);
                    }
                }
                else
                {
                    server_port = atoi(argv[i]);
                }
#endif // TWO_PAGE
            }
//             if(!strcmp(argv[i], "-e"))
//             {
//                 i++;
//                 if(i >= argc)
//                 {
//                     printf("-e needs a filename\n");
//                     exit(1);
//                 }
//                 else
//                 {
//                     export_path = argv[i];
//                 }
//             }
//             else
//             {
//                 path = argv[i];
//             }
        }
    }

// the mane database
    Score::instance = new Score;
    Score::instance->test();

//    init_fb();
    BC_Resources::dpi = BASE_DPI;
    BC_Resources::override_dpi = 1;
//    init_brushes();
    MWindow::init_colors();
    MWindow::instance = new MWindow;
    Capture::instance = new Capture;
    CaptureMIDI::instance = new CaptureMIDI;

    MWindow::instance->create_objects();
    Capture::instance->create_objects();
    CaptureMIDI::instance->initialize();

    if(!client_mode && client_address[0])
    {
        if(server_port < 0)
        {
            printf("Server port not specified\n");
            exit(1);
        }

        printf("Starting network server\n");
        pthread_create(&tid, &attr, server_thread, 0);

//         if(path)
//         {
//             load_file_entry(path);
//         }

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
    if(client_mode)
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
//         MWindow::instance->show_page(i, 1);
//         wait_command();
// #else
//         MWindow::instance->show_page(i, 1);
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

    MWindow::instance->run_window();
    if(file_changed)
    {
        save_annotations();
    }
    MWindow::instance->save_defaults();
    return 0;
}








