//-----------------------------------------------------------------------------
// Copyright 2015 Thiago Alves
//
// Based on the LDmicro software by Jonathan Westhues
// This file is part of the OpenPLC Software Stack.
//
// OpenPLC is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenPLC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenPLC.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// This file is the hardware layer for the OpenPLC. If you change the platform
// where it is running, you may only need to change this file. All the I/O
// related stuff is here. Basically it provides functions to read and write
// to the OpenPLC internal buffers in order to update I/O state.
// Thiago Alves, Dec 2015
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "iec_types.h"
#define  BUFFER_SIZE 1024

//Booleans
extern IEC_BOOL *bool_input[BUFFER_SIZE][8];
extern IEC_BOOL *bool_output[BUFFER_SIZE][8];

//Bytes
extern IEC_BYTE *byte_input[BUFFER_SIZE];
extern IEC_BYTE *byte_output[BUFFER_SIZE];

//Analog I/O
extern IEC_UINT *int_input[BUFFER_SIZE];
extern IEC_UINT *int_output[BUFFER_SIZE];

//Memory
extern IEC_UINT *int_memory[BUFFER_SIZE];
extern IEC_DINT *dint_memory[BUFFER_SIZE];
extern IEC_LINT *lint_memory[BUFFER_SIZE];

//Special Functions
extern IEC_LINT *special_functions[BUFFER_SIZE];

//lock for the buffer
extern pthread_mutex_t bufferLock;

//-----------------------------------------------------------------------------
// These are the ignored I/O vectors. If you want to override how OpenPLC
// handles a particular input or output, you must put them in the ignored
// vectors. For example, if you want to override %IX0.5, %IX0.6 and %IW3
// your vectors must be:
//     int ignored_bool_inputs[] = {5, 6}; //%IX0.5 and %IX0.6 ignored
//     int ignored_int_inputs[] = {3}; //%IW3 ignored
//
// Every I/O on the ignored vectors will be skipped by OpenPLC hardware layer
//-----------------------------------------------------------------------------
int ignored_bool_inputs[] = {-1};
int ignored_bool_outputs[] = {-1};
int ignored_int_inputs[] = {-1};
int ignored_int_outputs[] = {-1};

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#define MAX_INPUT_NUM 		8
#define MAX_OUTPUT_NUM 		8
#define MAX_ANALOG_OUT  3

//inBufferPinMask: pin mask for each input, which
//means what pin is mapped to that OpenPLC input
int inBufferPinMask[MAX_INPUT_NUM] = {64, 65, 66, 67, 68, 69, 70, 71};

//outBufferPinMask: pin mask for each output, which
//means what pin is mapped to that OpenPLC output
int outBufferPinMask[MAX_OUTPUT_NUM] =	{79, 78, 77, 76, 75, 74, 73, 72};

//analogOutBufferPinMask: pin mask for the analog PWM
//output of the coonix
int analogOutBufferPinMask[MAX_ANALOG_OUT] = {45, 108, 109};


static IEC_BOOL pinNotPresent(int *ignored_vector, int vector_size, int pinNumber)
{
    for (int i = 0; i < vector_size; i++)
    {
        if (ignored_vector[i] == pinNumber)
            return FALSE;
    }

    return TRUE;
}

static void export_digital_io()
{
    int n;
    unsigned char log_msg[1024];
    char buff[64];

    sprintf((char*)(log_msg), "export digital io\n");

    int fd = open("/sys/class/gpio/export", O_WRONLY);

    if(fd < 0) {
        perror("cannot open /sys/class/gpio/export");
    }

    for (int i = 0; i < MAX_INPUT_NUM; ++i) {
       n = sprintf(buff, "%d", inBufferPinMask[i]);
       if(write(fd, buff, n) != n) {
           perror("write /sys/class/gpio/export error");
       } else {
           sprintf((char*)(log_msg), "%d exported\n", inBufferPinMask[i]);
       }
    }

    for (int i = 0; i < MAX_OUTPUT_NUM; ++i) {
        n = sprintf(buff, "%d", outBufferPinMask[i]);
        if(write(fd, buff, n) != n) {
            perror("write /sys/class/gpio/export error");
        } else {
            sprintf((char*)(log_msg), "%d exported\n", outBufferPinMask[i]);
        }
    }

    close(fd);

    /* set io direction and default value */
    for (int i = 0; i < MAX_INPUT_NUM; ++i) {
        n = sprintf(buff, "/sys/class/gpio/gpio%d/direction", inBufferPinMask[i]);

        int gpio_fd = open(buff, O_WRONLY);
        if(gpio_fd < 0) {
            fprintf(stderr, "open %s error code:%d\n", buff, errno);
            continue;
        }

        if(write(fd, "in", 3) != 3) {
            fprintf(stderr, "write %s error code:%d\n", buff, errno);
        } else {
            sprintf((char*)(log_msg), "%d set input mode\n", inBufferPinMask[i]);
        }

        close(gpio_fd);
    }

    /* output is default */
    for (int i = 0; i < MAX_OUTPUT_NUM; ++i) {
        n = sprintf(buff, "/sys/class/gpio/gpio%d/direction", outBufferPinMask[i]);

        int gpio_fd = open(buff, O_WRONLY);
        if(gpio_fd < 0) {
            fprintf(stderr, "open %s error code:%d\n", buff, errno);
            continue;
        }

        if(write(fd, "out", 4) != 4) {
            fprintf(stderr, "write %s error code:%d\n", buff, errno);
        } else {
            sprintf((char*)(log_msg), "%d set output mode\n", outBufferPinMask[i]);
        }

        close(gpio_fd);
    }
}

static void unexport_digital_io()
{
    int n;
    unsigned char log_msg[1024];
    char buff[8];

    sprintf((char*)(log_msg), "unexport digital io\n");

    int fd = open("/sys/class/gpio/unexport", O_WRONLY);

    if(fd < 0) {
        perror("cannot open /sys/class/gpio/unexport");
    }

    for (int i = 0; i < MAX_INPUT_NUM; ++i) {
        n = sprintf(buff, "%d", inBufferPinMask[i]);
        if(write(fd, buff, n) != n) {
            perror("write /sys/class/gpio/unexport error");
        } else {
            sprintf((char*)(log_msg), "%d unexported", inBufferPinMask[i]);
        }
    }

    for (int i = 0; i < MAX_OUTPUT_NUM; ++i) {
        n = sprintf(buff, "%d", outBufferPinMask[i]);
        if(write(fd, buff, n) != n) {
            perror("write /sys/class/gpio/unexport error");
        } else {
            sprintf((char*)(log_msg), "%d unexported", outBufferPinMask[i]);
        }
    }

    close(fd);
}

static void set_digital_io_value(int index, uint8_t value)
{
    char buff[64];
    int ret;
    int fd;

    sprintf(buff, "/sys/class/gpio/gpio%d/value", index);

    fd = open(buff, O_WRONLY);

    if(fd < 0) {
        fprintf(stderr, "open %s error code:%d\n", buff, errno);
    }

    if(value) {
        ret = write(fd, "1", 1);
    } else {
        ret = write(fd, "0", 1);
    }

    if(ret < 0) {
        fprintf(stderr, "write %s error code:%d\n", buff, errno);
    }

    close(fd);
}

static uint8_t get_digital_io_value(int index)
{
    char buff[64];
    int fd;

    sprintf(buff, "/sys/class/gpio/gpio%d/value", index);

    fd = open(buff, O_RDONLY);

    if(fd < 0) {
        fprintf(stderr, "open %s error code:%d\n", buff, errno);
    }

    memset(buff, 0, sizeof(buff));

   if(read(fd, buff, 2) != 2) {
       fprintf(stderr, "read %s error code:%d\n", buff, errno);
   }

   close(fd);

   if(strstr(buff, "1")) {
       return 1;
   }

   return 0;
}


static void export_pwm_output(int i)
{
    char file_path[64];
    sprintf(file_path, "/sys/class/pwm/pwmchip%d/export", i);

    int fd = open(file_path, O_WRONLY);

    if(fd < 0) {
        fprintf(stderr, "open %s error code:%d\n", file_path, errno);
    }

    int ret = write(fd, "0", 1);

    if(ret < 0) {
        fprintf(stderr, "write %s error code:%d\n", file_path, errno);
    }

    close(fd);
}

static void unexport_pwm_output(int i)
{
    char file_path[64];
    sprintf(file_path, "/sys/class/pwm/pwmchip%d/unexport", i);

    int fd = open(file_path, O_WRONLY);

    if(fd < 0) {
        fprintf(stderr, "open %s error code:%d\n", file_path, errno);
    }

    int ret = write(fd, "0", 1);

    if(ret < 0) {
        fprintf(stderr, "write %s error code:%d\n", file_path, errno);
    }

    close(fd);
}

static void write_pwm_output_parameter(int index, const char* parameter, int value)
{
    char file_path[64];
    sprintf(file_path, "/sys/class/pwm/pwmchip%d/pwm0/%s", index, parameter);

    int fd = open(file_path, O_WRONLY);

    if(fd < 0) {
        fprintf(stderr, "open %s error code:%d\n", file_path, errno);
    }

    int ret = sprintf(file_path, "%d", value);

    ret = write(fd, file_path, ret);

    if(ret < 0) {
        fprintf(stderr, "write %s error code:%d\n", file_path, errno);
    }

    close(fd);
}

static void pwm_output_initialize()
{
    for (int i = 0; i < MAX_ANALOG_OUT; ++i) {
       export_pwm_output(i);
    }

    for (int i = 0; i < MAX_ANALOG_OUT; ++i) {
        write_pwm_output_parameter(i, "enable", 1);
        write_pwm_output_parameter(i, "period", 50000); // 20KHz
        write_pwm_output_parameter(i, "duty_cycle", 0);
    }
}

static void pwm_output_finalize()
{
    for (int i = 0; i < MAX_ANALOG_OUT; ++i) {
        write_pwm_output_parameter(i, "enable", 0);
    }

    for (int i = 0; i < MAX_ANALOG_OUT; ++i) {
        unexport_pwm_output(i);
    }
}

static void set_pwm_output_value(int index, uint16_t value)
{
    write_pwm_output_parameter(index, "duty_cycle", value);
}

//-----------------------------------------------------------------------------
// This function is called by the main OpenPLC routine when it is initializing.
// Hardware initialization procedures should be here.
//-----------------------------------------------------------------------------
void initializeHardware()
{
    export_digital_io();
    pwm_output_initialize();
}

//-----------------------------------------------------------------------------
// This function is called by the main OpenPLC routine when it is finalizing.
// Resource clearing procedures should be here.
//-----------------------------------------------------------------------------
void finalizeHardware()
{
    unexport_digital_io();
    pwm_output_finalize();
}

//-----------------------------------------------------------------------------
// This function is called by the OpenPLC in a loop. Here the internal buffers
// must be updated to reflect the actual Input state. The mutex bufferLock
// must be used to protect access to the buffers on a threaded environment.
//-----------------------------------------------------------------------------
void updateBuffersIn()
{
	pthread_mutex_lock(&bufferLock); //lock mutex

	/*********READING AND WRITING TO I/O**************
	**************************************************/
    //INPUT
    for (int i = 0; i < MAX_INPUT_NUM; i++)
    {
        if (pinNotPresent(ignored_bool_inputs, ARRAY_SIZE(ignored_bool_inputs), i))
            if (bool_input[i/8][i%8] != NULL) *bool_input[i/8][i%8] = get_digital_io_value(inBufferPinMask[i]);
    }

	pthread_mutex_unlock(&bufferLock); //unlock mutex
}

//-----------------------------------------------------------------------------
// This function is called by the OpenPLC in a loop. Here the internal buffers
// must be updated to reflect the actual Output state. The mutex bufferLock
// must be used to protect access to the buffers on a threaded environment.
//-----------------------------------------------------------------------------
void updateBuffersOut()
{
	pthread_mutex_lock(&bufferLock); //lock mutex

	/*********READING AND WRITING TO I/O**************
	**************************************************/
	//OUTPUT
	for (int i = 0; i < MAX_OUTPUT_NUM; i++)
	{
	    if (pinNotPresent(ignored_bool_outputs, ARRAY_SIZE(ignored_bool_outputs), i))
    		if (bool_output[i/8][i%8] != NULL) set_digital_io_value(outBufferPinMask[i], *bool_output[i/8][i%8]);
	}

    //ANALOG OUT (PWM)
    for (int i = 0; i < MAX_ANALOG_OUT; i++)
    {
        if (pinNotPresent(ignored_int_outputs, ARRAY_SIZE(ignored_int_outputs), i))
            if (int_output[i] != NULL) set_pwm_output_value(analogOutBufferPinMask[i], (*int_output[i]));
    }

	pthread_mutex_unlock(&bufferLock); //unlock mutex
}

