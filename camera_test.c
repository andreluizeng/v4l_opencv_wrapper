/**
 * @file
 * @author  Andre Luiz Vieira da Silva <andre.silva@freescale.com>
 *
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * This V4
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

#include "opencv2/video/tracking.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc_c.h"

#include "v4l_wrapper.h"


int Kbhit (void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}

int main (int argc, char **argv)
{
	fsl_v4l_cap mycamera;
	//fsl_v4l_out mydisplay;
	int ret;
	char *buffer;
	
	int width = 640;
	int height = 480;
	IplImage *image;

	printf ("\n\nInitialzing Camera Device: Video0 (640x480)...");
	
	ret = V4LWrapper_CreateCameraCapture (&mycamera, "/dev/video0", width, height);
	
	if (ret == V4LWrapper_SUCCESS)
	{
		printf ("OK\n");
	}
	
	else
	{
		printf ("\nDevice not found, make sure the driver are properly installed:");
		printf ("\nov5642_camera.ko, ov5640_camera_mipi.ko and mxc_v4l2_capture.ko\n");
		exit (0);
	}

	//printf ("\nInitialzing Display Device: video17, fb0 (640x480)...");
	//ret = V4LWrapper_CreateOutputDisplay (&mydisplay, "/dev/fb0", NULL, width, height);
	//if (ret == V4LWrapper_SUCCESS)
	//	printf ("OK\n");
	//else
	//{
	//	V4LWrapper_CloseCameraCapture (&mycamera);
	//	exit (0);
	//}
	
	printf ("\nAllocating data buffer...");
	buffer = (char *) malloc (mycamera.g_frame_size);
	
	if (buffer)
		printf ("OK\n");

	else
	{
		V4LWrapper_CloseCameraCapture (&mycamera);
		//V4LWrapper_CloseOutputDisplay (&mydisplay);1
		
		exit (0);
	}

	
	image = cvCreateImage (cvSize (width, height), IPL_DEPTH_8U, 3);
	
	IplImage *gray = cvCreateImage (cvSize (width, height), IPL_DEPTH_8U, 1);
	
	while (! Kbhit())
	{
		V4LWrapper_QueryFrame (&mycamera, buffer);	
		V4LWrapper_CvtColor (buffer, image->imageData, width, height, YUV422toRGB888);
		cvCvtColor (image, gray, CV_RGB2GRAY);
		cvCanny (gray, gray, 60, 110, 3);
		cvShowImage ("test", gray);
		//V4LWrapper_OutputDisplay (&mydisplay, buffer);
		
		cvWaitKey (10);
	}

	V4LWrapper_CloseCameraCapture (&mycamera);
	//V4LWrapper_CloseOutputDisplay (&mydisplay);
	free (buffer);
	cvReleaseImage (&image);
	
	return 0;
}