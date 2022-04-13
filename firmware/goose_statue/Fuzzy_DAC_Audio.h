/*
 Name:		Fuzzy_DAC_Audio.h
 Created:	6/4/2019 2:25:46 AM
 Author:	georgychen
 Editor:	http://www.visualmicro.com
*/

#ifndef _Fuzzy_DAC_Audio_h
#define _Fuzzy_DAC_Audio_h

#include <Arduino.h>

const uint_fast32_t SampleRate = 22050;
const int_fast8_t SampleBits = 8;

#define DAC_8_NEUTRAL 128
#define DEFAULT_SAMPLE_RATE 22050
#define ON HIGH
#define OFF LOW
#define LEFT_SHIFT_BIT 2

struct {
	// Current sample position
	uint_fast32_t samplePosition;

	// Current amplitude value
	// ------
	// MIC: Actual value should be integers within [0, 255], and its used for a 16-bit timer.
	// But I'm still setting it to int16 to avoid potential (lower-bound) overflow.
	// See value setting of OCR1A.
	int_fast16_t currentPCM;

} g_stat = { 0, 0 };


class FuzzyDACAudio
{
public:
	FuzzyDACAudio();
	void begin();
	void setShutdownPin(uint8_t pin);
	void play8BitArray(const uint8_t* arrayName, uint32_t arraySize);
	void interruptHandler();
	bool isPlaying();
	void setSampleRate(uint16_t sampleRate);
	void playHuffArray(const int_fast16_t * huffDict, uint_fast32_t soundDataBits, const uint8_t * soundData);

private:
	uint32_t _sampleRate = DEFAULT_SAMPLE_RATE;
	void tcConfigure(uint32_t sampleRate);
	void tcReset();
	void tcDisable();
	bool tcIsSyncing();
	void tcStartCounter();
	uint32_t __nowPlayingSampleIndex = 0;
	uint32_t __arraySize;
	const uint8_t* __arrayName;
	bool volatile __isPlaying = false;
	uint8_t shutdownPin;
	bool shutdownEnabled = false;

	inline int _get_bit(uint_fast32_t pos, boolean autoLoadOnBit0 = false);
	//const uint8_t* __SoundData;
	int_fast16_t _decode_huff(uint_fast32_t &pos, int_fast16_t const *huffDict);
	void loadSample();
	const int_fast16_t *_HuffDict;
	uint_fast32_t _SoundDataBits;
	const uint8_t * _SoundData;

	void setAmplifier(bool status); //turn amplifier on(true) or off(false)
};

#endif
