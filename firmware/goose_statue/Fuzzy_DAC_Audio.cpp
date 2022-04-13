/*
 Name:		Fuzzy_DAC_Audio.cpp
 Created:	6/4/2019 2:25:46 AM
 Author:	georgychen
 Editor:	http://www.visualmicro.com
*/

#include "Fuzzy_DAC_Audio.h"


/*pointer used to attach TC5 interrupt*/
FuzzyDACAudio* _audioInstancePointer;

FuzzyDACAudio::FuzzyDACAudio()
{
	_audioInstancePointer = this;
}

void FuzzyDACAudio::begin()
{
	//configure the dac
	analogWrite(A0, DAC_8_NEUTRAL<< LEFT_SHIFT_BIT);

	//configure the TC
	tcConfigure(_sampleRate);
}

void FuzzyDACAudio::setShutdownPin(uint8_t pin)
{
	shutdownPin = pin;
	pinMode(shutdownPin, OUTPUT);
	setAmplifier(OFF);
	shutdownEnabled = true;
}

void FuzzyDACAudio::tcConfigure(uint32_t sampleRate)
{
	// Enable GCLK for TC4 and TC5 (timer counter input clock)
	GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5));
	while (GCLK->STATUS.bit.SYNCBUSY);

	tcReset();

	// Set Timer counter Mode to 16 bits
	TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

	// Set TC5 mode as match frequency
	TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

	//set prescaler
	TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1;

	//setup the count value
	TC5->COUNT16.CC[0].reg = (uint16_t)(SystemCoreClock / sampleRate - 1);
	while (tcIsSyncing());


	// Configure interrupt request
	NVIC_DisableIRQ(TC5_IRQn);
	NVIC_ClearPendingIRQ(TC5_IRQn);
	NVIC_SetPriority(TC5_IRQn, 0);
	NVIC_EnableIRQ(TC5_IRQn);

	// Enable the TC5 interrupt request
	TC5->COUNT16.INTENSET.bit.MC0 = 1;
	while (tcIsSyncing());
}

void FuzzyDACAudio::play8BitArray(const uint8_t* arrayName, uint32_t arraySize)
{
	if (__isPlaying == true)return;
	__isPlaying = true;
	__nowPlayingSampleIndex = 0;
	__arraySize = arraySize;
	__arrayName = arrayName;


	tcStartCounter();

}

void FuzzyDACAudio::playHuffArray(const int_fast16_t * huffDict, uint_fast32_t soundDataBits, const uint8_t * soundData)
{
	if (__isPlaying == true)return;
	__isPlaying = true;

	setAmplifier(ON);

	//setup the values and array pointers 
	g_stat.samplePosition = 0;
	g_stat.currentPCM = 0;
	_HuffDict = huffDict;
	_SoundDataBits = soundDataBits;
	_SoundData = soundData;

	tcStartCounter();
	while (isPlaying());

	//reset the sample rate to default value
	//TC5->COUNT16.CC[0].reg = (uint16_t)(SystemCoreClock / _sampleRate - 1);
	//while (tcIsSyncing());

	setAmplifier(OFF);
}

void FuzzyDACAudio::setSampleRate(uint16_t sampleRate)
{
	//setup the new sample rate
	TC5->COUNT16.CC[0].reg = (uint16_t)(SystemCoreClock / sampleRate - 1);
	while (tcIsSyncing());
}

void FuzzyDACAudio::tcReset()
{
	/*
	Reset TCx
	All registers in the TC, except DBGCTRL, will be reset to their initial state,
	and the TC will be disabled.
	The TC should be disabled before the TC is reset in order to avoid undefined behavior.
	*/

	tcDisable();
	TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
	while (tcIsSyncing());
	while (TC5->COUNT16.CTRLA.bit.SWRST);
}

void FuzzyDACAudio::tcDisable()
{
	// Disable TC5
	TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
	while (tcIsSyncing());
}

bool FuzzyDACAudio::tcIsSyncing()
{
	return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

void FuzzyDACAudio::tcStartCounter()
{
	// Enable TC
	TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
	while (tcIsSyncing());
}

void FuzzyDACAudio::interruptHandler(void)
{
	loadSample();

	/*
	if (__nowPlayingSampleIndex < __arraySize)
	{
	//SERIAL_OBJECT << __arrayName[__nowPlayingSampleIndex];
	analogWrite(A0, __arrayName[__nowPlayingSampleIndex]);
	__nowPlayingSampleIndex++;
	}
	else
	{
	// Disable TC5
	tcDisable();
	__isPlaying = false;
	analogWrite(A0, DAC_8_NEUTRAL);
	}
	*/
	// Clear the interrupt
	TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

bool FuzzyDACAudio::isPlaying()
{
	return __isPlaying;
}

// Get one bit from sound data
inline int FuzzyDACAudio::_get_bit(uint_fast32_t pos, boolean autoLoadOnBit0) {
	const auto bitPosition = pos & 7;
	static uint_fast8_t bit;
	if (!autoLoadOnBit0 || !bitPosition) {
		// read indexed byte from Flash memory
		//bit = pgm_read_byte(&SoundData[pos >> 3]);
		bit = _SoundData[pos >> 3];
	}
	// extract the indexed bit
	return (bit >> (7 - bitPosition)) & 1;
}

// Decode bit stream using Huffman codes
int_fast16_t FuzzyDACAudio::_decode_huff(uint_fast32_t &pos, int_fast16_t const *huffDict) {
	auto p = pos;
	do {
		const auto b = _get_bit(p++, true);
		if (b) {
			const auto offs = *huffDict;
			huffDict += offs ? offs + 1 : 2;
		}
	} while (*(huffDict++));
	pos = p;
	return *huffDict;
}

// This is called at sample rate to load the next sample.
void FuzzyDACAudio::loadSample()
{
	auto samplePosition = g_stat.samplePosition;

	// end of playing
	if (samplePosition >= _SoundDataBits) {
		//SERIAL_OBJECT << "samplePosition = " << samplePosition << endl;
		samplePosition = 0;
		g_stat.currentPCM = 0;

		// Disable TC5
		tcDisable();
		__isPlaying = false;
		analogWrite(A0, DAC_8_NEUTRAL);

		//resume default sample rate
		TC5->COUNT16.CC[0].reg = (uint16_t)(SystemCoreClock / _sampleRate - 1);
		while (tcIsSyncing());

		setAmplifier(OFF);
	}

	// MIC: The differential series Dif[N+1] := sbytes[N+1] - sbytes[N], and Dif[0] = sbytes[0].
	// Has to be sint16, since each element is computed by subtraction between 2 sint8s.
	auto differential = _decode_huff(samplePosition, _HuffDict);
	g_stat.currentPCM += differential;

	// Set 16-bit PWM register with sample value
	//OCR1A = constrain(g_stat.currentPCM + (1 << (SampleBits - 1)), 0, (1 << SampleBits) - 1);

	uint16_t sample = constrain(g_stat.currentPCM + (1 << (SampleBits - 1)), 0, (1 << SampleBits) - 1);

	//SERIAL_OBJECT << "sample = " << sample << endl;

	//const uint16_t dev = 120;
	//sample = constrain(sample, DAC_8_NEUTRAL - dev, DAC_8_NEUTRAL + dev);
	//sample = map(sample, DAC_8_NEUTRAL - dev, DAC_8_NEUTRAL + dev, 0, 255);
	

	//output the value to DAC
	//analogWriteResolution() function didn't work. So manually shift 8bit sounddata to 10 bit.
	analogWrite(A0, sample << LEFT_SHIFT_BIT);

	g_stat.samplePosition = samplePosition;
}

#ifdef __cplusplus
extern "C" {
#endif

	void TC5_Handler(void)
	{
		//attach the TC5 interrupt handler to the instance
		_audioInstancePointer->interruptHandler();
	}


#ifdef __cplusplus
}
#endif


void FuzzyDACAudio::setAmplifier(bool status)
{
	if( shutdownEnabled )
		digitalWrite(shutdownPin, status);
}
