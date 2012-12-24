#include "common.h"
#define OUTPUT_BUF_SIZE  4096

#define v4l_ver(words) v4l2_##words

#define v4l_ver_fmtdesc 		v4l_ver(fmtdesc) 
#define v4l_ver_capability 		v4l_ver(capability)
#define v4l_ver_std_id 			v4l_ver(std_id)
#define v4l_ver_format 			v4l_ver(format)
#define v4l_ver_requestbuffers 	v4l_ver(requestbuffers)
#define v4l_ver_buffer 			v4l_ver(buffer)
#define v4l_ver_buf_type 		v4l_ver(buf_type)

typedef struct VideoBuffer{
	void *start;
	size_t length;
}VideoBuffer;

const int video_height = 240;
const int video_width = 320;

static struct v4l_ver_fmtdesc fmtdesc;
static struct v4l_ver_capability capability;
//static v4l_ver_std_id v4l2_std;
static struct v4l_ver_format fmt;
static struct v4l_ver_requestbuffers v4l2_reqbuf;
static struct v4l_ver_buffer v4l2_buf;
static enum v4l_ver_buf_type v4l2_type;

static struct timeval tv;
static int camerafd = -1;
static fd_set fds;
static VideoBuffer *buffers;
static VideoBuffer current_buffer;
SEND_BUFFER send_buffer;
JPEG jpeg;

typedef struct {
	struct jpeg_destination_mgr pub;
	JOCTET * buffer;
	unsigned char *outbuffer;
	int outbuffer_size;
	unsigned char *outbuffer_cursor;
	int *written;
} mjpg_destination_mgr;

typedef mjpg_destination_mgr *mjpg_dest_ptr;
static struct jpeg_compress_struct cinfo;
static struct jpeg_error_mgr jerr;
static JSAMPROW row_pointer[1];
static unsigned char *src_jpeg;
static unsigned char *dest_jpeg;
static int compress_ratio = 80;   //define compress ratio.

METHODDEF(void) init_destination(j_compress_ptr cinfo) {
	mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
	dest->buffer = (JOCTET *) (*cinfo->mem->alloc_small)((j_common_ptr) cinfo,
			JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));
	*(dest->written) = 0;
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo) {
	mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
	memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
	dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
	*(dest->written) += OUTPUT_BUF_SIZE;
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return TRUE;
}

METHODDEF(void) term_destination(j_compress_ptr cinfo) {
	mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
	/* Write any data remaining in the buffer */
	memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
	dest->outbuffer_cursor += datacount;
	*(dest->written) += datacount;
}

static void dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size,int *written) {
	mjpg_dest_ptr dest;
	if (cinfo->dest == NULL) {
		cinfo->dest = (struct jpeg_destination_mgr *) (*cinfo->mem->alloc_small)(
						(j_common_ptr) cinfo, JPOOL_PERMANENT,
						sizeof(mjpg_destination_mgr));
	}
	dest = (mjpg_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outbuffer = buffer;
	dest->outbuffer_size = size;
	dest->outbuffer_cursor = buffer;
	dest->written = written;
}

static int compress_yuyv_to_jpeg(unsigned char *primitive_data, unsigned char *compressed_data, int num_pixel, int quality)
{
//	int ret;
	int	z;
	unsigned char *line_buffer, *yuyv;
	static int written;
	
	memcpy(src_jpeg, current_buffer.start, current_buffer.length);
	line_buffer = calloc(video_width * 3, 1);
	assert(line_buffer);
	yuyv = primitive_data;
	//printf("compress start...\n");
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	/* jpeg_stdio_dest (&cinfo, file); */
	dest_buffer(&cinfo, compressed_data, num_pixel, &written);
	cinfo.image_width = video_width;
	cinfo.image_height = video_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	z = 0;
	while (cinfo.next_scanline < video_height) {
		int x;
		unsigned char *ptr = line_buffer;

		for (x = 0; x < video_width; x++) {
			int r, g, b;
			int y, u, v;
			if (!z)
				y = yuyv[0] << 8;
			else
				y = yuyv[2] << 8;
			u = yuyv[1] - 128;
			v = yuyv[3] - 128;

			r = (y + (359 * v)) >> 8;
			g = (y - (88 * u) - (183 * v)) >> 8;
			b = (y + (454 * u)) >> 8;

			*(ptr++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
			*(ptr++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
			*(ptr++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);

			if (z++) {
				z = 0;
				yuyv += 4;
			}
		}

		row_pointer[0] = line_buffer;
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	free(line_buffer);

	if (9999999 < written){
		printf("the jpg file is too large!\n");
		return 0;	
	}
	
	char send_buffer_head[10];
	memset(send_buffer_head, 0, sizeof(send_buffer_head));
	sprintf(send_buffer_head + 1, "%d",written);
	//printf("%s : size of jpg file is %s\n", __FUNCTION__, send_buffer_head + 1);
	unsigned char bitsoflength = strlen(send_buffer_head + 1) + '0';
	//printf("bitsoflength is %c\n", bitsoflength);
	memcpy(send_buffer_head, &bitsoflength, 1);
	//printf("send_buffer_head is %s\n", send_buffer_head);
	//printf("%s : send_buffer_head is %s, its length is %d bytes\n", __FUNCTION__, send_buffer_head, strlen(send_buffer_head));
	memcpy(send_buffer.start, send_buffer_head, strlen(send_buffer_head));
	memcpy(send_buffer.start + strlen(send_buffer_head), dest_jpeg, written);
	jpeg.start = dest_jpeg;
	jpeg.length = written;
	send_buffer.length = strlen(send_buffer_head) + written;
	return (written);
}

int init_video_device(void)
{
	int ret, num_bufs;
	camerafd = open("/dev/video0",O_RDWR);
	if (-1 == camerafd){
		perror("open");
		exit(EXIT_FAILURE);
	}
	//get the format supported by the camera
	memset(&fmtdesc, 0, sizeof(fmtdesc));
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while(0 == (ret = ioctl(camerafd, VIDIOC_ENUM_FMT, &fmtdesc))){
		fmtdesc.index++;
		printf("pixelformat = %c%c%c%c, description = %s\r\n",fmtdesc.pixelformat&0xFF, (fmtdesc.pixelformat>>8)&0xFF, (fmtdesc.pixelformat>>16)&0xFF, (fmtdesc.pixelformat>>24)&0xFF, fmtdesc.description);
	}
	
	if (-1 == ioctl(camerafd, VIDIOC_QUERYCAP,&capability)){
		perror("ioctl: VIDIOC_QUERYCAP");
		exit(EXIT_FAILURE);
	}
	
	if (!(capability.capabilities&V4L2_CAP_VIDEO_CAPTURE)){
		printf("V4L2_CAP_VIDEO_CAPTURE failed!\n");
		exit(EXIT_FAILURE);
	}
	
	if (!(capability.capabilities&V4L2_CAP_STREAMING)){
		printf("V4L2_CAP_STREAMING failed!\n");
		exit(EXIT_FAILURE);
	}
	//set the frame format.
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.height = video_height;
	fmt.fmt.pix.width = video_width;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	ret = ioctl(camerafd, VIDIOC_S_FMT,&fmt);
	
	if (ret){
		perror("ioctl:VIDIOC_S_FMT");
		close(camerafd);
		exit(EXIT_FAILURE);
	}

	int file_length = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	printf("the size of image is 0x%x bytes\n", file_length);
	printf("bytes per line is %d, height is %d, width is %d\n", fmt.fmt.pix.bytesperline,fmt.fmt.pix.height, fmt.fmt.pix.width);
	//require the buffers from  camera.
	memset(&v4l2_reqbuf, 0, sizeof(struct v4l2_requestbuffers));
	v4l2_reqbuf.count = 4;
	v4l2_reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_reqbuf.memory = V4L2_MEMORY_MMAP;
	if (-1 == (ioctl(camerafd, VIDIOC_REQBUFS, &v4l2_reqbuf))){
		perror("ioctl:VIDIOC_REQBUFS");
		close(camerafd);
		exit(EXIT_FAILURE);
	}
	printf("number of buffers : %d\n", v4l2_reqbuf.count);
	//mmap the buffer required.
	buffers = calloc(v4l2_reqbuf.count, sizeof(VideoBuffer));
	for (num_bufs = 0; num_bufs < v4l2_reqbuf.count; num_bufs++){
		memset(&v4l2_buf,0,sizeof(struct v4l2_buffer));
		v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		v4l2_buf.index = num_bufs;
		if (-1 == ioctl(camerafd, VIDIOC_QUERYBUF,&v4l2_buf)){
			perror("ioctl:VIDIOC_QUERYBUF");
			exit(EXIT_FAILURE);
		}
		buffers[num_bufs].start = mmap(NULL, v4l2_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, camerafd, v4l2_buf.m.offset);
		if (MAP_FAILED == buffers[num_bufs].start){
			printf("mmap %d", num_bufs);
			exit(EXIT_FAILURE);
		}
		buffers[num_bufs].length = v4l2_buf.length;
	}
	// put the buffers to the waiting queue of the camera!
	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;
	for (num_bufs = 0; num_bufs < v4l2_reqbuf.count; num_bufs++){
		v4l2_buf.index = num_bufs;
		ret = ioctl(camerafd, VIDIOC_QBUF, &v4l2_buf);
		if (ret){
			perror("ioctl : VIDIOC_QBUF");
			close(camerafd);
			exit(EXIT_FAILURE);
		}	
	}
	return 0;
}

int ready_for_capture(void)
{
	int ret;
	//init buffer for jpeg.
	src_jpeg = (unsigned char *)malloc(video_width * video_height * 2 + 1);
	dest_jpeg = (unsigned char *)malloc(video_width * video_height * 2 +1);
	
	send_buffer.start = (unsigned char *)malloc(video_width * video_height * 2 + 1 + 1 + 10); // 1 for length of size of jpg file.10 for characters for size of jpeg file.
	assert(send_buffer.start);
	assert(src_jpeg);
	assert(dest_jpeg);
	//printf("%s : %d  address of src_jpeg is 0x%x\n", __FUNCTION__, __LINE__, src_jpeg);
	//printf("%s : %d  address of send_buffer.start is 0x%x\n", __FUNCTION__, __LINE__, send_buffer.start);
	//printf("%s : %d  address of dest_jpeg is 0x%x\n", __FUNCTION__, __LINE__, dest_jpeg);
	
	v4l2_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(camerafd, VIDIOC_STREAMON, &v4l2_type);
	if (ret){
		perror("ioctl : VIDIOC_STREAMON");
		close(camerafd);
		exit(EXIT_FAILURE);
	}
	return 0;
}

SEND_BUFFER *get_send_buffer(void)
{
	//start to capture!
	//printf("start to capture...\n");
	int ret;
	FD_ZERO(&fds);
	FD_SET(camerafd, &fds);
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	ret = select(camerafd + 1, &fds, NULL, NULL, &tv);
	if (ret < 0) {
		perror("fail to select!");
		exit(EXIT_FAILURE);
	}	

	if (-1 == ioctl(camerafd, VIDIOC_DQBUF, &v4l2_buf)) {
		perror("ioctl: VIDIOC_DQBUF");
		exit(EXIT_FAILURE);
	}
	//printf("the buffer %d is ready, its length is 0x%x\n", v4l2_buf.index, v4l2_buf.length);
	current_buffer.start = buffers[v4l2_buf.index].start;
	current_buffer.length = buffers[v4l2_buf.index].length;

	compress_yuyv_to_jpeg(src_jpeg, dest_jpeg, video_width * video_height, compress_ratio);
	//write_fb(current_buffer.start);
	if (-1 == ioctl(camerafd, VIDIOC_QBUF, &v4l2_buf)){
		perror("ioctl: VIDIOC_QBUF"); 
		exit(EXIT_FAILURE);
	}
	//usleep(10);  // for A8, there is no delay; for PC, this should be uncommented! 
	return &send_buffer;
}

int close_video(void)
{
	int ret = ioctl(camerafd, VIDIOC_STREAMOFF, &v4l2_type); 
	if (ret){ 
		printf("ioctl : VIDIOC_STREAMOFF"); 
	 	exit(EXIT_FAILURE); 
	}
	close(camerafd);
	if (send_buffer.start){
		free(send_buffer.start);		
	}
	if (src_jpeg){
		free(src_jpeg);
	}
	printf("video closed...\n");
	return 0;
}


