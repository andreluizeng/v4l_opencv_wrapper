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

#include "v4l_wrapper.h"

int V4LWrapper_CreateOutputDisplay (fsl_v4l_out *display, char *framebuffer, char *device, int width, int height)
{

	//struct v4l2_control ctrl;
	struct v4l2_format fmt;
	struct v4l2_framebuffer fb;
	//struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_requestbuffers buf_req;

	// default is /dev/video17
	if (device == NULL)
		strcpy (display->v4l_output_dev, "/dev/video17");
	else
		strcpy (display->v4l_output_dev, device);
	
	display->g_fmt_out 		= V4L2_PIX_FMT_UYVY; // V4L2_PIX_FMT_RGB32
	display->fd_output_v4l		= 0;
	//display->g_rotate 		= 0;z
	//display->g_vflip 		= 0;
	//display->g_hflip 		= 0;
	//display->g_vdi_enable 	= 0;
	//display->g_vdi_motion 	= 0;
	//display->g_tb 		= 0;
	display->g_output 		= 3;
	display->g_output_num_buffers 	= NUM_OUT_BUFFERS;
	display->g_display_width 	= width;
	display->g_display_height 	= height;
	//display->g_display_top 	= 0;
	//display->g_display_left 	= 0;
	display->g_frame_size 		= 0;
	display->g_frame_period 	= 33333;

	//-----  For framebuffer settings
	char fb_device[100] = "/dev/fb0";
	
	if (framebuffer != NULL)
		strcpy (fb_device, framebuffer);
		
	display->fd_fb = 0;
	
	struct mxcfb_gbl_alpha alpha;
	//--------------------------------

	if ((display->fd_output_v4l = open(display->v4l_output_dev, O_RDWR, 0)) < 0)
	{
		printf("Unable to open %s\n", display->v4l_output_dev);
		return V4LWrapper_ERROR;
	}
	
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	
	while (! ioctl (display->fd_output_v4l, VIDIOC_ENUM_FMT, &fmtdesc)) 
	{
		//printf("fmt %s: fourcc = 0x%08x\n", fmtdesc.description, fmtdesc.pixelformat);
		fmtdesc.index++;
	}

	fb.flags = V4L2_FBUF_FLAG_OVERLAY;
	fb.fmt.width = width;
	fb.fmt.height = height;
	fb.fmt.pixelformat = display->g_fmt_out;
	fb.fmt.bytesperline = width;
	
	ioctl (display->fd_output_v4l, VIDIOC_S_FBUF, &fb);
	
	memset (&fmt, 0, sizeof(fmt));
	
	fmt.type		 = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width	 = display->g_display_width;
	fmt.fmt.pix.height	 = display->g_display_height;
	fmt.fmt.pix.sizeimage	 = display->g_display_width * display->g_display_height * 2;
	fmt.fmt.pix.pixelformat  = display->g_fmt_out;
	fmt.fmt.pix.bytesperline = display->g_display_width * 2;
	fmt.fmt.pix.priv 	 = 0;
	fmt.fmt.pix.sizeimage 	 = 0;
	fmt.fmt.pix.field 	 = V4L2_FIELD_NONE;
	
	if (ioctl (display->fd_output_v4l, VIDIOC_S_FMT, &fmt) < 0)
	{
		printf("set format failed\n");
		return V4LWrapper_ERROR;
	}

	if (ioctl (display->fd_output_v4l, VIDIOC_G_FMT, &fmt) < 0)
	{
		printf("get format failed\n");
		return V4LWrapper_ERROR;
	}
	
	display->g_frame_size = fmt.fmt.pix.sizeimage;
	
	memset(&buf_req, 0, sizeof(buf_req));
	
	buf_req.count	= display->g_output_num_buffers;
	buf_req.type 	= V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf_req.memory	= V4L2_MEMORY_MMAP;
	
	if (ioctl (display->fd_output_v4l, VIDIOC_REQBUFS, &buf_req) < 0)
	{
		printf("request buffers failed\n");
		return V4LWrapper_ERROR;
	}
	
	//-------- framebuffer init
	if ((display->fd_fb = open (fb_device, O_RDWR )) < 0)
	{
		printf("Unable to open frame buffer\n");
		close (display->fd_output_v4l);
		
		return V4LWrapper_ERROR;
	}

	// Overlay setting
	alpha.alpha = 0;
	alpha.enable = 1;
	if (ioctl (display->fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0) 
	{
		printf("Set global alpha failed\n");
		
		close(display->fd_fb);
		close(display->fd_output_v4l);
		
		return V4LWrapper_ERROR;
	}

	if (V4LWrapper_OutputSetup (display) < 0)
	{
		printf("prepare_output failed\n");
		return V4LWrapper_ERROR;
	}


	return V4LWrapper_SUCCESS;
}

void V4LWrapper_CloseOutputDisplay (fsl_v4l_out *display)
{
	enum v4l2_buf_type type;
	int i;
	
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ioctl (display->fd_output_v4l, VIDIOC_STREAMOFF, &type);
	
	for (i = 0; i < NUM_OUT_BUFFERS; i++)
	{
		munmap(out_buffer[i].start, out_buffer[i].length);
	}
	
	close (display->fd_output_v4l);
	close (display->fd_fb);
	return;
}

int V4LWrapper_OutputSetup (fsl_v4l_out *display)
{
	int i;
	struct v4l2_buffer output_buf;

	for (i = 0; i < display->g_output_num_buffers; i++)
	{
		memset(&output_buf, 0, sizeof(output_buf));
		
		output_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		output_buf.memory = V4L2_MEMORY_MMAP;
		output_buf.index = i;
		
		if (ioctl (display->fd_output_v4l, VIDIOC_QUERYBUF, &output_buf) < 0)
		{
			printf("VIDIOC_QUERYBUF error\n");
			return V4LWrapper_ERROR;
		}
		out_buffer[i].length = output_buf.length;
		out_buffer[i].offset = (size_t) output_buf.m.offset;
		out_buffer[i].start = mmap (NULL, out_buffer[i].length,	PROT_READ | PROT_WRITE, MAP_SHARED, display->fd_output_v4l, out_buffer[i].offset);
		
		if (out_buffer[i].start == NULL) 
		{
			printf("v4l2 tvin test: output mmap failed\n");
			return V4LWrapper_ERROR;
		}
	}

	return V4LWrapper_SUCCESS;
}

int V4LWrapper_OutputDisplay (fsl_v4l_out *display, char *buffer)
{
	
	struct v4l2_buffer output_buf;
	static int i = 0;
	enum v4l2_buf_type type;
	

	memset (&output_buf, 0, sizeof(output_buf));
	
	output_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	output_buf.memory = V4L2_MEMORY_MMAP;
	
	if (i < display->g_output_num_buffers) 
	{
		output_buf.index = i;
		
		if (ioctl (display->fd_output_v4l, VIDIOC_QUERYBUF, &output_buf) < 0)
		{
			printf("VIDIOC_QUERYBUF failed\n");
			return V4LWrapper_ERROR;
		}
	
	} 
	else 
	{
		output_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		output_buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl (display->fd_output_v4l, VIDIOC_DQBUF, &output_buf) < 0)
		{
			printf("VIDIOC_DQBUF failed !!!\n");
			return V4LWrapper_ERROR;
		}
		
	}
	
	if (buffer != NULL)
	{
		memcpy (out_buffer[output_buf.index].start, buffer, display->g_frame_size);
		
	
		if (ioctl (display->fd_output_v4l, VIDIOC_QBUF, &output_buf) < 0)
		{
			printf("VIDIOC_QBUF failed\n");
			return V4LWrapper_ERROR;
		}
	}
	if (i == 1) 
	{
		type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		
		if (ioctl (display->fd_output_v4l, VIDIOC_STREAMON, &type) < 0) 
		{
			printf("Could not start stream\n");
			return V4LWrapper_ERROR;
		}
	}
	

	
	if (i > (display->g_output_num_buffers + 10)) 
		i = display->g_output_num_buffers+1;
	else i++;
		
	
	return V4LWrapper_SUCCESS;
}

int V4LWrapper_CreateCameraCapture (fsl_v4l_cap *capture, char *device, int width, int height)
{
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_dbg_chip_ident chip;
	struct v4l2_streamparm parm;
	v4l2_std_id id;
	
	struct v4l2_crop crop;

	capture->fd_capture_v4l		= 0;
	//capture->g_cap_mode		= 0;
	capture->g_input		= 1;
	capture->g_fmt_in		= V4L2_PIX_FMT_UYVY;
	//capture->g_rotate 		= 0;
	//capture->g_vflip 		= 0;
	//capture->g_hflip 		= 0;
	//capture->g_vdi_enable		= 0;
	//capture->g_vdi_motion		= 0;
	//capture->g_tb			= 0;
	capture->g_capture_num_buffers	= NUM_CAP_BUFFERS;
	capture->g_frame_size 		= 0;
	capture->g_in_width 		= width;
	capture->g_in_height 		= height;
	capture->g_current_std 		=  V4L2_STD_UNKNOWN; //V4L2_STD_NTSC

	// default is /dev/video0
	if (device == NULL)
		strcpy (capture->v4l_capture_dev, "/dev/video0");
	else
		strcpy (capture->v4l_capture_dev, device);
	
	
	if ((capture->fd_capture_v4l = open(capture->v4l_capture_dev, O_RDWR, 0)) < 0)
	{
		printf("\nUnable to open %s\n", capture->v4l_capture_dev);
		return V4LWrapper_ERROR;
	}
	
	if (ioctl(capture->fd_capture_v4l, VIDIOC_DBG_G_CHIP_IDENT, &chip))
	{
		printf("\nVIDIOC_DBG_G_CHIP_IDENT failed.\n");
		close(capture->fd_capture_v4l);
		return V4LWrapper_ERROR;
	}
	
	//printf("\nTV decoder chip is %s", chip.match.name);

	if (ioctl(capture->fd_capture_v4l, VIDIOC_S_INPUT, &capture->g_input) < 0)
	{
		printf("\nVIDIOC_S_INPUT failed\n");
		close(capture->fd_capture_v4l);
		return V4LWrapper_ERROR;
	}

	if (ioctl(capture->fd_capture_v4l, VIDIOC_G_STD, &id) < 0)
	{
		printf("\nVIDIOC_G_STD failed\n");
		close(capture->fd_capture_v4l);
		return V4LWrapper_ERROR;
	}
	
	capture->g_current_std = id;

	if (ioctl(capture->fd_capture_v4l, VIDIOC_S_STD, &id) < 0)
	{
		printf("\nVIDIOC_S_STD failed\n");
		close(capture->fd_capture_v4l);
		return V4LWrapper_ERROR;
	}

	// Select video input, video standard and tune 
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = 0;
	parm.parm.capture.capturemode = 0;
	
	if (ioctl (capture->fd_capture_v4l, VIDIOC_S_PARM, &parm) < 0)
	{
		printf("\nVIDIOC_S_PARM failed\n");
		close(capture->fd_capture_v4l);
		return V4LWrapper_ERROR;
	}
	
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c.left = 0;
	crop.c.top = 0;
	crop.c.width = width;
	crop.c.height = height;
	
	if (ioctl (capture->fd_capture_v4l, VIDIOC_S_CROP, &crop) < 0)
	{
		printf ("\nset cropping failed\n");
		return V4LWrapper_ERROR;
	}

	memset (&fmt, 0, sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = capture->g_in_width;
	fmt.fmt.pix.height      = capture->g_in_height;
	fmt.fmt.pix.sizeimage   = capture->g_in_width * capture->g_in_height * 2; 
	fmt.fmt.pix.bytesperline = capture->g_in_width * 2;
	fmt.fmt.pix.pixelformat = capture->g_fmt_in;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;//V4L2_FIELD_INTERLACED;

	if (ioctl (capture->fd_capture_v4l, VIDIOC_S_FMT, &fmt) < 0){
		fprintf (stderr, "\n%s iformat not supported \n", capture->v4l_capture_dev);
		return V4LWrapper_ERROR;
	}

	// Note VIDIOC_S_FMT may change width and height.
	// Buggy driver paranoia. 
	/*
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
*/
	if (ioctl(capture->fd_capture_v4l, VIDIOC_G_FMT, &fmt) < 0)
	{
		printf("\nVIDIOC_G_FMT failed\n");
		close(capture->fd_capture_v4l);
		return V4LWrapper_ERROR;
	}

	capture->g_frame_size = fmt.fmt.pix.sizeimage;

	memset(&req, 0, sizeof (req));

	req.count               = capture->g_capture_num_buffers;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (ioctl (capture->fd_capture_v4l, VIDIOC_REQBUFS, &req) < 0) 
	{
		if (EINVAL == errno) 
		{
			fprintf (stderr, "\n%s does not support memory mapping\n", capture->v4l_capture_dev);
			return V4LWrapper_ERROR;
		} 
		else 
		{
			fprintf (stderr, "\n%s does not support memory mapping, unknow error\n", capture->v4l_capture_dev);
			return V4LWrapper_ERROR;
		}
	}

	if (req.count < 2) 
	{
		fprintf (stderr, "\nInsufficient buffer memory on %s\n", capture->v4l_capture_dev);
		return V4LWrapper_ERROR;
	}

	return V4LWrapper_SUCCESS;
}

void V4LWrapper_CloseCameraCapture (fsl_v4l_cap *capture)
{
	enum v4l2_buf_type type;
	int i;
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl (capture->fd_capture_v4l, VIDIOC_STREAMOFF, &type);

	for (i = 0; i < NUM_CAP_BUFFERS; i++)
	{
		munmap(cap_buffer[i].start, cap_buffer[i].length);
	}
	
	close (capture->fd_capture_v4l);
	
	return;
}


int V4LWrapper_StartCapturing (fsl_v4l_cap *capture)
{
        unsigned int i;
        struct v4l2_buffer buf;
        enum v4l2_buf_type type;

        for (i = 0; i < capture->g_capture_num_buffers; i++)
        {
                memset (&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                if (ioctl (capture->fd_capture_v4l, VIDIOC_QUERYBUF, &buf) < 0)
                {
                        printf("VIDIOC_QUERYBUF error\n");
                        return V4LWrapper_ERROR;
                }

                cap_buffer[i].length = buf.length;
                cap_buffer[i].offset = (size_t) buf.m.offset;
                cap_buffer[i].start = mmap (NULL, cap_buffer[i].length, PROT_READ | PROT_WRITE, MAP_SHARED,capture->fd_capture_v4l, cap_buffer[i].offset);
		
		memset (cap_buffer[i].start, 0xFF, cap_buffer[i].length);
	}

	for (i = 0; i < capture->g_capture_num_buffers; i++)
	{
		memset (&buf, 0, sizeof (buf));
		
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		buf.m.offset = cap_buffer[i].offset;
		
		if (ioctl (capture->fd_capture_v4l, VIDIOC_QBUF, &buf) < 0) 
		{
			printf("VIDIOC_QBUF error\n");
			return V4LWrapper_ERROR;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl (capture->fd_capture_v4l, VIDIOC_STREAMON, &type) < 0)
	{
		printf("VIDIOC_STREAMON error\n");
		return V4LWrapper_ERROR;
	}
	
	return V4LWrapper_SUCCESS;
}

int V4LWrapper_QueryFrame (fsl_v4l_cap *capture, char *buffer)
{
	
	static int i = 0;
	struct v4l2_buffer capture_buf;

	if (i == 0)
	{
		if (V4LWrapper_StartCapturing (capture) < 0)
		{
			printf("start_capturing failed\n");
			return V4LWrapper_ERROR;
		}
		i = 1;
	}

	else
	{
		memset (&capture_buf, 0, sizeof(capture_buf));
		capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		capture_buf.memory = V4L2_MEMORY_MMAP;
		
		if (ioctl (capture->fd_capture_v4l, VIDIOC_DQBUF, &capture_buf) < 0) 
		{
			printf("VIDIOC_DQBUF failed.\n");
			return V4LWrapper_ERROR;
		}
		
		if (buffer != NULL)
		{
			memcpy (buffer, cap_buffer[capture_buf.index].start, capture->g_frame_size);
		}
		
		if (ioctl (capture->fd_capture_v4l, VIDIOC_QBUF, &capture_buf) < 0) 
		{
			printf("VIDIOC_QBUF failed\n");
			return V4LWrapper_ERROR;
		}
	}

	return V4LWrapper_SUCCESS;
}

int V4LWrapper_CvtColor (char *buffer_in, char *buffer_out, int width, int height, int colorflag)
{
	float r0, g0, b0;
	float r1, g1, b1;
	float r2, g2, b2;
	float r3, g3, b3;
	
	int i = 0;
	int j = 0;
	
	long imagesize = width * height;
	
	if (colorflag == RGB888toYUV422)
	{
		float Y0, U0;
		float Y1, V1;
		float Y2, U2;
		float Y3, V3;
		
		for (i = 0; i < imagesize; i+=12)
		{
			r0 = buffer_in[i];
			g0 = buffer_in[i + 1];
			b0 = buffer_in[i + 2];
			
			r1 = buffer_in[i + 3];
			g1 = buffer_in[i + 4];
			b1 = buffer_in[i + 5];

			r2 = buffer_in[i + 6];
			g2 = buffer_in[i + 7];
			b2 = buffer_in[i + 8];
			
			r3 = buffer_in[i + 9];
			g3 = buffer_in[i + 10];
			b3 = buffer_in[i + 11];

			// RGB888 to YUV444
			Y0  = (0.299 * r0) + (0.587 * g0) + (0.114 * b0);
			U0 = (-0.169 * r0) - (0.331 * g0) + (0.499 * b0) + 128;
			//V0 = (0.499 * r0) - (0.418 * g0) - (0.0813 * b0) + 128;

			Y1  = (0.299 * r1) + (0.587 * g1) + (0.114 * b1);
			//U1 = (-0.169 * r1) - (0.331 * g1) + (0.499 * b1) + 128;
			V1 = (0.499 * r1) - (0.418 * g1) - (0.0813 * b1) + 128;

			Y2  = (0.299 * r2) + (0.587 * g2) + (0.114 * b2);
			U2 = (-0.169 * r2) - (0.331 * g2) + (0.499 * b2) + 128;
			//V2 = (0.499 * r2) - (0.418 * g2) - (0.0813 * b2) + 128;

			Y3  = (0.299 * r3) + (0.587 * g3) + (0.114 * b3);
			//U3 = (-0.169 * r3) - (0.331 * g3) + (0.499 * b3) + 128;
			V3 = (0.499 * r3) - (0.418 * g3) - (0.0813 * b3) + 128;
			
			if (Y0 > 255) Y0 = 255;
			if (Y0 < 0) Y0 = 0;
			if (U0 > 255) U0 = 255;
			if (U0 < 0) U0 = 0;
			
			if (Y1 > 255) Y1 = 255;
			if (Y1 < 0) Y1 = 0;
			if (V1 > 255) V1 = 255;
			if (V1 < 0) V1 = 0;

			if (Y2 > 255) Y2 = 255;
			if (Y2 < 0) Y2 = 0;
			if (U2 > 255) U2 = 255;
			if (U2 < 0) U2 = 0;

			if (Y3 > 255) Y3 = 255;
			if (Y3 < 0) Y3 = 0;
			if (V3 > 255) V3 = 255;
			if (V3 < 0) V3 = 0;
			
			buffer_out[j] 		= round (U0);
			buffer_out[j + 1] 	= round (Y0);
			buffer_out[j + 2] 	= round (V1);
			buffer_out[j + 3] 	= round (Y1);
			
			buffer_out[j + 4] 	= round (U2);
			buffer_out[j + 5] 	= round (Y2);
			buffer_out[j + 6] 	= round (V3);
			buffer_out[j + 7] 	= round (Y3);
			j = j + 8;
		}
	}
	
	else if (colorflag == YUV422toRGB888)
	{
		float Y0, U0, V0;
		float Y1, U1, V1;
		float Y2, U2, V2;
		float Y3, U3, V3;

		unsigned char u0, y0, v1, y1, u2, y2, v3, y3;
		
		for (j = 0; j < imagesize; j+=8)
		{
			u0 = buffer_in[j];
			y0 = buffer_in[j + 1];
			v1 = buffer_in[j + 2];
			y1 = buffer_in[j + 3];

			u2 = buffer_in[j + 4];
			y2 = buffer_in[j + 5];
			v3 = buffer_in[j + 6];
			y3 = buffer_in[j + 7];

			// YUV422 to YUV444
			Y0 = y0;
			U0 = u0;
			V0 = v1;
			
			Y1 = y1;
			U1 = u0;
			V1 = v1;
			
			Y2 = y2;
			U2 = u2;
			V2 = v3;
			
			Y3 = y3;
			U3 = u2;
			V3 = v3;
			
			r0 = (0.997 * Y0) - (0.007 * U0) + (1.405 * V0) - 189.114;
			g0 = Y0 - (0.343 * U0) - (0.716 * V0) + 135.46;
			b0 = (1.006 * Y0) + (1.780 * U0) - 227.81;

			r1 = (0.997 * Y1) - (0.007 * U1) + (1.405 * V1) - 189.114;
			g1 = Y1 - (0.343 * U1) - (0.716 * V1) + 135.46;
			b1 = (1.006 * Y1) + (1.780 * U1) - 227.810;

			
			r2 = (0.997 * Y2) - (0.007 * U2) + (1.405 * V2) - 189.114;
			g2 = Y2 - (0.343 * U2) - (0.716 * V2) + 135.46;
			b2 = (1.006 * Y2) + (1.780 * U2) - 227.81;

			r3 = (0.997 * Y3) - (0.007 * U3) + (1.405 * V3) - 189.114;
			g3 = Y3 - (0.343 * U3) - (0.716 * V3) + 135.46;
			b3 = (1.006 * Y3) + (1.780 * U3) - 227.81;

			if (r0 > 255) r0 = 255;
			if (r0 < 0) r0 = 0;
			if (g0 > 255) g0 = 255;
			if (g0 < 0) g0 = 0;
			if (b0 > 255) b0 = 255;
			if (b0 < 0) b0 = 0;

			if (r1 > 255) r1 = 255;
			if (r1 < 0) r1 = 0;
			if (g1 > 255) g1 = 255;
			if (g1 < 0) g1 = 0;
			if (b1 > 255) b1 = 255;
			if (b1 < 0) b1 = 0;

			if (r2 > 255) r2 = 255;
			if (r2 < 0) r2 = 0;
			if (g2 > 255) g2 = 255;
			if (g2 < 0) g2 = 0;
			if (b2 > 255) b2 = 255;
			if (b2 < 0) b2 = 0;
			
			if (r3 > 255) r3 = 255;
			if (r3 < 0) r3 = 0;
			if (g3 > 255) g3 = 255;
			if (g3 < 0) g3 = 0;
			if (b3 > 255) b3 = 255;
			if (b3 < 0) b3 = 0;

			buffer_out[i] 		= round (r0);
			buffer_out[i + 1] 	= round (g0);
			buffer_out[i + 2] 	= round (b0);
			buffer_out[i + 3]	= round (r1);
			buffer_out[i + 4]	= round (g1);
			buffer_out[i + 5]	= round (b1);
			buffer_out[i + 6] 	= round (r2);
			buffer_out[i + 7]	= round (g2);
			buffer_out[i + 8]	= round (b2);
			buffer_out[i + 9] 	= round (r3);
			buffer_out[i + 10]	= round (g3);
			buffer_out[i + 11]	= round (b3);
			i = i  + 12;
		}
	}
	
	else
	{
		printf ("\nFormat not supported\n");
		return V4LWrapper_ERROR;
	}
	
	return V4LWrapper_SUCCESS;
}