__asm("jmp kmain");
#define VIDEO_BUF_PTR (0xb8000)
#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
// Селектор секции кода, установленный загрузчиком ОС
#define GDT_CS (0x8)
#define PIC1_PORT (0x20)
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) // Ширина текстового экрана

int line = 0, column = 0;
unsigned char color;
char buffer[40] = { 0 };
int count = 0; int shift = 1;


void printf(const char* ptr);
void shutdown();
void check_buffer();
void control_lines();
void clear_screen();
// Структура описывает данные об обработчике прерывания
struct idt_entry
{
	unsigned short base_lo; // Младшие биты адреса обработчика
	unsigned short segm_sel; // Селектор сегмента кода
	unsigned char always0; // Этот байт всегда 0
	unsigned char flags; // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
	unsigned short base_hi; // Старшие биты адреса обработчика
} __attribute__((packed)); // Выравнивание запрещено
// Структура, адрес которой передается как аргумент команды lidt
struct idt_ptr
{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed)); // Выравнивание запрещено
struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt
// Пустой обработчик прерываний. Другие обработчики могут быть реализованы по этому шаблону
void default_intr_handler()
{
	asm("pusha");
	// ... (реализация обработки)
	asm("popa; leave; iret");
}
typedef void(*intr_handler)();
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr)
{
	unsigned int hndlr_addr = (unsigned int)hndlr;
	g_idt[num].base_lo = (unsigned short)(hndlr_addr & 0xFFFF);
	g_idt[num].segm_sel = segm_sel;
	g_idt[num].always0 = 0;
	g_idt[num].flags = flags;
	g_idt[num].base_hi = (unsigned short)(hndlr_addr >> 16);
}
// Функция инициализации системы прерываний: заполнение массива с адресами обработчиков
void intr_init()
{
	int i;
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	for (i = 0; i < idt_count; i++)
		intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
			default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}
void intr_start()
{
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	g_idtp.base = (unsigned int)(&g_idt[0]);
	g_idtp.limit = (sizeof(struct idt_entry) * idt_count) - 1;
	asm("lidt %0" : : "m" (g_idtp));
}
void intr_enable()
{
	asm("sti");
}
void intr_disable()
{
	asm("cli");
}

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

void clear_buffer() {
	for (int i = 0; i < 40; i++) {
		buffer[i] = 0;
	}
	count = 0;
}

static inline unsigned char inb(unsigned short port) // Чтение из порта
{
	unsigned char data;
	asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
	return data;
}
static inline void outb(unsigned short port, unsigned char data) // Запись
{
	asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline void outw(unsigned int port, unsigned int data) {
	asm volatile ("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

void cursor_moveto(unsigned int strnum, unsigned int pos)
{
	unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
	outb(CURSOR_PORT, 0x0F);
	outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
	outb(CURSOR_PORT, 0x0E);
	outb(CURSOR_PORT + 1, (unsigned char)((new_pos >> 8) & 0xFF));
}

char scan_table_symbols_shift[]{
	'0', //0
	'0', //ESC -- 1
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	'0', //BKSP -- 14
	'0', //TAB -- 15
	//'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
	'0', //ENTER -- 28
	'0', //LCTRL -- 29
	//'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	'0', //LSHIFT -- 42
	//'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
	'0', // RSHIFT -- 54
	'0', // LALT, RALT -- 56
	'0', // SPACE -- 57
	'0', // CAPS -- 58
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', //F1--F10 -- 59 -- 68
	'0', // NUM Lock -- 69
	'0', // scrl lock -- 70
	'0', // home -- 71
	'0', // up arrow -- 72
};

char scancode_to_ascii[] = {
  0,  //0 = ?
  0,  //1 = ESC
  '1',  //0x02
  '2',  //0x03
  '3',  //0x04
  '4',  //0x05
  '5',  //0x06
  '6',  //0x07
  '7',  //0x08
  '8',  //0x09
  '9',  //0x0A
  '0',  //0x0B
  '-',  //0x0C
  '=',  //0x0D
  '\b', //0x0E
  '\t', //0x0F
  'q',  //0x10
  'w',  //0x11
  'e',  //0x12
  'r',  //0x13
  't',  //0x14
  'y',  //0x15
  'u',  //0x16
  'i',  //0x17
  'o',  //0x18
  'p',  //0x19
  '[',  //0x1A
  ']',  //0x1B
  '\n', //0x1C = ENTER
  0,    //0x1D = LEFT CTRL
  'a',  //0x1E
  's',  //0x1F
  'd',  //0x20
  'f',  //0x21
  'g',  //0x22
  'h',  //0x23
  'j',  //0x24
  'k',  //0x25
  'l',  //0x26
  ';',  //0x27
  '\'', //0x28
  '`',  //0x29
  0,    //0x2A = LEFT SHIFT
  '\\', //0x2B
  'z',  //0x2C
  'x',  //0x2D
  'c',  //0x2E
  'v',  //0x2F
  'b',  //0x30
  'n',  //0x31
  'm',  //0x32
  ',',  //0x33
  '.',  //0x34
  '/',  //0x35
  0,    //0x36 = RIGHT SHIFT
  '*',  //0x37 = KEYPAD *
  0,   //0x38 = LEFT ALT
  ' ', //0x39 = SPACE
};

void out_ch(char ptr) {
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR + 2 * column;
	video_buf += 80 * 2 * line;
	buffer[count] = video_buf[0] = (unsigned char)ptr; count++;
	video_buf[1] = get_color(color);
	video_buf[2] = ' ';
	column++;
	cursor_moveto(line, column);
}

void info() {
	printf("VSKiZI 2020\n");
	control_lines();
	printf("Developer is Ivan Gavalidi\n");
	control_lines();
	printf("Asm interprete: YASM\n");
	control_lines();
	printf("GCC\n");
	control_lines();
	switch (get_color(color)) {
	case (0x02):
		printf("color is green\n");
		break;
	case (0x01):
		printf("color is blue\n");
		break;
	case (0x04):
		printf("color is red\n");
		break;
	case (0x0E):
		printf("color is yellow\n");
		break;
	case (0x08):
		printf("color is grey\n");
		break;
	case (0x07):
		printf("color is white\n");
		break;
	}
	control_lines();
}

void enter() {
	line++; column = 0;
	control_lines();
	check_buffer();
	printf("user@user:~$ ");
}

void control_lines() {
	if (line > 24) {
		clear_screen();
	}
}

void backspace() {
	if (column >= 14) {
		column--; count--; buffer[count] = 0;
		unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR + 2 * column;
		video_buf += 80 * 2 * line;
		video_buf[0] = ' ';
		cursor_moveto(line, column);
	}
}

int strcmp(char*str1, char *str2) {
	int i = 0;
	for (i = 0; i < 40; i++) {
		if (str1[i] == 0 && str2[i] == 0) {
			break;
		}
		if ((str1[i] - str2[i] != 0)) {
			return str1[i] - str2[i];
		}
	}
	return 0;
}

void clear_screen() {
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	int i;
	for (i = 0; i < 80 * 2 * 40; i++) {
		video_buf[i] = 0;
	}
	line = 0;
	column = 0;
	cursor_moveto(line, column);
}

void check_buffer() {
	if (!strcmp(buffer, "shutdown")) {
		shutdown();
	}
	else
	{
		if (!strcmp(buffer, "info"))
			info();
		else {
			if (!strcmp(buffer, "clear"))
				clear_screen();
		}
	}
	clear_buffer();
}

void on_key(unsigned char code) {
	switch (code)
	{
	case 14:
		backspace();
		break;
	case 28:
		enter();
		break;
	case 42:
		shift *= -1;
		break;
	default:
		if (count < 40) {
			if (shift < 0)
				out_ch(scan_table_symbols_shift[code]);
			else
				out_ch(scancode_to_ascii[code]);
		}
		break;
	}
}

void keyb_process_keys()
{
	// Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует)
	if (inb(0x64) & 0x01)
	{
		unsigned char scan_code;
		unsigned char state;
		scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры
		if (scan_code < 128) // Скан-коды выше 128 - это отпускание клавиши
			on_key(scan_code);
	}
}

void keyb_handler()
{
	asm("pusha");
	// Обработка поступивших данных
	keyb_process_keys();
	// Отправка контроллеру 8259 нотификации о том, что прерывание обработано
	outb(PIC1_PORT, 0x20);
	asm("popa; leave; iret");
}

void keyb_init()
{
	// Регистрация обработчика прерывания
	intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
	// Разрешение только прерываний клавиатуры от контроллера 8259
	outb(PIC1_PORT + 1, 0xFF ^ 0x02); // 0xFF - все прерывания, 0x02 - бит IRQ1 (клавиатура).
	// Разрешены будут только прерывания, чьи биты установлены в 0
}

void printf(const char* ptr)
{
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * line;
	while (*ptr)
	{
		if ((unsigned char)*ptr == '\n') { line++; column = 0; ptr++; continue; }
		else {
			column++;
		}
		video_buf[0] = (unsigned char)*ptr; // Символ (код)
		video_buf[1] = get_color(color); // Цвет символа и фона
		video_buf += 2;
		ptr++;
	}
	cursor_moveto(line, column);
}

void shutdown() {
	//outw(0xB004, 0x2000);//qemu <1.7
	outw(0x604, 0x2000);//qemu>=1.7
}

extern "C" int kmain()
{
	color = *(unsigned char*)(0x9FF0);
	const char* hello = "Welcome to ConvertOs!\n";
	// Вывод строки
	printf(hello);
	printf("user@user:~$ ");
	intr_init();
	intr_start();
	intr_enable();
	keyb_init();
	// Бесконечный цикл
	while (1)
	{
		asm("hlt");
	}
	return 0;
}