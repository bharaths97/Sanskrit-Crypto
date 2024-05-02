#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#define FILE_OUT 1
#define TEMRINAL 2

#define EXIT 0
#define NO 1
#define ONLY 2
#define YES 3
#define BITSTREAM_MODIFIED 4
#define BITSTREAM_ORIGINAL 5


typedef unsigned char byte;
typedef unsigned long word;
typedef word bit;

namespace a51 {
	void clock();
}

void A51_GSM(byte* key, int klen, int count, byte* block1, byte* block2, int modified , byte* block1_modified , byte* block2_modified , bool print = true);
void A51_GenerateBits(byte* key, int klen, int count, int version, long long size, char unit, int opt);