#ifndef BLOWFISH_H
#define BLOWFISH_H

#include <cstdint>

inline constexpr int NUM_SUBKEYS = 18;
inline constexpr int NUM_S_BOXES = 4;
inline constexpr int NUM_ENTRIES = 256;
inline constexpr int MAX_STRING = 256;
inline constexpr int MAX_PASSWD = 56;

#ifndef BF_BIG_ENDIAN
#define BF_LITTLE_ENDIAN
#endif

struct WordByte {
#ifdef BF_BIG_ENDIAN
	unsigned int zero : 8;
	unsigned int one : 8;
	unsigned int two : 8;
	unsigned int three : 8;
#else
	unsigned int three : 8;
	unsigned int two : 8;
	unsigned int one : 8;
	unsigned int zero : 8;
#endif
};

union Word {
	unsigned int word;
	WordByte byte;
};

struct DWord {
	Word word0;
	Word word1;
};

class Blowfish {
public:
	Blowfish();
	~Blowfish();

	void Reset();
	void Set_Passwd(char* passwd = nullptr);
	void Encrypt(void* ptr, unsigned int n_bytes);
	void Decrypt(void* ptr, unsigned int n_bytes);

private:
	unsigned int PA[NUM_SUBKEYS];
	unsigned int SB[NUM_S_BOXES][NUM_ENTRIES];

	void Gen_Subkeys(char* passwd);
	void BF_En(Word* x1, Word* x2);
	void BF_De(Word* x1, Word* x2);
};

#endif