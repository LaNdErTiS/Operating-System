__asm("jmp kmain");
#define VIDEO_BUF_PTR (0xb8000)
#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
// Селектор секции кода, установленный загрузчиком ОС
#define GDT_CS (0x8)
#define PIC1_PORT (0x20)
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) // Ширина текстового экрана

int line =0, column=0;
unsigned char color;
char buffer[40]={0}; char base[25]={0};
int count = 0;int shift=1;

void printf(const char* ptr);
void shutdown();
void check_buffer();
void control_lines();
void clear_screen();
int strcmp_len(char *str1, char *str2, int len);
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

void clear_buffer(){
	for(int i =0;i<40;i++){
		buffer[i]=0;
	}
	count=0;
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
	'0', 
	'0', 
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	'0', 
	'0', 
	'0', 
	'0', 
	'0', 
	'0', 
	'0', 
	'0', 
	'0', 
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 
	'0', 
	'0', 
	'0', 
	'0', 
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
	case (0x09):
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
	line++;column=0;
	control_lines();
	check_buffer();
	printf("user@user:~$ ");
}

void control_lines(){
	if(line>=24){
		clear_screen();
	}
}

void backspace() {
	if (column>=14) {
		column--;count--;buffer[count]=0;
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

void clear_screen(){
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	int i;
	for (i = 0; i < 80 * 2 * 40; i++) {
		video_buf[i] = 0;
	}
	line = 0;
	column = 0;
	cursor_moveto(line, column);
}

bool isDigit(char c) {
    return '0' <= c && c <= '9';
}

long long getNumber(char *strEcxp, int iter) {
	int max=0;
	if(iter == 10){max=10;}
	if(iter == 8){max=18;}
    unsigned long long res = 0;
    int countNumber = 0;
    int i = 0;
 
    while (strEcxp[(iter)] == ' ') {
        (iter)++;
    }
 
    if (strEcxp[(iter)] == '\0') {
        return -1;
    }
 
    while (isDigit(strEcxp[(iter)])) {
        	countNumber++; (iter)++;
	}
	int temp = iter;
	while (strEcxp[(temp)] == ' ') {
        (temp)++;
    }

	if( countNumber>max){
		return -2;
	}

	if ((strEcxp[(temp)] != '\0' && temp!=count) || countNumber==0) {
        return -1;
    }
 
    for (i = countNumber - 1; i >= 0; i--) {
        res += strEcxp[(iter) - i - 1] - '0';
        if (countNumber > 1 && i > 0){
            res *= 10;
        }
    }
 
    return res;
}

void itoac (unsigned long num) {
char str[25]={0};  int col=0;
unsigned long temp = num, raz=1;
 while(temp){
	 raz*=10;
	 temp/=10;col++;
 }
 raz/=10;int i =0;

 if(col==1){
	 str[i]='0';i++;col++;
 }

 for(;i<col;i++){
	 str[i]='0'+num/raz;
	 num = num - (num/raz)*raz;raz/=10;
 }

str[i]='\0';
printf(str);
}

int masMonth[]{
	31,28, 31,30,31, 30, 31, 31, 30, 31, 30, 31,
};

struct tm
{       /* date and time components */
	long	tm_sec;
	long	tm_min;
	long	tm_hour;
	long	tm_mday;
	long	tm_mon;
	long	tm_year;
};

#define _TBIAS_DAYS		((70 * (long)365) + 17)
#define _TBIAS_SECS		(_TBIAS_DAYS * (long)86400)
#define	_TBIAS_YEAR		1900
#define MONTAB(year)		((((year) & 03) || ((year) == 0)) ? mos : lmos)
const int	lmos[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
const int	mos[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
#define	Daysto32(year, mon)	(((year - 1) / 4) + MONTAB(year)[mon])

void xttotm(struct tm *t, double secsarg)
{
double 		secs;
long		days;
long		mon;
long		year;
long		i;
const long*	pm;

	#ifdef	_XT_SIGNED
		if (secsarg >= 0) {
			secs = (u32_t)secsarg;
			days = _TBIAS_DAYS;
		} else {
			secs = (u32_t)secsarg + _TBIAS_SECS;
			days = 0;
		}
	#else
		secs = secsarg;
		days = _TBIAS_DAYS;
	#endif

		/* days, hour, min, sec */
	long long temp = secs / 86400; days+=temp;
	long zamSecs = secs - temp*86400;
	t->tm_hour = zamSecs / 3600;	zamSecs %= 3600;
	t->tm_min = zamSecs / 60;		t->tm_sec = zamSecs % 60;

		/* determine year */
	for (year = days / 365; days < (i = Daysto32(year, 0) + 365*year); ) { --year; }
	days -= i;
	t->tm_year = year + _TBIAS_YEAR;

		/* determine month */
	pm = (long *)MONTAB(year);
	for (mon = 12; days < pm[--mon]; );
	t->tm_mon = mon + 1;
	t->tm_mday = days - pm[mon] + 1;
}

int isSymbol(char c){
	return c>='0' && c<='9' || c>='a' && c<='z';
}

int digit(char num)
{
  switch (num) {
  case '0': return 0;
  case '1': return 1;
  case '2': return 2;
  case '3': return 3;
  case '4': return 4;
  case '5': return 5;
  case '6': return 6;
  case '7': return 7;
  case '8': return 8;
  case '9': return 9;
  case 'a': return 10;
  case 'b': return 11;
  case 'c': return 12;
  case 'd': return 13;
  case 'e': return 14;
  case 'f': return 15;
  case 'g': return 16;
  case 'h': return 17;
  case 'i': return 18;
  case 'j': return 19;
  case 'k': return 20;
  case 'l': return 21;
  case 'm': return 22;
  case 'n': return 23;
  case 'o': return 24;
  case 'p': return 25;
  case 'q': return 26;
  case 'r': return 27;
  case 's': return 28;
  case 't': return 29;
  case 'u': return 30;
  case 'v': return 31;
  case 'w': return 32;
  case 'x': return 33;
  case 'y': return 34;
  case 'z': return 35;
  }
}

int colZnak=0;

void getNumbers(char *strEcxp, int iter, int *size, char *num, int *mas){
	int res = 0;
    int i = 0;
	while(*size!=2){
		res=0;
		int countNumber = 0;
    while (strEcxp[(iter)] == ' ') {
        (iter)++;
    }
 
    if (strEcxp[(iter)] == '\0') {
        *size=-3;return;
    }
 
    while (isSymbol(strEcxp[(iter)])) {
        	countNumber++; (iter)++;
	}
	if(countNumber==0){*size=-3;return;}
	if(*size==-2){
		if(countNumber>9){*size=-1;printf("error: int overflow\n");return;}
		int j =0;colZnak=countNumber;
		for(i = iter - countNumber;i<iter;i++){
			num[j]=strEcxp[i];j++;
		}
		num[j]='\0';*size=0;
	}else{
	for (i = iter - countNumber; i < iter; i++) {
        res += strEcxp[i] - '0';
            res *= 10;
    }
	res/=10;
	if(countNumber>2 || res>36 || res < 2){
		*size=-1;printf("error: base overflow\n");return;
	}
	mas[(*size)]=res;(*size)++;
	}
	}
	    while (strEcxp[(iter)] == ' ') {
        (iter)++;
    }
 
    if (strEcxp[(iter)] != '\0') {
        *size=-3;return;
    }
}
long long opat=0;
int checkNumber(char* num, long long foot){
	int i=0; long long temp=1;int chet = 1;
	while(*num){
		//char c = *(num);
		if(digit(num[i])>=foot)
			return -1;
		for(int j =0;j<colZnak-chet;j++){temp*=foot;}//itoac(temp);printf("\n");}
		//itoac(temp*digit(num[i]));printf("\n");
		opat=opat+temp*digit(num[i]);
		num++;temp=1;chet++;
	}
	return 1;
}

int translate(int chislo, int base, char * final){
	int num = (int)chislo;
  	int rest = num % base;
  	num /= base;
  	if (num == 0)
  	{
		if(rest<=9){final[0]=(char(rest+'0'));}
		else{final[0] = char(rest+87);}
    	return 1;
  	}
  	int k = translate(num, base, final);
  	if(rest<=9){final[k++]=(int(rest+'0'));}
		else{final[k++] = char(rest+87);}
  	return k;
}

void check_buffer(){
	int flag = 0;
	if(!strcmp_len(buffer, "nsconv ", 7)){
		flag=1;
		int size=-2; char num[20]={0};int mas[2]={0};
		getNumbers(buffer, 7, &size, num, mas);
		int base = mas[0];int nextBase=mas[1];
		if(size==-3){printf("Invalid Syntax\n");
			clear_buffer();return;}
		if(size==-1){
			opat=0;colZnak=0;
			clear_buffer();return;
		}
		if(checkNumber(num,base)==-1){
			opat=0;colZnak=0;
			clear_buffer();printf("error: invalid symbol\n");return;
		}
		//itoac(opat);printf("\n");
		if(opat>2147483647){opat=0;colZnak=0;
			printf("error: int overflow\n"); clear_buffer();return;}
		char finalchik[25]={0};
		(translate(opat, nextBase, finalchik));
		printf(finalchik);printf("\n");
		opat=0;colZnak=0;
	}
	if(!strcmp_len(buffer, "wintime ", 8)){
		flag=1;
		double temp = getNumber(buffer, 8);
		if(temp < 0 && temp>-2){printf("Invalid Syntax\n");
			clear_buffer();return;
		}
		if(temp<=-2){printf("Too big number\n"); clear_buffer();return;}
		temp=temp/10000000-11644473600;
		if(temp<0){printf("Impossible to get date\n");clear_buffer();return;}
		struct tm check;
		xttotm(&check, temp);
		itoac(check.tm_mday);printf(":");
		itoac(check.tm_mon);printf(":");
		itoac(check.tm_year);printf(" ");
		if(check.tm_hour==0)printf("00");else itoac(check.tm_hour); printf(":");
		if(check.tm_min==0) printf("00"); else itoac(check.tm_min); printf(":");
		if(check.tm_sec==0) printf("00"); else itoac(check.tm_sec); printf("\n");
	}
	if(!strcmp_len(buffer, "posixtime ", 10)){
		flag=1;
		long number = getNumber(buffer, 10);
		if(number ==-1){printf("Invalid Syntax\n");
			clear_buffer();return;
		}
		if(number ==-2){printf("Too big number\n"); clear_buffer();return;}
		struct tm check;
		xttotm(&check, number);
		itoac(check.tm_mday);printf(":");
		itoac(check.tm_mon);printf(":");
		itoac(check.tm_year);printf(" ");
		if(check.tm_hour==0)printf("00");else itoac(check.tm_hour); printf(":");
		if(check.tm_min==0) printf("00"); else itoac(check.tm_min); printf(":");
		if(check.tm_sec==0) printf("00"); else itoac(check.tm_sec); printf("\n");
	}
	if(!strcmp(buffer, "shutdown")){
		shutdown();
	}
	else
	{
		if(!strcmp(buffer, "info")){
			info();flag=1;}
		else{
			if(!strcmp(buffer, "clear")){
				clear_screen();flag=1;}
		}
	}
	if(!flag)printf("Unknown command\n");
	clear_buffer();
}

int strcmp_len(char *str1, char *str2, int len) {
	int i = 0;
	for (i = 0; i < len; i++) {
		if (str1[i] == 0 || str2[i] == 0) {
			break;
		}
		if ((str1[i] - str2[i] != 0)) {
			return str1[i] - str2[i];
		}
	}
	if(count==0)return -1;
	return 0;
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
		shift*=-1;
		break;
	default:
		if(count<40){
			if(shift<0)
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
	video_buf += (80 * line+column)*2;
	while (*ptr)
	{
		if((unsigned char)*ptr=='\n'){line++;column=0;ptr++;continue;}
		else{
		column++;
		}
		video_buf[0] = (unsigned char)*ptr; 
		video_buf[1] = get_color(color); 
		video_buf += 2;
		ptr++;
	}
	cursor_moveto(line, column);
}

void shutdown() {
	outw(0x604, 0x2000);
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
