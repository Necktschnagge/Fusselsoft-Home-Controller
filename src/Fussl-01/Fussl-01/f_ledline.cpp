/************************************************************************/
/* status:
	there aren't any known bugs or omissions
	
to do:
	overview, doc strings, check visibilities
	                                                                     */
/************************************************************************/


#include "f_ledline.h"
#include <stdlib.h> //needed for abs()
#include "f_hardware.h" // needed for hardware::delay() (busy waiting)
#include <avr/interrupt.h> // needed for cli()
#include <avr/pgmspace.h> // needed for pgm_read_byte, etc.

constexpr uint8_t DOTPOSITION = 7; // dot bit
constexpr uint8_t DOTSIGN = 1<<DOTPOSITION; // use pushByte to push an only-dot

static uint8_t LINELENGTH; // is set by the init function
static uint64_t _LEDLINE_; // contains the current state which is displayed on the led line and which was pushed to it somewhere in past.
							// functions that modify output have to update this value.


void led::init(const uint8_t lineLength){
	DDRA = 0b00000111; // LATCH BIT ::: CLOCK BIT ::: DATA BIT
	PORTA = 0b11111000;
	_LEDLINE_ = 0;
	led::clear();
	LINELENGTH = lineLength;
}

void led::latch(){
	/* send a latch clock signal */
	PORTA = PORTA ^ 0b00000100;
	PORTA = PORTA ^ 0b00000100;
}

void led::pushBitIntern(const bool bit){
	/* deprecated for user */
	/* set data bit and send a shift register clock signal (and do NOT update the coherent memory _LEDLINE_) */
	PORTA = (PORTA & 0b11111110) | bit;
	PORTA = PORTA ^ 0b00000010;
	PORTA = PORTA ^ 0b00000010;
}

void led::pushBit(const bool bit){
	/* push a bit to output stream (coherent) */
	_LEDLINE_ = (_LEDLINE_<<1) + bit;
	led::pushBitIntern(bit);
}

void led::pushByte(const uint8_t bitcode){
	/* push 8 bit to the output stream. MSB is pushed first, (coherent) */
	_LEDLINE_ = (_LEDLINE_<<8) + bitcode;
	for(int8_t shift = 7; shift >= 0; --shift){
		led::pushBitIntern(bitcode & (1<<shift));
	}
}

void led::push64(uint64_t bitcode){
	/* push 64 bit to the output stream. MSB is pushed first (coherent) */
	_LEDLINE_ = bitcode;
	for(int8_t shift = 63; shift >= 0; --shift){
		led::pushBitIntern(bitcode & (1<<shift));
	}
}

void led::pushMemory(){
	/* push the memory of the output stream VISIBLE to the output stream */
	led::push64(_LEDLINE_);
	led::latch();
}

void led::pushByteVisible(uint8_t bitcode){
	/* push a byte to the led output stream and update the latch */
	/* see ledPushByte() (MSB first) */
	led::pushByte(bitcode);
	led::latch();
}

bool led::isDotted(char sign){
	/* returns whether the sign contains an implicit dot or not */
	return (sign & 0b10000000);
}

void led::setDot(char* sign, bool dot){
	/* override the implicit dot of the sign with the explicit given dot */
	*sign = (*sign & 0b01111111) | (dot<<7);
}

bool led::changeDot(char* sign){
	/* change the dot of the sign and return whether the sign has a dot @after */
	*sign ^= 1<<7;
	return led::isDotted(*sign); 
}

uint8_t led::signCode(char sign){
	/* channel coding: look up the sign code in the PROGMEM and return the bit code for LED output (supports implicit dotting) */
	/* implicit dotting:
	*	char sign with MSB == 0 will normally be a character without a dot.
		if MSB ==1 an additional dot is printed
		Only in case you print e.g. a '?' or a ';' there will be a dot even if with MSB == 0. Actually signs with 'real implicit' dot will have no dot if MSB == 1
		
		equivalent description
		Every sign of the character set has its own 'real implicit' dot status {with dot | without dot}.
		MSB == 0 is the standard case and the sign will be printed as given in the character set.
		By using MSB == 1 instead of MSB == 0 the dot state will be inverted.
		
		nomenclature:
		 a char sign is basically a code with an >>implicit dot<< (the MSB)
		 the 'real implicit dot' is stored in the look-up table and means the dot of an question mark or an semicolon for example
	*/
	
	uint16_t code = sign & 0b01111111;
	
									//		!		"	#		$	%		&	' '		(	)		*	+		,	-		.	/		0	1		2	3		4	5		6	7		8	9		:	;		<	=		>	?		@
	const uint8_t codeTable[] PROGMEM = {   0xC8, 0x0A, 0x7F, 0x6E, 0x99, 0x3F, 0x00, 0x16, 0x4C, 0x2B, 0x49, 0x40, 0x01, 0x80, 0x19, 0x7E, 0x48, 0x3D, 0x6D, 0x4B, 0x67, 0x77, 0x4C, 0x7F, 0x6F, 0x81, 0xC0, 0x39, 0x21, 0x63, 0x9D, 0x5D,
		
									//	A		B	C		D	E		F	G		H	I		J	K		L	M		N	O		P	Q		R	S		T	U		V	W		X	Y		Z
										0x5F, 0x73, 0x31, 0x79, 0x37, 0x17, 0x76, 0x53, 0x12, 0x78, 0x3B, 0x32, 0x5E, 0x51, 0x71, 0x1F, 0x4F, 0x57, 0x66, 0x4C, 0x70, 0x2A, 0x7A, 0x5B, 0x6B, 0x3C,
										
									//	[		\	]		^	_		`	{		|	}		~	[127]
										0x36, 0x43, 0x6C, 0x0E, 0x20, 0x08, 0x27, 0x5A, 0x2D, 0x75, 0x0F};
		
	if (code == 10){
		/* do smth in order to make a line feed */
		return 0x00; // <<<<< the newline char is currently a space sign, please change the function signature / provide an additional function
	}
	
	if (code < 33){
		/* throw a standard sign */
		return 0xFF ^ (DOTSIGN * isDotted(sign));
	}
	
	if (code >96){// small letters match to capital letters
		if (code > 122){
			code += 6;
		}
		code -= 23;
	}
	
	code -=33;
	return static_cast<uint8_t>( pgm_read_byte(&codeTable[code])) ^ (isDotted(sign) * DOTSIGN);
}

void led::printSign(char sign){//for user
	/* (visible) print a sign to the end of the led output */
	led::pushByteVisible(led::signCode(sign));
}

void led::printDigit(uint8_t digit){//for user
	/* print a digit {0~9} (visible) to the end of the led output */
	/* DO NOT CALL WITH AN INTEGER GREATER THAN 9 (will cause a weak error) !!!!!*/
	if (digit/10){
		led::error(2);
	}
	led::printSign(48 + digit);
}

void led::printInt(int16_t integer){//for user
	/* print an integer to the end of the led output (of course with "-" if negative) */
	if (integer < 0){
		led::printSign('-');
	}
	uint8_t fractional = (abs(integer)) % 10;
	if (abs(integer/10)){
		led::printInt(abs(integer/10));
	}
	led::printDigit(fractional);
}

void led::printSignDottable(char sign, bool dot){//for user
	/* push a sign (visible) to the led output with explicit dotting (implicit dot will be overwritten) */
	led::setDot(&sign,dot);
	led::printSign(sign);
}

void led::printString(const char* string){//for user
	/* print a string to the led output (also supports implicit dotting ) */
	uint8_t i {0};
	while (*(string+i)!='\0'){
		led::printSign(*(string+i));
		++i;
	}
}

void led::clear(){//for user
	/* clear the LED line */
	for(uint8_t i = 0; i<LINELENGTH; ++i){
		led::pushByte(0);
	}
	led::latch();
}

void led::LFPrintString(const char* const string){//for user
	/* print the given string and as much as needed space signs before to push the previous string away */
	
	//////////////////////////////////////////////////////////////////////////
	/*char* ptr = string;
	while (*string!='\0'){
		++ptr;
	}
	for(uint8_t i = 0; i<(ptr-string); ++i){
		ledPushByte(0);
	}
	led::latch();
	ledPrintString(string);
	*/
	//////////////////////////////////////////////////////////////////////////
	
	//we dont have more than one line in our controller
	// for our controller we want to be flash memory friendly and not fast so we do:
	led::clear();
	led::printString(string);
}

void led::printDotsOnly(const uint8_t dotCode){//<<<<<< this function isnt ready for scalable linelength, please change sometime
	for (uint8_t i = 0; i<8; ++i){
		_LEDLINE_ &= ~(1<<(DOTPOSITION + 8 * i)); // make it 0
		_LEDLINE_ |= ((1<<(DOTPOSITION + 8 * i)) * !! (dotCode & (1<<i) ));
	}
	pushMemory();
}

void led::error(const uint16_t code){
	
	//static_assert(LINELENGTH>=4,"The ledError() needs at least 4 led elements!");  <<<<<<<< config c++11
	
	
	uint8_t SREG_temporal = SREG; // save interrupts
	cli();
	led::clear();
	led::printSign('E');
	hardware::delay(3000);
	// led::pushByteVisible(0x00); to push a space between 'E' and code
	led::printInt(code);
	hardware::delay(60000);// add some fancy blinking <<<<<<<<
	if (code > 99){
		while (1) {
			led::pushBitIntern(false);// this is only to avoid compiler optimization trashing this infinite loop
		}
	}
	SREG = SREG_temporal; // activate interrupts
}
