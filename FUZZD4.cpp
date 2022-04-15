#pragma warning(disable : 4996)
#pragma warning(disable : 4789)
#pragma warning(disable : 6200)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADD_AI 0b00000000
#define MOV_AB 0b00010000
#define IN_A   0b00100000
#define MOV_AI 0b00110000
#define MOV_BA 0b01000000
#define ADD_BI 0b01010000
#define IN_B   0b01100000
#define MOV_BI 0b01110000
#define OUT_I1 0b10000000
#define OUT_B  0b10010000
#define OUT_I2 0b10100000
#define OUT_I3 0b10110000
#define JMP_B1 0b11000000
#define JMP_B2 0b11010000
#define JNC_I  0b11100000
#define JMP_I  0b11110000

unsigned char REG_A = 0b0000;
unsigned char REG_B = 0b0000;
unsigned char REG_P = 0b0000;
unsigned char REG_O = 0b0000;
int point = 0;
unsigned char ROM[17];
unsigned char opcode;
unsigned char im;
char c_flag = 0;
char loop[256 * 256 * 2];

int loop_chk() {
    long tmp = 0;
    tmp += REG_A & 0b00001111;
    tmp <<= 4;
    tmp += REG_B & 0b00001111;
    tmp <<= 4;
    tmp += REG_P & 0b00001111;
    tmp <<= 4;
    tmp += REG_O & 0b00001111;
    tmp <<= 1;
    tmp += c_flag & 0b00000001;
    if (loop[tmp] == 0) {
        loop[tmp]++;
    }
    else {
        return 1;
    }
    return 0;
}

void dumm12(int point) {
    puts("almost there!\n");
}
void dumm11(int point) {
    if (point > 20480) {
        dumm12(point);
    }
}
void dumm10(int point) {
    if (point > 10240) {
        dumm11(point);
    }
}
void dumm9(int point) {
    if (point > 5120) {
        dumm10(point);
    }
}
void dumm8(int point) {
    if (point > 2560) {
        dumm9(point);
    }
}
void dumm7(int point) {
    if (point > 1280) {
        dumm8(point);
    }
}
void dumm6(int point) {
    if (point > 640) {
        dumm7(point);
    }
}
void dumm5(int point) {
    if (point > 320) {
        dumm6(point);
    }
}
void dumm4(int point) {
    if (point > 160) {
        dumm5(point);
    }
}
void dumm3(int point) {
    if (point > 80) {
        dumm4(point);
    }
}
void dumm2(int point) {
    if (point > 40) {
        dumm3(point);
    }
}
void dumm1(int point) {
    if (point > 20) {
        dumm2(point);
    }
}
void coverage(int point) {
    if (point > 10) {
        dumm1(point);
    }
}
int main(int argc, char** argv) {
    int point = 0;

    if (argc != 2) {
        return -1;
    }
    FILE* fp;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        return -1;
    }
    if (fgets((char *)ROM, 17, fp) == NULL) {
        return -1;
    }
    fclose(fp);
    for (int i = 0; i < 256 * 256 * 2; i++) {
        loop[i] = 0;
    }
    for (int i = 0; i < 16; i++) {
        printf("%02X ", ROM[i]);
    }
    puts("\n");
    while (REG_O < 0b1000) {
        if (loop_chk() == 1) {
            puts("loop detected!!\n");
            return 0;
        }
        opcode = ROM[REG_P] & 0b11110000;
        im = ROM[REG_P] & 0b00001111;
        switch (opcode) {
        case MOV_AI:
            REG_A = im;
            c_flag = 0;
            REG_P = (++REG_P) & 0b1111;
            break;
        case MOV_BI:
            REG_B = im;
            c_flag = 0;
            REG_P = (++REG_P) & 0b1111;
            break;
        case MOV_AB:
            REG_A = REG_B + im;
            c_flag = 0;
            if (REG_A > 0b1111) {
                c_flag = 1;
                REG_A = REG_A & 0b00001111;
            }
            REG_P = (++REG_P) & 0b1111;
            break;
        case MOV_BA:
            REG_B = REG_A + im;
            c_flag = 0;
            if (REG_B > 0b1111) {
                c_flag = 1;
                REG_B = REG_B & 0b00001111;
            }
            REG_P = (++REG_P) & 0b1111;
            break;
        case ADD_AI:
            REG_A = REG_A + im;
            c_flag = 0;
            if (REG_A > 0b1111) {
                c_flag = 1;
                REG_A = REG_A & 0b00001111;
            }
            REG_P = (++REG_P) & 0b1111;
            break;
        case ADD_BI:
            REG_B = REG_B + im;
            c_flag = 0;
            if (REG_B > 0b1111) {
                c_flag = 1;
                REG_B = REG_B & 0b1111;
            }
            REG_P = (++REG_P) & 0b1111;
            break;
        case IN_A:
            REG_A = REG_O + im;
            c_flag = 0;
            if (REG_A > 0b1111) {
                c_flag = 1;
                REG_A = REG_A & 0b1111;
            }
            REG_P = (++REG_P) & 0b1111;
            break;
        case IN_B:
            REG_B = REG_O + im;
            c_flag = 0;
            if (REG_B > 0b1111) {
                c_flag = 1;
                REG_B = REG_B & 0b00001111;
            }
            REG_P = (++REG_P) & 0b1111;
            break;
        case OUT_B:
            REG_O = REG_B + im;
            c_flag = 0;
            if (REG_O > 0b1111) {
                c_flag = 1;
                REG_O = REG_B & 0b00001111;
            }
            REG_P = (++REG_P) & 0b1111;
            break;
        case OUT_I1:
        case OUT_I2:
        case OUT_I3:
            REG_O = im;
            c_flag = 0;
            REG_P = (++REG_P) & 0b1111;
            break;
        case JMP_I:
            REG_P = im;
            c_flag = 0;
            break;
        case JMP_B1:
        case JMP_B2:
            c_flag = 0;
            if (c_flag == 0) {
                REG_P = (REG_B + im) & 0b1111;
                c_flag = 1;
            }
            else {
                REG_P = (++REG_P) & 0b1111;
            }
            break;
        case JNC_I:
            if (c_flag == 0) {
                REG_P = im;
            }
            else {
                REG_P = (++REG_P) & 0b1111;
            }
            c_flag = 0;
        default:
            break;
        }
        point++;
        if (point > 26000) {
            for (int i = 16; i < 10000; i++) {
                ROM[i] = 0xFF;
            }
        }
        coverage(point);
    }
    printf("%d point\n", point);

    return 0;
}
