/*
This file contains all Controllino specific extensions to standard Arduino framework:
	- RTC API interface
	- RS485 interface

Version 0.6
12.6.2015

Version History
0.1 - 6.8.2014 - All RTC related functions
0.2 - 9.10.2014 - Code refurbished
0.3 - 17.10.2014 - Changes to conserve memory. Cosmetic changes
0.4 - 14.1.2014 - RS485 interface. Changed RTC functions to memorize previous SPI setting and put it back again after use.
0.5 - 10.2.2014 - Updated RTC functions for new RTC chip RV-2123
0.6 - 12.6.2014 - Updated RTC handling for MINI
*/


#include "Controllino.h"

byte chipSelect = 8;
boolean isRTCInitialized = false;

/* This function initializes RTC chip (RV-2123) and prepares communication via SPI using specified pin as chip select. 
WARNING: This function uses 10ms delay, because the RTC chip requires it. */
//=====================================
char Controllino_RTC_init(unsigned char aChipSelect)
{
	unsigned char SPISetting; // variable to hold SPI setting
	//Store current SPI setting
	SPISetting = SPCR;
	//chipSelect = aChipSelect; // store the chip select.
	Controllino_RTCSSInit(); 
	// start the SPI library:
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0); // both mode 1 & 3 should work
	//SW reset
	/*Controllino_SetRTCSS(HIGH);
	SPI.transfer(0x10); //writing in the control register
	SPI.transfer(0x58); //Software reset command
	Controllino_SetRTCSS(LOW);
	delay(10); // needed delay for reset*/
	isRTCInitialized = true;
	//Return the SPI settings to previous state
	SPCR = SPISetting;
	return 0;
}

/* This function sets the time and date to the connected RTC chip (RV-2123). Return code -1 means RTC chip was not properly initialized before. */
#define SKIPPEDINDEX 4 //Fifth value in the array is skipped
#define ARRAYSIZE 7 //Six time values and skipped index
//=====================================
char Controllino_SetTimeDate(unsigned char aDay, unsigned char aMonth, unsigned char aYear, unsigned char aHour, unsigned char aMinute, unsigned char aSecond)
{
	unsigned char TimeDate [ARRAYSIZE]={aSecond,aMinute,aHour,aDay,0,aMonth,aYear}; //we use a zero in the middle of the array as thats how the data are stored inside the the chip
	unsigned char i,a,b;//Help variable i is used to control for cycle and a,b are used to prepare time data for the RTC chip.
	unsigned char SPISetting; // variable to hold SPI setting
	if (isRTCInitialized)
	{
	//Store SPI setting
	SPISetting = SPCR;
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	//go through all the time values
	for(i = 0; i < ARRAYSIZE;i++)
	{
		if(i == SKIPPEDINDEX)
		{
			i++; //skip the weekday register
		}
		b = TimeDate[i]/10; //get the tens
		a = TimeDate[i]%10; // get the rest
		TimeDate[i]= a + (b<<4);   //Shift it and store it.
		//digitalWrite(chipSelect, LOW);
		Controllino_SetRTCSS(HIGH);
		SPI.transfer(i + 0x12); //0x12 is starting address for write 
		SPI.transfer(TimeDate[i]);
		//Serial.println(i + 0x12);
		//Serial.println(TimeDate[i]);
		//Serial.println();
		//digitalWrite(chipSelect, HIGH);
		Controllino_SetRTCSS(LOW);
	}
	//Return the SPI settings to previous state
	SPCR = SPISetting;
	return 0; 
	}
	else
	{
		return -1; //RTC chip was initialized properly, return -1
	}
}

/* This function reads the time and date from the connected RTC chip (RV-2123) and fills supplied variables. Return code -1 means RTC chip was not properly initialized before. */
//=====================================
char Controllino_ReadTimeDate(unsigned char *aDay, unsigned char *aMonth, unsigned char *aYear, unsigned char *aHour, unsigned char *aMinute, unsigned char *aSecond)
{
	unsigned char i,a,b; //help variables a,b are used to store modified time information for a while during readout, and i is used for for cycle
	unsigned int n; //n stores exact unmodified recieved data from the chip
	int timeDate [ARRAYSIZE]; //second,minute,hour,null,day,month,year
	unsigned char SPISetting; // variable to hold SPI setting	
	if (isRTCInitialized)
	{
		//Store SPI setting
		SPISetting = SPCR;
		SPI.setBitOrder(MSBFIRST);
		SPI.setDataMode(SPI_MODE0);
		for(i = 0; i < ARRAYSIZE;i++)
		{
			if(i == SKIPPEDINDEX)
			{ 
				i++; //skip the empty register
			}
			//digitalWrite(chipSelect, LOW);
			Controllino_SetRTCSS(HIGH);
			SPI.transfer(i + 0x92); //0x92 is starting address for read
			n = SPI.transfer(0x00);       
			//digitalWrite(chipSelect, HIGH);
			Controllino_SetRTCSS(LOW);
			//Serial.println(n);
			a = n & B00001111;   
			if(i == 2) //hours
			{  
				b = (n & B00110000)>>4; //24 hour mode
				if(b == B00000010)
				{
					b = 20;
				}
				else if(b == B00000001)
				{
					b = 10;
				}
				timeDate[i] = a + b;
			}
			else if(i == 3) //day
			{ 
				b = (n & B00110000)>>4;
				timeDate[i] = a + (b * 10);
			}
			else if(i == 5) //month
			{ 
				b = (n & B00010000)>>4;
				timeDate[i] = a + (b * 10);
			}
			else if(i == 6) //year
			{ 
				b = (n & B11110000)>>4;
				timeDate[i] = a + (b * 10);
			}
			else //minute,second
			{  
				b = (n & B01110000)>>4;
				timeDate[i] = a + (b * 10);
			}
		}
		//if supplied pointer was not NULL, return the recieved data
		if (aDay != NULL)
		{
			*aDay = timeDate[4];
		}
		
		if (aMonth != NULL)
		{
			*aMonth = timeDate[5];
		}
		
		if (aYear != NULL)
		{
			*aYear = timeDate[6];
		}
		
		if (aHour != NULL)
		{
			*aHour = timeDate[2];
		}
		
		if (aMinute != NULL)
		{
			*aMinute = timeDate[1];
		}
		
		if (aSecond != NULL)
		{
			*aSecond = timeDate[0];
		}
		//Return the SPI settings to previous state
		SPCR = SPISetting;
		return 0;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}

/* This function reads day of connected RTC chip and returns it. Return code -1 means RTC chip was not properly initialized before. */
//=================================================
char Controllino_GetDay()
{
	unsigned char day;
	if (Controllino_ReadTimeDate(&day, NULL, NULL, NULL, NULL, NULL) >= 0)
	{		
		return day;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}

/*This function reads month of connected RTC chip and returns it. Return code -1 means RTC chip was not properly initialized before. */
//=================================================
char Controllino_GetMonth()
{
	unsigned char month;
	if (Controllino_ReadTimeDate(NULL, &month, NULL, NULL, NULL, NULL) >= 0)
	{
		return month;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}

/*This function reads year of connected RTC chip and returns it. Return code -1 means RTC chip was not properly initialized before. */
//=================================================
char Controllino_GetYear()
{
	unsigned char year;
	if (Controllino_ReadTimeDate(NULL, NULL, &year, NULL, NULL, NULL) >= 0)
	{
		return year;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}

/*This function reads hour of connected RTC chip and returns it. Return code -1 means RTC chip was not properly initialized before. */
//=================================================
char Controllino_GetHour()
{
	unsigned char hour;
	if (Controllino_ReadTimeDate(NULL, NULL, NULL, &hour, NULL, NULL) >= 0)
	{	
		return hour;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}

/*This function reads minute of connected RTC chip and returns it. Return code -1 means RTC chip was not properly initialized before. */
//=================================================
char Controllino_GetMinute()
{
	unsigned char minute;
	if (Controllino_ReadTimeDate(NULL, NULL, NULL, NULL, &minute, NULL) >= 0)
	{		
		return minute;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}

/*This function reads second of connected RTC chip and returns it. Return code -1 means RTC chip was not properly initialized before. */
//=================================================
char Controllino_GetSecond()
{
	unsigned char second;
	if (Controllino_ReadTimeDate(NULL, NULL, NULL, NULL, NULL, &second) >= 0)
	{	
		return second;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}
/* This function reads time and date of connected RTC chip and prints it on serial line. This function expects that the serial line was initialized before calling it. Return code -1 means RTC chip was not properly initialized before.
Format is DD/MM/YY   HH:MM:SS */
//=================================================
char Controllino_PrintTimeAndDate()
{
	unsigned char day, month, year, hour, minute, second;
	if (Controllino_ReadTimeDate(&day, &month, &year, &hour, &minute, &second) >= 0)
	{
		Serial.print(day);
		Serial.print("/");
		Serial.print(month);
		Serial.print("/");
		Serial.print(year);
		Serial.print("   ");
		Serial.print(hour);
		Serial.print(":");
		Serial.print(minute);
		Serial.print(":");
		Serial.println(second);
		return 0;
	}
	else
	{
		return -1;//RTC chip was initialized properly, return -1
	}
}

#if defined(CONTROLLINO_MAXI) || defined(CONTROLLINO_MEGA)
/* RS485 initialization function. Serial3 still needs to be initialized separately. This only inits RE and DE pins. */
//=================================================
char Controllino_RS485Init()
{
	//set PORTJ pin 5,6 direction (RE,DE)
	DDRJ |= B01100000;
	//set RE,DE on LOW
	PORTJ &= B10011111;
	
	return 0;
}

/* RS485 RE pin (PORT J5) switch. Expected values are 0 or 1 of LOW or HIGH. */
//=================================================
char Controllino_SwitchRS485RE(char mode)
{
	if (mode == 0)
	{
		// set RE on LOW
		PORTJ &= B11011111;
	}
	else if (mode == 1)
		{
			// set RE on HIGH
			PORTJ |= B00100000;
		}
		else return -1; // Unknown mode value
	return 0;
}

/* RS485 DE pin (PORT J6) switch. Expected values are 0 or 1 of LOW or HIGH. */
//=================================================
char Controllino_SwitchRS485DE(char mode)
{
	if (mode == 0)
	{
		// set DE on LOW
		PORTJ &= B10111111;
	}
	else if (mode == 1)
		{
			// set DE on HIGH
			PORTJ |= B01000000;
		}
		else return -1; // Unknown mode value
	return 0;
}
#endif

/* RTC SS initialization function. */
//=================================================
char Controllino_RTCSSInit()
{
	#if defined(CONTROLLINO_MAXI) || defined(CONTROLLINO_MEGA)
	//set PORTJ pin 2 RTC SS direction
	DDRJ |= B00000100;	
	pinMode(CONTROLLINO_PIN_HEADER_DIGITAL_OUT_00,OUTPUT);
	// set RTC SS on LOW
	PORTJ &= B11111011;
	digitalWrite(CONTROLLINO_PIN_HEADER_DIGITAL_OUT_00, LOW);
	return 0;
	#endif
	#if defined(CONTROLLINO_MINI)
	pinMode(CONTROLLINO_PIN_HEADER_SS,OUTPUT);
	digitalWrite(CONTROLLINO_PIN_HEADER_SS, LOW);
	return 0;
	#endif
	return -2; // No Controllino
}

/* RTC SS switch function. Expected values are 0 or 1 of LOW or HIGH.*/
//=================================================
char Controllino_SetRTCSS(char mode)
{
	#if defined(CONTROLLINO_MAXI) || defined(CONTROLLINO_MEGA)
	if (mode == 0)
	{
		// set RTC SS on LOW
		PORTJ &= B11111011;
		digitalWrite(CONTROLLINO_PIN_HEADER_DIGITAL_OUT_00, LOW);
	}
	else if (mode == 1)
		{
			// set RTC SS on HIGH
			PORTJ |= B00000100;
			digitalWrite(CONTROLLINO_PIN_HEADER_DIGITAL_OUT_00, HIGH);
		}
		else return -1; // Unknown mode value
	return 0;
	#endif
	#if defined(CONTROLLINO_MINI)
	if (mode == 0)
	{
		// set RTC SS on LOW
		digitalWrite(CONTROLLINO_PIN_HEADER_SS, LOW);
	}
	else if (mode == 1)
		{
			// set RTC SS on HIGH
			digitalWrite(CONTROLLINO_PIN_HEADER_SS, HIGH);
		}
		else return -1; // Unknown mode value
	return 0;
	#endif
	return -2; // No Controllino
}

