#include <conio.h>
#include <assert.h>
#include <fstream>

#include "Algo.h"

/* Masks for the three shift registers */
#define R1MASK	0x07FFFF /* 19 bits, numbered 0..18 */
#define R2MASK	0x3FFFFF /* 22 bits, numbered 0..21 */
#define R3MASK	0x7FFFFF /* 23 bits, numbered 0..22 */

/* Middle bit of each of the three shift registers, for clock control */
#define R1MID	0x000100 /* bit 8 */
#define R2MID	0x000400 /* bit 10 */
#define R3MID	0x000400 /* bit 10 */

typedef unsigned char byte;
typedef unsigned long word;
typedef word bit;

using namespace std;
/* The three shift registers.  They're in global variables to make the code
 * easier to understand.
 * A better implementation would not use global variables. */
word R1, R2, R3;
/* Look at the middle bits of R1,R2,R3, take a vote, and
 * return the majority value of those 3 bits. */
bit majority() {
	int sum;
	sum = ((R1 >> 8) & 1) + ((R2 >> 10) & 1) + ((R3 >> 10) & 1);
	if (sum >= 2)
		return 1;
	else
		return 0;
}

/* Clock two or three of R1,R2,R3, with clock control
 * according to their middle bits.
 * Specifically, we clock Ri whenever Ri's middle bit
 * agrees with the majority value of the three middle bits.*/
void a51::clock() {
	bit maj = majority();
	if (((R1 & R1MID) != 0) == maj)
		R1 = ((R1 << 1) & R1MASK) | (1 & (R1 >> 18 ^ R1 >> 17 ^ R1 >> 16 ^ R1 >> 13));
	if (((R2 & R2MID) != 0) == maj)
		R2 = ((R2 << 1) & R2MASK) | (1 & (R2 >> 21 ^ R2 >> 20));
	if (((R3 & R3MID) != 0) == maj)
		R3 = ((R3 << 1) & R3MASK) | (1 & (R3 >> 22 ^ R3 >> 21 ^ R3 >> 20 ^ R3 >> 7));
}


/* Clock all three of R1,R2,R3, ignoring their middle bits.
 * This is only used for key setup. */
void clockallthree() {
	R1 = ((R1 << 1) & R1MASK) | (1 & (R1 >> 18 ^ R1 >> 17 ^ R1 >> 16 ^ R1 >> 13));
	R2 = ((R2 << 1) & R2MASK) | (1 & (R2 >> 21 ^ R2 >> 20));
	R3 = ((R3 << 1) & R3MASK) | (1 & (R3 >> 22 ^ R3 >> 21 ^ R3 >> 20 ^ R3 >> 7));
}
/* 3-bit to 8-bit substitution */
byte substitutions[] = {
	0x1D, // 000
	0x3A, // 001
	0x47, // 010
	0x74, // 011
	0x8E, // 100
	0xA3, // 101
	0xD1, // 110
	0xE8  // 111 
};

bit getbit() {
	// Get the last three bits of each register.
	bit r1bits = (R1 >> 16) & 0x7;
	bit r2bits = (R2 >> 19) & 0x7;
	bit r3bits = (R3 >> 20) & 0x7;

	// Look up the 8-bit substitution for each three-bit value.
	byte r1sub = substitutions[r1bits];
	byte r2sub = substitutions[r2bits];
	byte r3sub = substitutions[r3bits];

	// XOR the three 8-bit substitutions together.
	byte xorresult = r1sub ^ r2sub ^ r3sub;

	// Get the least significant three bits of the XOR result.
	bit lastthreebits = (xorresult >> 5) & 0x7;

	// XOR the three LSBs to generate the keystream bit.
	bit keystreambit;
	keystreambit = lastthreebits;
	keystreambit ^= (lastthreebits >> 1);
	keystreambit ^= (lastthreebits >> 2);
	keystreambit = keystreambit & 0x1;

	return keystreambit;
}


bit getbit_original() {
	return ((R1 >> 18) ^ (R2 >> 21) ^ (R3 >> 22)) & 1; //- original
}


/* Do the A5/1 key setup.  This routine accepts a 64-bit key and a 22-bit frame number. */
void keysetup(byte key[8], word frame) {
	int i;
	bit keybit, framebit;

	/* Zero out the shift registers. */
	R1 = R2 = R3 = 0;

	/* Load the key into the shift registers, LSB of first byte of key array first,
	 * clocking each register once for every key bit loaded.  (The usual clock
	 * control rule is temporarily disabled.) */
	for (i = 0; i < 64; i++) {
		clockallthree(); /* always clock */
		keybit = (key[7-i / 8] >> (i & 7)) & 1; /* The i-th bit of the key */
		R1 ^= keybit; R2 ^= keybit; R3 ^= keybit;
	}

	/* Load the frame number into the shift registers, LSB first,
	 * clocking each register once for every key bit loaded.  (The usual clock
	 * control rule is still disabled.) */
	for (i = 0; i < 22; i++) {
		clockallthree(); /* always clock */
		framebit = (frame >> i) & 1; /* The i-th bit of the frame #*/
		R1 ^= framebit; R2 ^= framebit; R3 ^= framebit;
	}

	/* Run the shift registers for 100 clocks to mix the keying material and frame number
	 * together with output generation disabled, so that there is sufficient avalanche.
	 * We re-enable the majority-based clock control rule from now on. */
	for (i = 0; i < 100; i++) {
		a51::clock();
	}

	/* Now the key is properly set up. */
}


long long convertToBits(long long size, char unit) {
	cout << size << " ";
	switch (unit) {
	case 'b':
	case 'B':
		cout << "Bytes" << endl;
		return (long long)size * 8;
	case 'k':
	case 'K':
		cout << "Kilobytes" << endl;
		return (long long)size * 8 * 1024;
	case 'm':
	case 'M':
		cout << "Megaytes" << endl;
		return (long long)size * 8 * 1024 * 1024;
	case 'g':
	case 'G':
		cout << "Gigbaytes" << endl;
		return (long long)size * 8 * 1024 * 1024 * 1024;
	default:
		cout << "Invalid Input" << endl;
		return -1; // Invalid unit
	}
}


/* Generate bitstream of user's input size requirement */
void run_generator(long long size, int version, int opt) {
	long long bits = 0;
	unsigned int bit;
	ofstream outfile;

	if (opt == FILE_OUT)
		version == BITSTREAM_ORIGINAL ? outfile.open("bit_out_original_LARGE.txt") : outfile.open("bit_out_modified_500mb.txt");
		

	while (bits<=size) {

		a51::clock();
		bit = (version == BITSTREAM_ORIGINAL ? getbit_original() : getbit());

		if (opt == FILE_OUT)
			outfile << bit;
		else
			cout << bit;
		bits++;
	}
	
	if (opt == FILE_OUT)
		outfile.close();
	else
		cout << endl;
}


/* Generate output.  We generate 228 bits of keystream output.  The first 114 bits is for
 * the A->B frame; the next 114 bits is for the B->A frame.  You allocate a 15-byte buffer for each direction, and this function fills
 * it in. */
void run(bool print, int version, byte AtoBkeystream[], byte BtoAkeystream[], byte AtoBKetStream_Modified[], byte BtoAKetStream_Modified[]) {
	int i;

	/* Zero out the output buffers. */
	for (i = 0; i <= 113 / 8; i++)
	{
		if (version == NO || version == YES)
			AtoBkeystream[i] = BtoAkeystream[i] = 0;
		if (version == ONLY|| version == YES)
			AtoBKetStream_Modified[i] = BtoAKetStream_Modified[i] = 0;
	}


	/* Generate 114 bits of keystream for the A->B direction.  Store it, MSB first. */
	if (print)
		cout << "A->B direction" << endl;

	for (i = 0; i < 114; i++) {
		a51::clock();
		
		if (version == ONLY || version == YES)
		{
			if (print)
				cout << getbit();
			AtoBKetStream_Modified[i / 8] |= getbit() << (7 - (i & 7));
		}

		if (version == NO || version == YES)
			AtoBkeystream[i / 8] |= getbit_original() << (7 - (i & 7));
	}
	if (print)
		cout << endl;

	/* Generate 114 bits of keystream for the
	 * B->A direction.  Store it, MSB first. */
	if (print)
		cout << "B->A direction" << endl;
	
	for (i = 0; i < 114; i++) {
		a51::clock();

		if (version == ONLY || version == YES)
		{
			if (print)
				cout << getbit();
			BtoAKetStream_Modified[i / 8] |= getbit() << (7 - (i & 7));
		}

		if (version == NO || version == YES)
			BtoAkeystream[i / 8] |= getbit_original() << (7 - (i & 7));
	}
	if (print)
		cout << endl;
}

void A51_GSM(byte* key, int klen, int count, byte* block1, byte* block2, int version, byte* block1_modified, byte* block2_modified, bool print)
{
	assert(klen == 64);
	keysetup(key, count); // TODO - frame and count are not the same
	run(print, version, block1, block2, block1_modified, block2_modified);
}

void A51_GenerateBits(byte* key, int klen, int count, int version, long long size, char unit, int opt)
{
	assert(klen == 64);
	keysetup(key, count);
	long long bit_size = convertToBits(size, unit);
	cout << "Number of bits:" << bit_size << endl;
	if (bit_size != -1)
		run_generator(bit_size, version, opt);
}	