/***********************************************************************
; ECE 362 - Mini-Project C Source File - Fall 2012                     
;***********************************************************************
;	 	   			 		  			 		  		
; Team ID: 17
;
; Project Name: Morse Code Converter
;
; Team Members:
;
;   - Team/Doc Leader: Zihao Lu           Signature: ______________________
;   
;   - Software Leader: Daniel Li          Signature: ______________________
;
;   - Interface Leader: Jiyuan Zhao       Signature: ______________________
;
;   - Peripheral Leader: Jinglun Huang    Signature: ______________________
;
;
; Academic Honesty Statement:  In signing above, we hereby certify that we 
; are the individuals who created this HC(S)12 source file and that we have
; not copied the work of any other student (past or present) while completing 
; it. We understand that if we fail to honor this agreement, we will receive 
; a grade of ZERO and be subject to possible disciplinary action.
;
;***********************************************************************
;
; The objective of this Mini-Project is to make a device that can convert
; user inputted characters into Morse code in the form of a blinking LED
; and a speaker. The device can also convert Morse code entered from a 
; push button into characters.
;
;
;***********************************************************************
;
; List of project-specific success criteria (functionality that will be
; demonstrated):
;
; 1. The device will have a mode where the user can input characters in
;1 	 a menu shown on the LCD. The inputted characters will be converted 
;	 into Morse code.
;
; 2. The device will have a mode where the user can input Morse code from
;	 a push button. The Morse code will be converted into characters and 
;	 displayed on the LCD.
;
; 3. The user can change modes by pressing push button corresponding to 
;	 each mode.
;
; 4. When Morse code is being output, an LED will blink the correct 
;	 length and amount of time. While the LED is on, a speaker will also
;	 be turned on. Similarly, while morse code is being input, the LED 
;	 and speaker will be on while the button is pressed.
;
; 5. When entering characters in the first mode, the user can perform
;	 a backspace operation by selecting "BS" and an enter operation by
;	 selecting "EN".
;
;***********************************************************************
;
;  Date code started: 4/9/2013
;
;  Update history:
;
;  Date: 4/9/2013  Name: Daniel Li   Update: Started coding, finished coding Mode 1
;
;  Date: 4/13/2013  Name: Daniel Li   Update: Finished Mode 2
;
;  Date: 4/15/2013  Name: Daniel Li   Update: Debugging
;
;
;***********************************************************************/
#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

// All funtions after main should be initialized here
char inchar(void);
void outchar(char x);

void fdisp(void);
void morse(void);
void clearinput(void);

void morseout(int length);
void timerwait(int waitcount);

void timerwait2(int waitcount);
void getchar(void);
void getcharerror(void);
void checkend(void);

void shiftout(byte outbyte);
void lcdwait(void);
void send_byte(byte outbyte);
void send_i(byte inst);
void chgline(byte curpos);
void print_c(byte outbyte);
void pmsglcd(char* outString, int lneint);

//  Variable declarations  	   			 		  			 		       
int leftpb	= 0;  // left pushbutton flag, PAD3
int rghtpb	= 0;  // right pushbutton flag, PAD2
int morsepb = 0;  // morse (mode2) pushbutton flag, PAD1
int charpb  = 0;  // char (mode1) pushbutton flag, PAD0
int prevpb	= 0;  // previous pushbutton state
int twait   = 0;  // amount of cycles the timer waits for
int waitmod = 250; // wait multiplier (250 ms)
int waitcnt = 0;  // counter for timer wait


int enterflag = 0;
char cursor = 0; //current location of the cursor
int page = 0;    //current page number
int i; //loop index variable

int endflag = 0;
int in[4]; //0 - none, 1 - dot, 2 - dash

char test;
int oncount; //amount of time the button was pressed
int offcount; //amount of time the button was off

int mode = 1;
char minput[17]; //string of morse input
char input[17]; //string input by the user
int index; //current index of input string


char morsetable[26]; //table of morse code values;
char morsetable2[256]; //dots and dashes -> letters


//;LCD COMMUNICATION BIT MASKS
int RS = 0x04;     //;RS pin mask (PTT[2])
int RW = 0x08;     //;R/W pin mask (PTT[3])
int LCDCLK  = 0x10;     //;LCD EN/CLK pin mask (PTT[4])

//;LCD INSTRUCTION CHARACTERS
char LCDON = 0x0F;     //;LCD initialization command
char LCDCLR = 0x01;     //;LCD clear display command
char TWOLINE = 0x38;     //;LCD 2-line enable command

char CURMOV = 0xFE;     //;LCD cursor move instruction

char LINE1 = 0x80;     //;LCD line 1 cursor position
char LINE2 = 0xC0;     //;LCD line 2 cursor position

int loop = 1;
	 	   		
/***********************************************************************
Initializations
***********************************************************************/
void  initializations(void) {

//; Set the PLL speed (bus clock = 24 MHz)
  CLKSEL = CLKSEL & 0x80; //; disengage PLL from system
  PLLCTL = PLLCTL | 0x40; //; turn on PLL
  SYNR = 0x02;            //; set PLL multiplier
  REFDV = 0;              //; set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; //; engage PLL


// Disable watchdog timer (COPCTL register)
  COPCTL = 0x40   ; //COP off; RTI and COP stopped in BDM-mode

         
//  Add additional port pin initializations here
    DDRT = 0x7F;    //PTT7 is input, PTT6-0 are outputs
    ATDDIEN = 0xCF; //program PAD7-6,3-0 pins as digital inputs   
	
/*
	Initialize PWM for output of a square wave on PTT0
;*/ 	   			 		  			 		  		
	MODRR = 0x01;	//route PTT0 to PWM
	PWME = 0x01;    //enable ch 0
	PWMPOL = 0x01;  //active high
	PWMCTL = 0x00;  //no concatenate
	PWMCAE = 0x00;  //left aligned
	PWMPRCLK = 0x06;// div 64
	PWMCLK = 0x00;  // use clock a on ch 0;
	PWMPER0 = 0xFF; // div 255
	PWMDTY0 = 0x00; // intially off	


//; Initialize SPI for baud rate of 6.25MHz, MSB first
//; - Consult 3.1.3 of the SPI Block User Guide     
//;  < add SPI initialization code here >
    SPIBR = 0x01;    // divisor = (1 + b000) * 2^(1 + b001) = 4
    SPICR1 = 0x50;
    SPICR2 = 0x00;

/*; Initialize TIM Ch 7 (TC7) for periodic interrupts every 1.0 ms
;    Enable timer subsystem                         
;    Set channel 7 for output compare
;    Set appropriate pre-scale factor and enable counter reset after OC7
;    Set up channel 7 to generate 1 ms interrupt rate
;    Initially disable TIM Ch 7 interrupts	 	   			 		  			 		  		
*/	 	   			 		  			 		  		
    TSCR1 = 0x80; //7 bit = enable
    TIOS = 0x80; //channel 7 output compare
    TSCR2 = 0x0C; //counter reset, prescale of 16    
    TC7 = 1500;  //  1500 * 16 = 0.001 * 24000000    
    TIE = 0x00; //channel 7 disable
    TIE = 0x80; //channel 7 enable interrupt

/* Initialize the LCD
       ; - pull LCDCLK high (idle) PTT_PTT4
       ; - pull R/W' low (write state) PTT_PTT3
       ; - turn on LCD (LCDON instruction)
       ; - enable two-line mode (TWOLINE instruction)
       ; - clear LCD (LCDCLR instruction)
       ; - wait for 2ms so that the LCD can wake up
*/
    PTT_PTT4 = 1;
    PTT_PTT3 = 0;
    send_i(LCDON);
    send_i(TWOLINE);
    send_i(LCDCLR);
    lcdwait();
    
// Initialize input strings
    clearinput();
    input[16] = 0; //end string with null
    for(i = 0; i < 15; i++) //clear minput
      minput[i] = ' ';
    minput[16] = 0; //end string with null

	 	   			 		  			 		  		
// Initialize RTI for 2.048 ms interrupt rate	
    RTICTL = RTICTL | 0x1F;
    CRGINT = CRGINT | 0x80;

// Initialize Morse Tables

    morsetable[0] = (1 << 6) | (2 << 4) | (0 << 2) | (0); //A 
    morsetable[1] = (2 << 6) | (1 << 4) | (1 << 2) | (1); //B 
    morsetable[2] = (2 << 6) | (1 << 4) | (2 << 2) | (1); //C 
    morsetable[3] = (2 << 6) | (1 << 4) | (1 << 2) | (0); //D 
    morsetable[4] = (1 << 6) | (0 << 4) | (0 << 2) | (0); //E 
    morsetable[5] = (1 << 6) | (1 << 4) | (2 << 2) | (0); //F
    morsetable[6] = (2 << 6) | (2 << 4) | (1 << 2) | (0); //G
    morsetable[7] = (1 << 6) | (1 << 4) | (1 << 2) | (0); //H
    morsetable[8] = (1 << 6) | (1 << 4) | (0 << 2) | (0); //I
    morsetable[9] = (1 << 6) | (2 << 4) | (2 << 2) | (2); //J
    morsetable[10] = (2 << 6) | (1 << 4) | (2 << 2) | (0); //K
    morsetable[11] = (1 << 6) | (2 << 4) | (1 << 2) | (0); //L
    morsetable[12] = (2 << 6) | (2 << 4) | (0 << 2) | (0); //M
    morsetable[13] = (2 << 6) | (1 << 4) | (0 << 2) | (0); //N
    morsetable[14] = (2 << 6) | (2 << 4) | (2 << 2) | (0); //O
    morsetable[15] = (1 << 6) | (2 << 4) | (2 << 2) | (1); //P
    morsetable[16] = (2 << 6) | (2 << 4) | (1 << 2) | (2); //Q
    morsetable[17] = (1 << 6) | (2 << 4) | (1 << 2) | (0); //R
    morsetable[18] = (1 << 6) | (1 << 4) | (1 << 2) | (0); //S
    morsetable[19] = (2 << 6) | (0 << 4) | (0 << 2) | (0); //T
    morsetable[20] = (1 << 6) | (1 << 4) | (2 << 2) | (0); //U
    morsetable[21] = (1 << 6) | (1 << 4) | (1 << 2) | (2); //V
    morsetable[22] = (1 << 6) | (2 << 4) | (2 << 2) | (0); //W
    morsetable[23] = (2 << 6) | (1 << 4) | (1 << 2) | (2); //X
    morsetable[24] = (2 << 6) | (1 << 4) | (2 << 2) | (2); //Y
    morsetable[25] = (2 << 6) | (2 << 4) | (1 << 2) | (1); //Z

    for(i = 0; i < 256; i++)
      morsetable2[i] = 0;
    morsetable2[(1 << 6) | (2 << 4) | (0 << 2) | (0)] = 0 + 'A';//A 
    morsetable2[(2 << 6) | (1 << 4) | (1 << 2) | (1)] = 1 + 'A';//B 
    morsetable2[(2 << 6) | (1 << 4) | (2 << 2) | (1)] = 2 + 'A';//C 
    morsetable2[(2 << 6) | (1 << 4) | (1 << 2) | (0)] = 3 + 'A';//D 
    morsetable2[(1 << 6) | (0 << 4) | (0 << 2) | (0)] = 4 + 'A';//E 
    morsetable2[(1 << 6) | (1 << 4) | (2 << 2) | (0)] = 5 + 'A';//F
    morsetable2[(2 << 6) | (2 << 4) | (1 << 2) | (0)] = 6 + 'A';//G
    morsetable2[(1 << 6) | (1 << 4) | (1 << 2) | (0)] = 7 + 'A';//H
    morsetable2[(1 << 6) | (1 << 4) | (0 << 2) | (0)] = 8 + 'A';//I
    morsetable2[(1 << 6) | (2 << 4) | (2 << 2) | (2)] = 9 + 'A';//J
    morsetable2[(2 << 6) | (1 << 4) | (2 << 2) | (0)] = 10 + 'A';//K
    morsetable2[(1 << 6) | (2 << 4) | (1 << 2) | (0)] = 11 + 'A';//L
    morsetable2[(2 << 6) | (2 << 4) | (0 << 2) | (0)] = 12 + 'A';//M
    morsetable2[(2 << 6) | (1 << 4) | (0 << 2) | (0)] = 13 + 'A';//N
    morsetable2[(2 << 6) | (2 << 4) | (2 << 2) | (0)] = 14 + 'A';//O
    morsetable2[(1 << 6) | (2 << 4) | (2 << 2) | (1)] = 15 + 'A';//P
    morsetable2[(2 << 6) | (2 << 4) | (1 << 2) | (2)] = 16 + 'A';//Q
    morsetable2[(1 << 6) | (2 << 4) | (1 << 2) | (0)] = 17 + 'A';//R
    morsetable2[(1 << 6) | (1 << 4) | (1 << 2) | (0)] = 18 + 'A';//S
    morsetable2[(2 << 6) | (0 << 4) | (0 << 2) | (0)] = 19 + 'A';//T
    morsetable2[(1 << 6) | (1 << 4) | (2 << 2) | (0)] = 20 + 'A';//U
    morsetable2[(1 << 6) | (1 << 4) | (1 << 2) | (2)] = 21 + 'A';//V
    morsetable2[(1 << 6) | (2 << 4) | (2 << 2) | (0)] = 22 + 'A';//W
    morsetable2[(2 << 6) | (1 << 4) | (1 << 2) | (2)] = 23 + 'A';//X
    morsetable2[(2 << 6) | (1 << 4) | (2 << 2) | (2)] = 24 + 'A';//Y
    morsetable2[(2 << 6) | (2 << 4) | (1 << 2) | (1)] = 25 + 'A';//Z    
     
    fdisp();
    chgline(LINE1 + cursor);
}
	 		  			 		  		
/***********************************************************************
Main
***********************************************************************/
void main(void) {
  DisableInterrupts;
	initializations(); 		  			 		  		
	EnableInterrupts;

	
  while(loop) {
  //loop    
    
//; If right pushbutton pressed, increment cursor
    if(rghtpb == 1){
        rghtpb = 0;
        if(mode == 1) {
          cursor++;
          if(cursor >= 16) { //if cursor goes over the bound, change page
            page = (page + 1) % 2; 
            cursor = 0;
          }
          fdisp();
          chgline(LINE1 + cursor);
        }
    }

//; If left pushbutton pressed, decrement cursor
    if(leftpb == 1){
        leftpb = 0;
        if(mode == 1) {
          cursor--;
          if(cursor < 0) { //if cursor goes over the bound, change page
            page = (page + 1) % 2;
            cursor = 15;
          }
          fdisp();
          chgline(LINE1 + cursor);
        }
    }

    
//; If char pushbutton pressed, enter the value into the input string, enter mode 1
    if(charpb == 1){  
        charpb = 0;
        if(mode == 2) { //change mode and clear input
          mode = 1;
          clearinput();
        }
                
        if(page == 0) {
          input[index] = 'A' + cursor; //insert character from first page
          index++;
        }
        else if(page == 1 && (cursor == 10 || cursor == 13)) { //insert space
          input[index] = ' ';
          index++;
        }
        else if(page == 1 && (cursor == 11 || cursor == 12)) { //backspace
          if(index > 0) {
            input[index - 1] = ' ';
            index--;
          }
        } 
        else if(page == 1 && (cursor == 14 || cursor == 15)) { //enter
          enterflag = 1;
        }
        else if(page == 1) {
          input[index] = 'Q' + cursor; //insert character from second page
          index++;
        }
        
        if(index >= 16 || enterflag == 1) {
          enterflag = 0;        
          morse(); //outputs the characters
          clearinput();
        }
        
        fdisp();
        chgline(LINE1 + cursor);        
    }
    
    //records the morse input, outputs characters    
    if(morsepb == 1){ 
      morsepb = 0; //intialize variables for mode2
      endflag = 0;  
      TIE = 0x80;      
      mode = 2;
      clearinput();
      
      if(PTAD_PTAD0 == 1) { //sound
        PTT_PTT6 = 1;
        PWMDTY0 = 0x10;
      }
      
      while(index < 16 && endflag == 0) //get characters until full or break
        getchar();
        
      TIE = 0x00;
      PTT_PTT6 = 0; //sound off   
      PWMDTY0 = 0x00;      
      morsepb = 0;
      charpb = 0;       
    } 
    
    _FEED_COP(); /* feeds the dog */
  } /* loop forever */
  /* please make sure that you never leave main */
}




/***********************************************************************                       
; RTI interrupt service routine: RTI_ISR
;
;  Initialized for 2.048 ms interrupt rate
;
;  Samples state of pushbuttons
;		PAD0 - morsepb (mode2)
;		PAD1 - charpb (mode1)
;		PAD2 - rghtpb 
;	  	PAD3 - leftpb
;***********************************************************************/
interrupt 7 void RTI_ISR(void)
{
  	// set CRGFLG bit 
  	CRGFLG = CRGFLG | 0x80; 

	if((prevpb & 0x80) && !PTAD_PTAD7) //mask PAD3
		leftpb = 1;
	if((prevpb & 0x40) && !PTAD_PTAD6) //mask PAD3
		rghtpb = 1;	

	if(!(prevpb & 0x08) && PTAD_PTAD3) //mask PAD3
		leftpb = 1;
	if(!(prevpb & 0x04) && PTAD_PTAD2) //mask PAD2
		rghtpb = 1;
	if(!(prevpb & 0x02) && PTAD_PTAD1) //mask PAD1
		charpb = 1;		
	if(!(prevpb & 0x01) && PTAD_PTAD0) //mask PAD0
		morsepb = 1;
	prevpb = PTAD;
}


/***********************************************************************                       
;  TIM interrupt service routine
;
;  Initialized for 1.0 ms interrupt rate
;
;	mode1 - interrupts enabled only during wait loops, timer controls the wait length
;	mode2 - interrupts always enabled, samples the PAD0 input every millisecond
;		 		  			 		  		
;***********************************************************************/
interrupt 15 void TIM_ISR( void)
{
  // set TFLG1 bit 
 	TFLG1 = TFLG1 | 0x80; 
// No need to add anything in the .PRM file, the interrupt number is included above
  if(mode == 1) {
    waitcnt++;
     if(waitcnt >= waitmod) {
       waitcnt = 0;
       twait--;
     }
     if(twait <= 0) {
       TIE = 0x00; //channel 7 interrupt disable
     }
  } 
  else if(mode == 2) {
    if(PTAD_PTAD0 == 1) {
      oncount++;
      PTT_PTT6 = 1;   //sound on
      PWMDTY0 = 0x10; 
    }
    if(PTAD_PTAD0 == 0) {
      offcount++;
      PTT_PTT6 = 0;   //sound off
      PWMDTY0 = 0x00;
    }
  }   
}


/***********************************************************************                              
;  fdisp: Display the current page of letters and the input       
;***********************************************************************/
void fdisp()
{	
  send_i(LCDCLR);
  chgline(LINE1);
  if(mode == 1) {
    if(page == 0)
      pmsglcd("ABCDEFGHIJKLMNOP", 1); //page 0 
    else if(page == 1)
      pmsglcd("QRSTUVWXYZ BS EN", 1); //page 1
  }
  else {
    	pmsglcd(minput, 1);
    	print_c(' ');
  }
      
  chgline(LINE2);
  pmsglcd(input, 2);
}

//morse - "input" is converted into morse code
void morse(void) {
  char morsecode; 

  int part1; //splits the letter into 4 parts
  int part2; //each part corresponds to a dot, dash, or nothing
  int part3;
  int part4;
  for(i = 0; i < index; i++) {
    morsecode = morsetable[input[i] - 'A']; //split char
    part1 = (morsecode & 0xC0) >> 6;
    part2 = (morsecode & 0x30) >> 4;
    part3 = (morsecode & 0x0C) >> 2;
    part4 = morsecode & 0x03;

    if(input[i] == ' ')
      timerwait(1); //delay for a space
    else {      
      if(part1 != 0) { //outputs each part
        morseout(part1);
      }
      if(part2 != 0) {
        timerwait(1);
        morseout(part2);
      }
      if(part3 != 0) {
        timerwait(1);
        morseout(part3);
      }
      if(part3 != 0) {
        timerwait(1);
        morseout(part4);
      }
              
    }    
    timerwait(3); //delay between chararacters
  }
}


/*morseout
    1: led on for 1   .
    2: led on for 3   -
*/
void morseout(int length) {
  if(length == 1) { // output '.'
    PTT_PTT6 = 1;
  	PWMDTY0 = 0x10;
    timerwait(1);
    PTT_PTT6 = 0;
	PWMDTY0 = 0x00;	
  }
  if(length == 2) { // output '-'
  	PWMDTY0 = 0x10;
    PTT_PTT6 = 1;
    timerwait(3);
    PTT_PTT6 = 0;
  	PWMDTY0 = 0x00;
  }
}

//clearinput - clears input string
void clearinput(void) {
  for(i = 0; i < 16; i++)
    input[i] = ' ';
  index = 0;
}

//timerwait - loop until a certain amount of interrupts occur
void timerwait(int cycle) {
  twait = cycle;
  TIE = 0x80; //enable timer interrupts
  while(twait > 0) {
    asm {
      nop
    }
  }
}

//timerwait2 - loop and count how many cycles the button is pushed
void timerwait2(int cycles) {
  oncount = 0;
  offcount = 0;
  TIE = 0x80;
  while( (oncount + offcount) < cycles ) {
    asm {
      nop
    }
  }
}


//getchar - converts morse input to character
void getchar(void) {
  in[0] = 0; //morse input is combined into a input for table2
  in[1] = 0;
  in[2] = 0;
  in[3] = 0;
  for(i = 0; i<4; i++)
    minput[i] = ' ';  
  
  for(i = 0; i < 4; i++) {    
    if(charpb == 1) { //break if entering mode 1
      endflag = 1;
      break;
    }
  
    timerwait2(waitmod*2);
    if(oncount >= waitmod*3/4 && oncount <= waitmod*3/2) { //if the input is the length of a '.'
      minput[i] = '.';
      fdisp();
      in[i] = 1;
    } 
    else if(oncount >= waitmod*3/2) { //if the input is the length of a '-'
      timerwait2(waitmod*2);
      minput[i] = '-';
      fdisp();
      in[i] = 2;
    } 
    else { //if no entry, then convert to character
      test = morsetable2[(in[0] << 6) | (in[1] << 4) | (in[2] << 2) | in[3]];       
      if(test != 0) {   //checks the entry matches a value in table2
        input[index++] = test;          
        fdisp();
        chgline(LINE2 + index);
        checkend();
        break;
      } 
      else 
        getcharerror();
    } 
  }
}

//getcharerror - enter if the input doesn't match a value in the table
//      		 shows error, restarts character entry
void getcharerror(void) {
  pmsglcd("Error           ", 1);
  for(i = 0; i < 5; i++){
    in[i] = 0;
    minput[i] = ' ';
  }
  i = -1;
  while( PTAD_PTAD0 == 0 && !charpb ) {
    if(leftpb == 1 && index > 0){
      leftpb = 0;
      input[--index] = ' ';
      fdisp();
    }
  } //wait for user
  pmsglcd("                ", 1);
}
  
//checkend - check for space or end of stream in morse input   
void checkend(void) {
  TIE = 0x80;
  oncount = 0;
  offcount = 0;
  i = 1;
  while(i < 40) { //counts number of cycles that the button is off
    if(leftpb == 1 && index > 0){
      leftpb = 0;
      input[index - 1] = ' ';
      index--;
      fdisp();
      i = 0;
    }
    
    timerwait2(waitmod/4);
    if(oncount >= waitmod/8)
      break;
    else
      i++;
  }
  if(i >= 39) //if over 10 cycles, break
    endflag = 1;
  else if(i >= 20) //if over 5 cycles, print space
    input[index++] = ' ';

}

/***********************************************************************                              
;  shiftout: Transmits the contents of register A to external shift 
;            register using the SPI.  It should shift MSB first.  
;             
;            MISO = PM[4]
;            SCK  = PM[5]
;***********************************************************************/
void shiftout(byte outbyte)
{
	
	while(!SPISR_SPTEF){}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
	SPIDR = outbyte;
	asm{	ldaa	#11				;//wait for 30 cycles for SPI data to shift out
	spwait:	dbne	a , spwait			
	}
}

/***********************************************************************                              
;  lcdwait: Delay for 2 ms
;***********************************************************************/
void lcdwait()
{
	asm{
			psha
			pshb
			pshc
			ldaa #$FF
			ldab #63

	lp2srt:	dbeq 	b , lpend
	lp1srt:	dbne	a , lp1srt
			    bra		lp2srt

	lpend:	pulc
			pulb
			pula
	}
}

/***********************************************************************                              
;  send_byte: writes contents of register A to the LCD
;***********************************************************************/
void send_byte(byte outbyte)
{
	//Shift out character
	//Pulse LCD clock line low->high
	//Wait 2 ms for LCD to process data
	shiftout(outbyte);
	PTT_PTT4 = 0;
	PTT_PTT4 = 1;
	PTT_PTT4 = 0;
	lcdwait();
}
/***********************************************************************                              
;  send_i: Sends instruction passed in register A to LCD  
;***********************************************************************/
void send_i(byte inst)
{
	PTT_PTT2 = 0;		//Set the register select line low (instruction data)
	send_byte(inst);	//Send byte
}

/***********************************************************************                        
;  chgline: Move LCD cursor to the cursor position passed in A
;  NOTE: Cursor positions are encoded in the LINE1/LINE2 variables
;***********************************************************************/
unsigned char cc;
void chgline(byte curpos)
{
	send_i((char)CURMOV);
	cc = curpos;
	send_i(curpos);
}

/***********************************************************************                       
;  print_c: Print character passed in register A on LCD ,            
;***********************************************************************/
void print_c(byte outbyte)
{
	PTT_PTT2 = 1;       //Set the register select line back to high (actual data)
	send_byte(outbyte);
}

/***********************************************************************                              
;  pmsglcd: pmsg, now for the LCD! Expect characters to be passed
;           by call.  Registers should return unmodified.  Should use
;           print_c to print characters.  
;***********************************************************************/
void pmsglcd(char * outString, int lneint)
{
  int cntt;
	cntt = 0;
	
	if(lneint == 1){chgline((char)LINE1);}
	else if(lneint == 2){chgline((char)LINE2);}
	
	while(outString[cntt] != 0){
		print_c(outString[cntt]);
		cntt++;
	}
}