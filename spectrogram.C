extern "C"
{

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/serial.h>
#include <unistd.h>


#include "radar.h"

}


#include "guicast.h"
#include "thread.h"
#include "keys.h"



#define BAUD B115200
#define CUSTOM_BAUD 0
#define LINE_H 4
#define GAIN 255
#define HORIZONTAL_SCALE 1
#define DB_MIN -4

#define WINDOW_W 512
#define WINDOW_H 480



// Returns the FD of the serial port
static int init_serial(char *path, int baud, int custom_baud)
{
	struct termios term;

// Initialize serial port
	int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
	if(fd < 0)
	{
		printf("init_serial %d: path=%s: %s\n", __LINE__, path, strerror(errno));
		return -1;
	}
	
	if (tcgetattr(fd, &term))
	{
		printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		close(fd);
		return -1;
	}


// Try to set kernel to custom baud and low latency
	if(custom_baud)
	{
		struct serial_struct serial_struct;
		if(ioctl(fd, TIOCGSERIAL, &serial_struct) < 0)
		{
			printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		}

		serial_struct.flags |= ASYNC_LOW_LATENCY;
		serial_struct.flags &= ~ASYNC_SPD_CUST;
		if(custom_baud)
		{
			serial_struct.flags |= ASYNC_SPD_CUST;
			serial_struct.custom_divisor = (int)((float)serial_struct.baud_base / 
				(float)custom_baud + 0.5);
			baud = B38400;
		}
/*
 * printf("init_serial: %d serial_struct.baud_base=%d serial_struct.custom_divisor=%d\n", 
 * __LINE__,
 * serial_struct.baud_base,
 * serial_struct.custom_divisor);
 */


// Do setserial on the command line to ensure it actually worked.
		if(ioctl(fd, TIOCSSERIAL, &serial_struct) < 0)
		{
			printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		}
	}

/*
 * printf("init_serial: %d path=%s iflag=0x%08x oflag=0x%08x cflag=0x%08x\n", 
 * __LINE__, 
 * path, 
 * term.c_iflag, 
 * term.c_oflag, 
 * term.c_cflag);
 */
	tcflush(fd, TCIOFLUSH);
	cfsetispeed(&term, baud);
	cfsetospeed(&term, baud);
//	term.c_iflag = IGNBRK;
	term.c_iflag = 0;
	term.c_oflag = 0;
	term.c_lflag = 0;
//	term.c_cflag &= ~(PARENB | PARODD | CRTSCTS | CSTOPB | CSIZE);
//	term.c_cflag |= CS8;
	term.c_cc[VTIME] = 1;
	term.c_cc[VMIN] = 1;
/*
 * printf("init_serial: %d path=%s iflag=0x%08x oflag=0x%08x cflag=0x%08x\n", 
 * __LINE__, 
 * path, 
 * term.c_iflag, 
 * term.c_oflag, 
 * term.c_cflag);
 */
	if(tcsetattr(fd, TCSANOW, &term))
	{
		printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}


// Read a character
unsigned char read_char(int fd)
{
	unsigned char c;
	int result;
	do
	{
		result = read(fd, &c, 1);
	} while(result <= 0);
	return c;
}

class Reader;

class MainWindow : public BC_Window
{
public:
	MainWindow();
	~MainWindow();

	void create_objects();
	int close_event();
	int keypress_event();
	
	BC_Bitmap *bitmap;
	Reader *reader;
};

class Reader : public Thread
{
public:
	Reader(int serial_fd, MainWindow *mwindow);
	~Reader();
	
	void run();
	
	int serial_fd;
	MainWindow *mwindow;
	int gain;
};


MainWindow::MainWindow() : 
	BC_Window(
		"Radar: Spectrogram",
		0,
		0,
        WINDOW_W, 
        WINDOW_H,
		10,
		10,
		1,
		0,
		1,
		BLACK)
{
}

MainWindow::~MainWindow()
{
}

int MainWindow::close_event()
{
	

	set_done(0);
	return 1;
}

int MainWindow::keypress_event()
{
	switch(get_keypress())
	{
		case ESC:
		case 'q':
			set_done(0);
			return 1;
			break;
		case '+':
			reader->gain *= 2;
			printf("MainWindow::keypress_event %d gain=%d\n", __LINE__, reader->gain);
			return 1;
			break;
		case '-':
			reader->gain /= 2;
			if(reader->gain < 1) reader->gain = 1;
			printf("MainWindow::keypress_event %d gain=%d\n", __LINE__, reader->gain);
			return 1;
			break;
	}
	
	return 0;
}

void MainWindow::create_objects()
{
	bitmap = new_bitmap(WINDOW_W, WINDOW_H);
	show_window(1);
	
}




Reader::Reader(int serial_fd, MainWindow *mwindow) : Thread()
{
	this->serial_fd = serial_fd;
	this->mwindow = mwindow;
	gain = GAIN;
}

Reader::~Reader()
{
}

void Reader::run()
{
	unsigned char buffer[DISPLAY_SAMPLES * 4];
	double magnitude[DISPLAY_SAMPLES];
	double fft_accum[DISPLAY_SAMPLES];
	unsigned char code[4];
	int total_fft = 0;
	while(1)
	{
		unsigned char c = read_char(serial_fd);
		code[0] = code[1];
		code[1] = code[2];
		code[2] = code[3];
		code[3] = c;
		if(code[0] == 0xb &&
			code[1] == 0xe &&
			code[2] == 0xe &&
			code[3] == 0xf)
		{
//printf("main %d\n", __LINE__);
			int i;
			for(i = 0; i < DISPLAY_SAMPLES * 4; i++)
			{
				buffer[i] = read_char(serial_fd);
			}

			int16_t *data = (int16_t*)buffer;
			for(i = 0; i < DISPLAY_SAMPLES; i++)
			{
				fft_accum[i] += sqrt(data[i * 2] * data[i * 2] +
					data[i * 2 + 1] * data[i * 2 + 1]);
			}
			
			total_fft++;
			if(total_fft >= FFT_STACK_SIZE)
			{
				double max = 0;
				for(i = 0; i < DISPLAY_SAMPLES; i++)
				{
					magnitude[i] = fft_accum[i] / FFT_STACK_SIZE;
					if(magnitude[i] > max || i == 0) max = magnitude[i];
				}
//printf("%f\n", max);

				double db_min = 0;
				for(i = 0; i < DISPLAY_SAMPLES; i++)
				{
					double db;
					if(magnitude[i] < 0.001)
						db = 4.0;
					else
						db = log10(magnitude[i] / max);
					magnitude[i] = db;
					if(db < db_min || i == 0) db_min = db;
				}

printf("%f\n", db_min);
				db_min = DB_MIN;
				bzero(fft_accum, sizeof(double) * DISPLAY_SAMPLES);
				total_fft = 0;
			

				mwindow->lock_window("main 1");
				unsigned char *row = mwindow->bitmap->get_data(); 
				for(i = 0; i < WINDOW_W; i++)
				{
					int index = i * (DISPLAY_SAMPLES / HORIZONTAL_SCALE) / WINDOW_W;
					double mag = (magnitude[index] - db_min) / (-db_min);

					if(magnitude[index] == 4.0) mag = 0;
					int color = gain * mag;


//printf("%d %d\n", i, index);
					if(color > 255) color = 255;
					if(color < 0) color = 0;
					row[i * 4 + 0] = color;
					row[i * 4 + 1] = color;
					row[i * 4 + 2] = color;
					row[i * 4 + 3] = color;
				}

				mwindow->draw_bitmap(mwindow->bitmap, 
					1, 
					0, 
					WINDOW_H - 1, 
					WINDOW_W, 
					1,
					0,
					0,
					WINDOW_W,
					1);

				for(int i = 0; i < LINE_H; i++)
				{
					mwindow->copy_area(0, 1, 0, 0, WINDOW_W, WINDOW_H - 1);
				}

				mwindow->flash();
				mwindow->unlock_window();
			}

		}
	}
}








int main(int argc, char *argv[])
{
	int serial_fd = init_serial((char*)"/dev/ttyUSB0", BAUD, CUSTOM_BAUD);
	if(serial_fd < 0) serial_fd = init_serial((char*)"/dev/ttyUSB1", BAUD, CUSTOM_BAUD);
//	if(serial_fd < 0) return 1;



	MainWindow *mwindow = new MainWindow;
	mwindow->create_objects();

	Reader *reader = new Reader(serial_fd, mwindow);
	reader->start();
	
	mwindow->reader = reader;
	mwindow->run_window();


}






