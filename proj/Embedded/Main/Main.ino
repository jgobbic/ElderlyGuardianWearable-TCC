
//#include <PinChangeInt.h> 
#include <avr/io.h>

#include <avr/interrupt.h>

#include <util/atomic.h>

#include <util/delay.h>

#include <stdbool.h>

#define LED_PORT 1
#define BUTTON_PORT 9
#define TOUCH_PORT  8
//#define MIC_PORT 6
#define FLAME_PORT  7
#define SHOCK_PORT  6

#define HRTBIT_PORT A0


#define STATE_NORMAL 0
#define STATE_HRTBIT 1
#define STATE_SOS    2

#define PSTATE_ENTSOS 3
#define PSTATE_LEAVESOS 4

#define invertMicroStateMacro(a) ((a) = (a) : 0 ? 1)  //TODO: HAVE TO TEST IT

//Prototypes
void hrtBeatInterruptSetup();
void sosState();
void hrtbitState();
void checkSOSState();
void checkLongPress();
void updateScratch();

//****VARIABLES**////

//FSM STATE
  //uint8_t State = STATE_HRTBIT;  //TODO:CHANGE
  uint8_t State = STATE_NORMAL;
  
  //USEFUL  
  uint8_t myBanks[5][20];
  uint8_t sensors_byte;
  
//READING  
  volatile uint8_t readTouch = 0;
  volatile uint8_t readButtom = 1;  //Buttom Unpressed value is 1
  volatile uint8_t readFlame = 0;
  volatile uint8_t readShock = 0;

  volatile uint8_t updatedTouch = 0;
  volatile uint8_t updatedButtom = 0;
  volatile uint8_t updatedFlame = 0;
  volatile uint8_t updatedShock = 0;
  
//MicroSTATES and ACCELREADING
  AccelerationReading dvcAccel;
  uint32_t magnetude ;
  volatile uint8_t st_buzzer = 0;
  volatile uint8_t st_fell   = 0;
  volatile uint8_t st_emergency = 0;
  volatile uint8_t st_hrtbit = 0;
  volatile uint8_t st_shock = 0;
  uint32_t times_that_fell = 0;

  volatile uint8_t last_SOS;

  volatile uint8_t toScratchStates = 0 ;
  volatile uint8_t toScratchStatesOld = 0;

  volatile uint8_t toScratchBPM;


 volatile uint8_t buzzer_2sec = 1; //FIXME
 volatile uint8_t handled_3secPush = 0; //handled
 volatile uint8_t handled_6secPush = 0;

 volatile uint8_t handled_3secPushTouch =0;
  volatile uint8_t handled_6secPushTouch =0;
  
  uint8_t FALLINGSIZE_threshold = 65;
  uint8_t FALLINGTIMES_threshold = 3;

// Interrupt and timers
  volatile uint8_t last_PINB = PINB;
  volatile uint8_t fiveHundredMs = 0;

 volatile uint8_t pressingButtomStartTime;
 volatile uint8_t releasingButtomTime;

 volatile uint8_t pressingTouchStartTime;
 volatile uint8_t releasingTouchTime;

 volatile uint8_t buzzer_start_time;
 volatile uint8_t buzzer_stop_time = 255;
 static uint8_t buzzer_start_time_count = buzzer_start_time;

  
  
  
//**HEARTH BEAT VARIABLES**////
volatile int rate[10];                    // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P =512;                      // used to find peak in pulse wave, seeded
volatile int T = 512;                     // used to find trough in pulse wave, seeded
volatile int thresh = 530;                // used to find instant moment of heart beat, seeded
volatile int amp = 0;                   // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;      // used to seed rate array so we startup with reasonable BPM

volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.
  
uint8_t invertMicroState(uint8_t* mState){
  if(*mState == 0){ *mState=1;}
  else { *mState = 0;}
  return *mState;
}

//todo st_shock never goes to 0
void setup(){
  //Serial.begin(9600);
  cli();
  pinMode(LED_PORT,OUTPUT);
  pinMode(BUTTON_PORT,INPUT_PULLUP);
  pinMode(TOUCH_PORT,INPUT_PULLUP);
 
//  pinMode(MIC_PORT,INPUT_PULLUP);
  pinMode(FLAME_PORT,INPUT_PULLUP);
  pinMode(SHOCK_PORT,INPUT_PULLUP);

//sets the CHANGE PIN interruption
PCICR |= (1<<0);
PCMSK0 |= (0x3C);
////////////////////////////
sei();
  hrtBeatInterruptSetup();                 // sets up to read Pulse Sensor signal every 2mS (for the 16MHz ARDUINO) FIXME/TODO

  Bean.setAccelerationRange(4);

  Bean.setLedGreen(255);
}//end setup





void accelRead(){
  dvcAccel = Bean.getAcceleration();
  magnetude = abs(dvcAccel.xAxis) + abs(dvcAccel.yAxis) + abs(dvcAccel.zAxis);
  if(magnetude<FALLINGSIZE_threshold){
    times_that_fell++;
    for(uint8_t i = 0; i<=(FALLINGTIMES_threshold-1) ; i++){ //este FOR conta o numero maximo-1, pois ja caiu uma vez, para ativar o if que levou ao for.
      dvcAccel = Bean.getAcceleration();
      magnetude = abs(dvcAccel.xAxis) + abs(dvcAccel.yAxis) + abs(dvcAccel.zAxis);
      
      if(magnetude<FALLINGSIZE_threshold) times_that_fell++;
      
      if(times_that_fell==FALLINGTIMES_threshold){
        st_emergency = 1;
        State = STATE_SOS;                  ///UNCOMMENT
        Bean.setLed(255,0,0);
       
      }//if(times_that_fell)
      
    }//for
    if(times_that_fell>0 && st_shock==1 ){
        st_emergency = 1;
        State = STATE_SOS;
        Bean.setLed(255,0,0);
      }
    
    times_that_fell = 0;
    
  }//if mg>size	
	
}



void loop(){
  
  
 
 accelRead();
 updateScratch();
 checkTimers();
 
 switch(State){
   case STATE_NORMAL:
   {   
        normalState();
        break;
   }
   
   case STATE_SOS:
   {
        sosState();
        break;
   }
   
   case STATE_HRTBIT:
   {
        hrtbitState();
        break;
   }
   
 }//end switch
 
}//end loop

 
 
 //NOTE EXPLAIN WHY THIS WORKS.
void checkTimers(){
  static uint8_t old5Hundreds = 0;
  static uint8_t new5Hundreds = 0;
  old5Hundreds = new5Hundreds;
  new5Hundreds = fiveHundredMs;

  
  if( (old5Hundreds-new5Hundreds) != 0){ //if a new time, count++ on what is needed to count
     buzzer_start_time_count++;
  }
  //Serial.println("old5Hundreds new5Hundreds buzzer_start_time_count buzzer_start_time buzzer_stop_time");  
  //Serial.println(old5Hundreds);
  //Serial.println(new5Hundreds);
  //Serial.println(buzzer_start_time_count);
  //Serial.println(buzzer_start_time);
  //Serial.println(buzzer_stop_time);
  if(buzzer_2sec)
  {
     if(buzzer_start_time_count >= buzzer_stop_time)
     {
      //Serial.println("triggered");
      digitalWrite(LED_PORT,LOW);
      buzzer_2sec = 0;     //this marks that the buzzer is no longer in need to be turned on for 2 seconds
     }
  }
  
}

void updateScratch()
{
  static uint8_t oldScratchBuffer[2];
  uint8_t toScratchBuffer[2];
  uint8_t toScratchByteWrite;
  ScratchData toScratchRead;
  volatile uint8_t scratchByte;
  
  toScratchRead = Bean.readScratchData(2);   

  if( toScratchRead.data[0] & (1<<5) ) //if now it should measure BPM
  {
    if(State == STATE_NORMAL) //saida nao implementada TODO
      {
        State = STATE_HRTBIT;
        Bean.setLed(0,0,255);
        st_hrtbit = 1;
      }
      else
      {
        //the toScratchByteWrite will handle
      }
  }

  scratchByte = toScratchRead.data[0];
  
  toScratchByteWrite = ((readShock & 0x01)<<1 | (st_fell & 0x01)<<2 |
                          (st_hrtbit & 0x01)<<5 | (st_emergency & 0x01)<<7 ); //fix change emergency and flame

  toScratchBuffer[0] = toScratchByteWrite;
  toScratchBuffer[1] = toScratchBPM;
  
  if(oldScratchBuffer[0] != toScratchBuffer[0] || oldScratchBuffer[1] != toScratchBuffer[1])
  {
    Bean.setScratchData(2,toScratchBuffer,2);
  }
  
  if(scratchByte & 0x4){ 
      activateBuzzer2sec();
      Bean.setLedBlue(255);
    }else
    {
      Bean.setLedBlue(0);
    }
    
  if(scratchByte & 0x40){
      if(last_SOS == 0){ //se acabou de entrrar..
        
        }
      last_SOS = 1;
    }else
    {
      last_SOS = 0;  
    }
    //toScratch 

  oldScratchBuffer[0] = toScratchBuffer[0];
  oldScratchBuffer[1] = toScratchBuffer[1];
}

void normalState()
{
  checkLongPress();
  checkLongPressTouch();
  digitalWrite(LED_PORT,LOW);
}


//TODO REMOVE BUZZER buzzer_start_time eh redunte com buzzer_start_time_count
void activateBuzzer2sec(){
  buzzer_2sec = 1;
  digitalWrite(LED_PORT,HIGH);
  buzzer_start_time = fiveHundredMs;
  buzzer_start_time_count = buzzer_start_time;
  buzzer_stop_time = buzzer_start_time + 4u;
  
  }

  
void checkLongPress(){
  //known issue> if you take more than 16 seconds holding the buttom, it does not work properly (however it is not very threatening)
  
  static uint8_t emergencyButtom;
  static uint8_t pressingStartTime;
  static uint8_t releasingTime;
  static uint8_t pressing;
  static uint8_t delta_release;
  static uint8_t delta_pressing;

 //nao quero colocar mais codigo no ler tempo
 
  delta_release = releasingButtomTime - pressingButtomStartTime;
  

 
  if( readButtom == 0 ){
     pressingStartTime = pressingButtomStartTime;
     delta_pressing = fiveHundredMs - pressingStartTime;
  }

  if(delta_release < 4) //normal click
  {
    //do nothing
  }
  else if(delta_release>=4 && delta_release<= 36) //between 2s and 7s
  {
   // digitalWrite(LED_PORT, HIGH);
  }
  else if(delta_release > 36)
  {
   //digitalWrite(LED_PORT,LOW);
  }


 
  if(delta_pressing>6 && delta_pressing <10 && !handled_3secPush ){ //HOLD IT FOR MORE THAN 3 SECONDS
    handled_3secPush = 1;
    //Serial.println("Buttom Pressed for 3 seconeds");
    //delta_pressing=0;//FIXME this and bellow should not be here.?
    //pressingStartTime=0;
    st_emergency = 1;
    State = STATE_SOS;
    Bean.setLed(255,0,0);
    //activateBuzzer2sec();
 }
  
  
  if(delta_pressing>12 && !handled_6secPush ){ //HOLD IT FOR MORE THAN 6 SECONDS
    handled_6secPush = 1;
    //Serial.println("Buttom Pressed for 6 seconeds");
    delta_pressing=0;
    Bean.setLed(0,255,0);
    State = STATE_NORMAL;
    pressingStartTime=0;

    st_emergency = 0;
    
    //activateBuzzer2sec();
 }


    
}//end checkLongPress


void checkLongPressTouch(){
  //known issue> if you take more than 16 seconds holding the buttom, it does not work properly (however it is not very threatening)
  
  static uint8_t emergencyButtom;
  static uint8_t pressingStartTime;
  static uint8_t releasingTime;
  static uint8_t pressing;
  static uint8_t delta_release;
  static uint8_t delta_pressing;

 //nao quero colocar mais codigo no ler tempo
 
  delta_release = releasingTouchTime - pressingTouchStartTime;
  

 
  if( readTouch == 1 ){
     pressingStartTime = pressingTouchStartTime;
     delta_pressing = fiveHundredMs - pressingStartTime;
  }

  if(delta_release < 4) //normal click
  {
    //do nothing
  }
  else if(delta_release>=4 && delta_release<= 36) //between 2s and 7s
  {
   // digitalWrite(LED_PORT, HIGH);
  }
  else if(delta_release > 36)
  {
   //digitalWrite(LED_PORT,LOW);
  }

  //Serial.println("handled_3secPushTouch | handled_6secPushTouch | delta_pressing");
  //Serial.println(handled_3secPushTouch);
  //Serial.println(handled_6secPushTouch);
  //Serial.println(delta_pressing);

 
  if(delta_pressing>6 && delta_pressing<10  && !handled_3secPushTouch ){ //HOLD IT FOR MORE THAN 3 SECONDS
    handled_3secPushTouch = 1;
    //Serial.println("Buttom Pressed for 3 seconeds");
    //delta_pressing=0;//FIXME this and bellow should not be here.?
    //pressingStartTime=0;
    st_hrtbit = 1;
    State = STATE_HRTBIT;
    Bean.setLed(0,0,255);
    //activateBuzzer2sec();
 }
  
  
  if(delta_pressing>12 && !handled_6secPushTouch ){ //HOLD IT FOR MORE THAN 6 SECONDS
    handled_6secPushTouch = 1;
    //Serial.println("Buttom Pressed for 6 seconeds");
    delta_pressing=0;
    st_hrtbit = 0;
    State = STATE_NORMAL;
    Bean.setLed(0,255,0);
    pressingStartTime=0;
    
    //activateBuzzer2sec();
 }


    
}//end checkLongPressTouch




void hrtbitState()
{
  checkLongPressTouch();

  
  if (QS == true){     // A Heartbeat Was Found

                       // BPM and IBI have been Determined

                       // Quantified Self "QS" true when arduino finds a heartbeat


                                // Set 'fadeRate' Variable to 255 to fade LED with pulse

       // serialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial.

        QS = false;                      // reset the Quantified Self flag for next time

  }

}

void checkFreeFall(){
  
  
}//end checkFreeFall

void sosState(){
  checkLongPress();  
  if(fiveHundredMs%2)
  {

  Bean.setLed(255,0,255);
  }
  else
  {
  Bean.setLed(255,0,0);
  }
  
  digitalWrite(LED_PORT,HIGH);
  
}
//*********************************INTERRUPTIO *************************************//
ISR(PCINT0_vect) {
  uint8_t changed_bits;
  changed_bits = PINB ^ last_PINB;
  last_PINB = PINB;
  
  if (changed_bits & (1 << PINB3)) // FLAME INT FIXME
  {
    updatedFlame = 1;
    readFlame = digitalRead(FLAME_PORT);    
  }
  else if (changed_bits & (1 << PINB4))  //TOUCH
  {
    readTouch = digitalRead(TOUCH_PORT);
    
    if(readTouch == 0){ //READTOUCH 0 = NOT PRESSING IT
      
      releasingTouchTime = fiveHundredMs;
    }
    else
    {
      updatedTouch = 1;   //the UPDATED variable tells us a new action hapanned
      handled_3secPushTouch = 0; //this marks that the action triggered by the 3 seconds push has been handled  //FIXME?
      handled_6secPushTouch = 0;
      pressingTouchStartTime = fiveHundredMs;
    }
    
    updatedTouch = 1;
    readTouch = digitalRead(TOUCH_PORT);
  }
  else if (changed_bits & (1 << PINB2))  //SHOCK
  {
   updatedShock = 1;
   readShock = digitalRead(SHOCK_PORT);
   if(readShock){
    st_shock = 1;
    }
    else
    {
    ///
    }
  }
  else if (changed_bits & (1 << PINB5)) // BUTTOM
  {
    
    readButtom = digitalRead(BUTTON_PORT);
    
    if(readButtom == 1){ //READBUTTON 1 = NOT PRESSING IT
      releasingButtomTime = fiveHundredMs;
    }
    else
    {
      updatedButtom = 1;   //the UPDATED variable tells us a new action hapanned
      handled_3secPush = 0; //this marks that the action triggered by the 3 seconds push has been handled  //FIXME?
      pressingButtomStartTime = fiveHundredMs;
    }
    
  }
}
  
//*********************************HRTBIT_CODE***************************************//  


void hrtBeatInterruptSetup(){




        // Initializes Timer2 to throw an interrupt every 1mS.
        TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
        TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER
        OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 1000Hz SAMPLE RATE
        TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
        sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED

      }







// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.

// Timer 2 makes sure that we take a reading every 4 miliseconds

ISR(TIMER2_COMPA_vect){                         // triggered when Timer2 counts to 124
  static uint8_t counting4ms = 0;
  counting4ms++;

  if(counting4ms==250){ //manipulacao UINT8 mais rapida que uint16
    counting4ms=0;
    fiveHundredMs++;//maximum of 122.5 sec
    
    }

  if(State == STATE_HRTBIT) //if is a hearthbeat, it is, else just count time
  {
    cli();                                      // disable interrupts while we do this
    Signal = analogRead(HRTBIT_PORT);              // read the Pulse Sensor
    sampleCounter += 2;                         // keep track of the time in mS with this variable
    int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise
  
  
  
      //  find the peak and trough of the pulse wave
  
    if(Signal < thresh && N > (IBI/5)*3){       // avoid dichrotic noise by waiting 3/5 of last IBI
      if (Signal < T){                        // T is the trough
        T = Signal;                         // keep track of lowest point in pulse wave
      }
    }
  
  
  
    if(Signal > thresh && Signal > P){          // thresh condition helps avoid noise
      P = Signal;                             // P is the peak
    }                                        // keep track of highest point in pulse wave
  
  
  
    //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  
    // signal surges up in value every time there is a pulse
  
    if (N > 250){                                   // avoid high frequency noise
  
      if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){
        Pulse = true;                               // set the Pulse flag when we think there is a pulse
        digitalWrite(LED_PORT,HIGH);                // turn on pin 13 LED
        IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
        lastBeatTime = sampleCounter;               // keep track of time for next pulse
  
  
  
        if(secondBeat){                        // if this is the second beat, if secondBeat == TRUE
          secondBeat = false;                  // clear secondBeat flag
          for(int i=0; i<=9; i++){             // seed the running total to get a realisitic BPM at startup
            rate[i] = IBI;
          }
  
        }
  
  
  
        if(firstBeat){                         // if it's the first time we found a beat, if firstBeat == TRUE
          firstBeat = false;                   // clear firstBeat flag
          secondBeat = true;                   // set the second beat flag
          sei();                               // enable interrupts again
          return;                              // IBI value is unreliable so discard it
  
        }
  
  
  
  
  
        // keep a running total of the last 10 IBI values
        word runningTotal = 0;                  // clear the runningTotal variable
  
        for(int i=0; i<=8; i++){                // shift data in the rate array
          rate[i] = rate[i+1];                  // and drop the oldest IBI value
          runningTotal += rate[i];              // add up the 9 oldest IBI values
        }
  
        rate[9] = IBI;                          // add the latest IBI to the rate array
        runningTotal += rate[9];                // add the latest IBI to runningTotal
        runningTotal /= 10;                     // average the last 10 IBI values
        BPM = 120000/runningTotal;               // how many beats can fit into a minute? that's BPM! //CHANGED TO 120000
        //Serial.println("BPM : ");
        //Serial.println(BPM);
        toScratchBPM = (uint8_t)BPM;
        QS = true;                              // set Quantified Self flag
        // QS FLAG IS NOT CLEARED INSIDE THIS ISR
  
      }
    }
  
  
  
    if (Signal < thresh && Pulse == true){   // when the values are going down, the beat is over
      digitalWrite(LED_PORT,LOW);            // turn off pin 13 LED
      Pulse = false;                         // reset the Pulse flag so we can do it again
      amp = P - T;                           // get amplitude of the pulse wave
      thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
      P = thresh;                            // reset these for next time
      T = thresh;
    }
  
    if (N > 2500){                           // if 2.5 seconds go by without a beat
      thresh = 530;                          // set thresh default
      P = 512;                               // set P default
      T = 512;                               // set T default
      lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date
      firstBeat = true;                      // set these to avoid noise
      secondBeat = false;                    // when we get the heartbeat back
    }
    sei();                                   // enable interrupts when youre done!
  }//end of IF State = hrtbit
}// end isr



