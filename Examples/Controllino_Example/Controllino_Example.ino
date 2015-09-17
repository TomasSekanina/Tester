#include <Controllino.h>
#include <SPI.h>

/*
This is basic example of arduino library use.
Any input or output can be referenced as CONTROLLINO_ and add the sign printed next to the I/O.
Remember please to install Controllino plugin before testing this sketch.
Also remember you need to select either CONTROLLINO MINI,CONTROLLINO MAXI or CONTROLLINO MEGA as your board.
*/

void setup() 
{
  /* Here we prepare D0 as output and A0 as input*/
  pinMode(CONTROLLINO_D0, OUTPUT);
  pinMode(CONTROLLINO_A0, INPUT);
  /* Here we initialize serial at 9600 baudrate for reporting */
  Serial.begin(9600);
  /* Now we report start of example */
  Serial.println("CONTROLLINO example sketch is starting now");
}

void loop() 
{
  /* We set digital output D0 to low voltage */
  digitalWrite(CONTROLLINO_D0, LOW);
  /* Now we read out the voltage on Analog input A0 and report it */
  Serial.print("A0: ");
  Serial.println(analogRead(CONTROLLINO_A0));
  /* We wait one second (1000 ms) and repeat the process */
  delay(1000);
  /* We set digital output D0 to high voltage now */
  digitalWrite(CONTROLLINO_D0, HIGH);
  /* And again we read out the voltage on Analog input A0 and report it */
  Serial.print("A0: ");
  Serial.println(analogRead(CONTROLLINO_A0));
  /* At least we wait again one second and let the loop repeat itself */
  delay(1000);
}

