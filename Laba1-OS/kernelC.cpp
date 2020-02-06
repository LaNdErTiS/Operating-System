__asm("jmp kmain");
#define VIDEO_BUF_PTR (0xb8000)

int get_color(char num) {
	switch (num) {
	case '1': return 0x7;
	case '2': return 0x9;
	case '3': return 0x4;
	case '4': return 0xe;
	case '5': return 0x8;
	case '6': return 0x2;
	}
}

void out_str(int color, const char* ptr, unsigned int strnum)
{
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * strnum;
	while (*ptr)
	{
		video_buf[0] = (unsigned char)*ptr; 
		video_buf[1] = color; 
		video_buf += 2;
		ptr++;
	}
}
const char* g_test = "This is test string.";
extern "C" int kmain()
{
	unsigned char color = *(unsigned char*)(0x9FF0);
	const char* hello = "Welcome to HelloWorldOS (gcc edition)!";
	out_str(get_color(color), hello, 0);
	out_str(get_color(color), g_test, 1);

	while (1)
	{
		asm("hlt");
	}
	return 0;
}