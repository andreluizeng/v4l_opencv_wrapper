/**
 * @file
 * @author  Andre Luiz Vieira da Silva <andre.silva@freescale.com>
 *
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * This files contains V4L Framebuffer Display and Camera capture utility functions
 */


#ifndef __v4l_wrapper_h__
#define __v4l_wrapper_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/ipu.h>
#include <linux/videodev2.h>
#include <linux/mxcfb.h>
#include <linux/mxc_v4l2.h>

#define V4LWrapper_ERROR 		-1
#define V4LWrapper_SUCCESS 		 0

#define NUM_OUT_BUFFERS		4	// number of output buffers
#define NUM_CAP_BUFFERS		3	// number of capture buffers

#define RGB888toYUV422		0
#define YUV422toRGB888		1
	
struct fsl_v4l_out_str
{
	char v4l_output_dev[100];   	// /dev/video17
	int fd_output_v4l;		// 0
	int g_fmt_out;			// V4L2_PIX_FMT_UYVY // V4L2_PIX_FMT_RGB32
	//int g_rotate; 		// 0
	//int g_vflip; 			// 0
	//int g_hflip; 			// 0
	//int g_vdi_enable;		// 0
	//int g_vdi_motion;		// 0
	//int g_tb; 			// 0
	int g_output; 			// 3
	int g_output_num_buffers; 	// 4
	int g_display_width; 		// 0
	int g_display_height; 		// 0
	//int g_display_top; 		// 0
	//int g_display_left; 		// 0
	int g_frame_size;		// 0
	int g_frame_period; 		// 33333
	int fd_fb;			//framebuffer id
};

typedef struct fsl_v4l_out_str fsl_v4l_out;


/**
 @struct fsl_v4l_cap struct to v4l capture/display resources
 */
struct fsl_v4l_cap_str
{
	char v4l_capture_dev[100]; 	// /dev/video0
	int fd_capture_v4l; 		// 0
	//int g_cap_mode;		// 0
	int g_input;			// 1
	int g_fmt_in;			// V4L2_PIX_FMT_UYVY // IPU_PIX_FMT_RGB32
	//int g_rotate; 		// 0
	//int g_vflip; 			// 0
	//int g_hflip; 			// 0
	//int g_vdi_enable;		// 0
	//int g_vdi_motion;		// 0
	//int g_tb; 			// 0
	int g_capture_num_buffers; 	// 3
	int g_frame_size;		// 0
	int g_in_width; 		// 0
	int g_in_height; 		// 0
	v4l2_std_id g_current_std; 	// V4L2_STD_UNKNOWN //V4L2_STD_NTSC
};


struct out_buffer_str
{
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

struct out_buffer_str out_buffer[NUM_OUT_BUFFERS];

/**
  Function to Create the Display Output
  @param display v4l display struct
  @param framebuffer currently framebuffer device (default "/dev/fb0")
  @param device installed device ("/dev/video17")
  @param width display width
  @param height display height
  @return V4LWrapper_SUCCESS if success, V4LWrapper_ERROR if it fails
  @note Usage:
  @code
  
	fsl_v4l_out mydisplay;
	.
	.
	.
	return_value = V4LWrapper_CreateOutputDisplay (&mydisplay, "/dev/fb0", "/dev/video17", 640, 480);
	if (return_value == V4LWrapper_SUCCESS)
		printf ("\nOutput Display Successfully Created\n");
	.
	.
	.
  @endcode
 */
int V4LWrapper_CreateOutputDisplay (fsl_v4l_out *display, char *framebuffer, char *device, int width, int height);

/**
  Function to close the output device
  @param display v4l display struct
  @note Usage:
  @code
  
	fsl_v4l_out mydisplay;
	.
	.
	.
	return_value = V4LWrapper_CreateOutputDisplay (&mydisplay, "/dev/vide17", 640, 480);
	if (return_value == V4LWrapper_SUCCESS)
		printf ("\nOutput Display Successfully Created\n");
	.
	.
	.
	V4LWrapper_CloseOutputDisplay (&mydisplay);
	.
	.
	.

	
  @endcode
 */
void V4LWrapper_CloseOutputDisplay (fsl_v4l_out *display);

/**
   @WARNING Function to prepare the output - used by V4LWrapper_OutputDisplay function (do not use this function directly)
  @param display v4l display struct
  @return V4LWrapper_SUCCESS if success, V4LWrapper_ERROR if it fails
  */
int V4LWrapper_OutputSetup (fsl_v4l_out *display);

/**
  Function to display the frame data
  @param display v4l display struct
  @param buffer	output buffer with a frame data
  @note Usage:
  @code
  
	fsl_v4l_out mydisplay;
	fsl_v4l_cap mycamera;
	char *buffer;
	.
	.
	.
	return_value = V4LWrapper_CreateCameraCapture (&mycamera, "/dev/vide0", 640, 480);
	if (return_value == V4LWrapper_SUCCESS)
		printf ("\nCamera Capture Successfully Created\n");
	
	return_value = V4LWrapper_CreateOutputDisplay (&mydisplay, "/dev/vide17", 640, 480);
	if (return_value == V4LWrapper_SUCCESS)
		printf ("\nOutput Display Successfully Created\n");
	
	buffer = (char *) malloc (mycamera.g_frame_size);

	while (1)
	{
		V4LWrapper_QueryFrame (&mycamera, buffer);
		V4LWrapper_OutputDisplay (&mydisplay, buffer);
	}

	V4LWrapper_CloseCameraCapture (&mycamera);
	V4LWrapper_CloseOutputDisplay (&mydisplay);
	free (buffer);
	.
	.
	.
	
	
  @endcode
 */
int V4LWrapper_OutputDisplay (fsl_v4l_out *display, char *buffer);


typedef struct fsl_v4l_cap_str fsl_v4l_cap;

struct cap_buffer_str
{
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

struct cap_buffer_str cap_buffer[NUM_CAP_BUFFERS];

/**
  Function to Create the Camera capture using the onboard Sensor
  @param capture camera struct
  @param device installed device ("/dev/video0")
  @param width image width
  @param height image height
  @return V4LWrapper_SUCCESS if success, V4LWrapper_ERROR if it fails
  @note Usage:
  @code
  
	fsl_v4l_cap mycamera;
	.
	.
	.
	return_value = V4LWrapper_CreateCameraCapture (&mycamera, "/dev/vide0", 640, 480);
	if (return_value == V4LWrapper_SUCCESS)
		printf ("\nCamera Capture Successfully Created\n");
	.
	.
	.
  @endcode
 */
int V4LWrapper_CreateCameraCapture (fsl_v4l_cap *capture, char *device, int width, int height);

/**
  Function to close the Camera device
  @param capture camera struct
  @note Usage:
  @code
  
	fsl_v4l_cap mycamera;
	.
	.
	.
	return_value = V4LWrapper_CreateCameraCapture (&mycamera, "/dev/video0", 640, 480);
	if (return_value == V4LWrapper_SUCCESS)
		printf ("\nCamera Capture Successfully Created\n");
	.
	.
	.
	V4LWrapper_CloseCameraCapture (&mycamera);
	.
	.
	.

	
  @endcode
 */
void V4LWrapper_CloseCameraCapture (fsl_v4l_cap *capture);

/**
   @WARNING Function to start capturing - used by V4LWrapper_QueryFrame function (do not use this function directly)
  @param capture camera struct
  */
int V4LWrapper_StartCapturing (fsl_v4l_cap *capture);

/**
  Function Query a frame from the capture device
  @param capture camera struct
  @param buffer	output buffer with a frame from camera
  @return V4LWrapper_SUCCESS if success, V4LWrapper_ERROR if it fails
  @note Usage:
  @code
  
	fsl_v4l_cap mycamera;
	char *buffer;
	.
	.
	.
	return_value = V4LWrapper_CreateCameraCapture (&mycamera, "/dev/vide0", 640, 480);
	if (return_value == V4LWrapper_SUCCESS)
		printf ("\nCamera Capture Successfully Created\n");
	.
	.
	.
	buffer = (char *) malloc (mycamera.g_frame_size);
	
	int return_value = V4LWrapper_QueryFrame (&mycamera, buffer);
	
	if (return_value != V4LWrapper_SUCCESS)
		printf ("\nError Query a frame from capture device\n");
	.
	.
	.
	V4LWrapper_CloseCameraCapture (&mycamera);
	free (buffer);
	.
	.
	.

	
  @endcode
 */
int V4LWrapper_QueryFrame (fsl_v4l_cap *capture, char *buffer);


/**
  Function to perform colorspace conversion (YUV422 to RGB888 and vice-versa)
  @param buffer_in 	input buffer with image data (
  @param buffer_out	output buffer with image data
  @param width		image width
  @param height		image height
  @param colorflag	RGB888toYUV422 or YUV422toRGB888
  @return V4LWrapper_SUCCESS if success, V4LWrapper_ERROR if it fails
  @note Usage:
  @code
  
	char *buffer_in;
	char *buffer_out;
	
	int width = 640;
	int height = 480;
	.
	.
	.
	buffer_in = (char *) malloc (width * height * 2); // YUV422 format - 16bpp
	buffer_out = (char *) malloc (width * height * 3); // RGB888 format - 24bpp
	
	return_value = V4LWrapper_CvtColor (buffer_in, buffer_out, width, height, RGB888toYUV422); // or YUV422toRGB888
	if (return_value != V4LWrapper_SUCCESS)
		printf ("\nFailed to colorspace conversion\n");
	.
	.
	.
	free (buffer_in);
	free (buffer_out);

  @endcode
 */
int V4LWrapper_CvtColor (char *buffer_in, char *buffer_out, int width, int height, int colorflag);

#endif