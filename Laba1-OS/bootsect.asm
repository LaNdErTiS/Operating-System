[BITS 16]
[ORG 0x7C00]
start:
mov ax,cs
mov ds,ax
mov ss,ax
mov sp, start

mov dh, '1'
mov ah, 0x0E
mov al, 0x0A
int 0x10     

big_loop:
mov edi, 0xb86E0
mov esi, string
call video_puts
mov ah,0x0E
mov al, 0x8
int 0x10

mov ah,  0x09            
mov cx,  1 ;number of strings
cmp dh, '1'
jne secondColor
mov bl, 0x7
jmp contin
secondColor:
cmp dh, '2'
jne thirdColor
mov bl, 0x9
jmp contin
thirdColor:
cmp dh, '3'
jne fourthColor
mov bl, 0x4
jmp contin
fourthColor:
cmp dh, '4'
jne fifthColor
mov bl, 0xE
jmp contin
fifthColor:
cmp dh, '5'
jne sixthColor
mov bl, 0x8
jmp contin
sixthColor:
mov bl, 0x2 
contin:      
mov al,  dh  ;current number 
int 0x10

mov ah,0x00 ;wait symbol
int 0x16
cmp al, 0x0D
je final
cmp ah, 0x48
je increment
cmp ah, 0x50
je decrement

jmp big_loop

increment:
cmp dh, '1'
jne twoDec
mov dh, '2'
jmp big_loop
twoDec:
cmp dh, '2'
jne threeDec
mov dh, '3'
jmp big_loop
threeDec:
cmp dh, '3'
jne fourDec
mov dh, '4'
jmp big_loop
fourDec:
cmp dh, '4'
jne fiveDec
mov dh, '5'
jmp big_loop
fiveDec:
cmp dh, '5'
jne sixDec
mov dh, '6'
jmp big_loop
sixDec:
mov dh, '1'
jmp big_loop

decrement:
cmp dh, '1'
jne two
mov dh, '6'
jmp big_loop
two:
cmp dh, '2'
jne three
mov dh, '1'
jmp big_loop
three:
cmp dh, '3'
jne four
mov dh, '2'
jmp big_loop
four:
cmp dh, '4'
jne five
mov dh, '3'
jmp big_loop
five:
cmp dh, '5'
jne six
mov dh, '4'
jmp big_loop
six:
mov dh, '5'
jmp big_loop

video_puts:
mov al, [esi]
test al, al 
jz video_puts_end
cmp dh, '1'
jne second
mov ah, 0x7
jmp cont
second:
cmp dh, '2'
jne third
mov ah, 0x9
jmp cont
third:
cmp dh, '3'
jne fourth
mov ah, 0x4
jmp cont
fourth:
cmp dh, '4'
jne fifth
mov ah, 0xE
jmp cont
fifth:
cmp dh, '5'
jne sixth
mov ah, 0x8 ;backspace
jmp cont
sixth:
mov ah, 0x2
cont:
mov [edi], al
mov [edi+1], ah
add edi, 2
add esi, 1
jmp video_puts
video_puts_end:
ret

gdt:
db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info:
dw gdt_info - gdt
dw gdt, 0

final:
mov ax, 0x3
int 0x10
mov [0x9FF0],dh

mov ax,0x1000 
mov es,ax  ;segment
mov bx,0x0 ;offset
mov dh,0	
mov ch,0	
mov cl,1	
mov al,0x12
mov dl,1
mov ah,0x2
int 0x13

cli		

lgdt [gdt_info]	

in al,0x92
or al,2
out 0x92,al

mov eax,cr0	
or al,1
mov cr0, eax

jmp 0x8:protected_mode

[BITS 32]
protected_mode:
mov ax,0x10
mov es,ax
mov ds,ax
mov ss,ax
call 0x10000

string:
db "1-white 2-blue 3-red 4-yellow 5-gray 6-green", 0

times (512 - ($ - start) - 2) db 0
db 0x55, 0xAA