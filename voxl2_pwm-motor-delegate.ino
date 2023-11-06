// Copyright (c) 2022 ModalAI Inc.
// 
// voxl2_pwm-motor-delegate.ino
// Prototype Alternative to m0065 ESC to PWM feature.
// Converts M0052 234000 baud signal to 9600 baud to standard 50Hz PWM servos and ESCs
// NANO AT328 only. Nano Every uses a different processor and "broken" bootloader process (See NanoEvery docs)
// Ref: CW
//

// SIMPLE SUMMARY
// Convert incoming voxl2 serial servo-out-raw commands into pwm
//
// ============================= ON VOXL ===================================================      (wire)     =============================== ON Arduino =========================================
// PX4 mixing module [thr, ele, rud, motor1/2/3/4] --> servo output raw [1,2,..8, we define] --must match--> [voxl2 serial packet, 1,2,..8, we define] --> Nano DIO pin out [11,10..n, we define]

#include <SoftwareSerial.h>

// if OEM servo API can handle the performance use this, though all the settings need 
// to commands in degrees between 0-180!
//#include <Servo.h>
// else use the newer servo api that uses a independent hw timer and commands in microseconds.
// reported as higher performance
#include"ServoTimer2.h" 


//////////////////////  INCOMING PACKETS /////////////////////////

// What is supported and designed below! VOXL can go upto 12
int CHANNELS = 5;

// Packets come in the form: '*',chan1,chan2,chan3, chan4... basically delimiter is an '*'
// number represents the packet position of this value
ServoTimer2 servo_thr;  // 1
ServoTimer2 servo_ele;  // 2
ServoTimer2 servo_rud;  // 3
ServoTimer2 servo_arL;  // 4
ServoTimer2 servo_arR;  // 5

// Packet default states
// standard setup to every RC radio out there.
int last_servo_thr = 1100;   // OpenTX radios have a better range at 1000, appears some spektrums are 1100
int last_servo_arL = 1500;  
int last_servo_arR = 1500; 
int last_servo_ele = 1500; 
int last_servo_rud = 1500; 
int standard_servo = 1500; 


//////////////////////  NANO OUTPUTS  /////////////////////////

// DIO pin outs -- See NANO docs for schematic
int id_servo_thr = 11;  // D11  
int id_servo_arL = 3;   // D3
int id_servo_arR = 5;   // D5
int id_servo_ele = 10;  // D10
int id_servo_rud = 9;   // D9


///////////////// debugging //////////////////

// visual counter
long ctnn = 0;

// if you want to see what the NANO is doing connect a ftdi RX line to pin 17 and 
// you can see what's happening in real time in screen or minicom/cutecom
#ifdef EN_DEBUG
SoftwareSerial DebugSerial(16, 17);// RX (not use), TX
#endif


// output servo PWM state on DIO line.
void set_servo(int chan, int val)
{
#ifdef EN_DEBUG
    DebugSerial.print(chan);
    DebugSerial.print(":");
    DebugSerial.println(val);
#endif

  int acc = 0;
  switch (chan)
  {
    case 0:
      last_servo_thr = val;
      acc = 1;
      break;
    case 1:
      last_servo_arL = val;  
      acc = 1;
      break;
    case 2:
      last_servo_arR = val; 
      acc = 1;
      break;
    case 3:
      last_servo_ele = val;
      acc = 1;
      break;
    case 4:
      last_servo_rud = val; 
      acc = 1;
      break;
    default:
      acc = 0;
  }

  if (acc)
    led_status();

  
}

// setup outputs
void setup() {

  delay(1000);
  
#ifdef EN_DEBUG
  DebugSerial.begin(9600);
#endif

  // VOXL2 outputs at this rate, unchangeable
  Serial.begin(230400);

#ifdef EN_DEBUG
    DebugSerial.println("VOXL2 ready");
#endif

  delay(1000);

#ifdef EN_DEBUG
    DebugSerial.println("Arduino Ready");
#endif

  servo_thr.attach(id_servo_thr);  
  servo_arL.attach(id_servo_arL);  
  servo_arR.attach(id_servo_arR);  
  servo_ele.attach(id_servo_ele);  
  servo_rud.attach(id_servo_rud); 

  set_servo(0, last_servo_thr);
  set_servo(1, standard_servo);
  set_servo(2, standard_servo);
  set_servo(3, standard_servo);
  set_servo(4, standard_servo);

  // tell user visually I'm running
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i=0; i< 10; i++)
  {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(300);                        // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      delay(300);  
  }
}

// tell user visually I'm getting data from VOXL2
void led_status()
{
  if (ctnn == 25)
  {
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
  }
  else if (ctnn == 50)
  {
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
    ctnn = 0;
  }
  ctnn++;
}

void loop() {

  if (Serial.available() > 0) {
       int incomingByte = Serial.read();
       if (incomingByte == '*')
       {    
          servo_ele.write(last_servo_ele);
          servo_thr.write(last_servo_thr);
          servo_arL.write(last_servo_arL);
          servo_arR.write(last_servo_arR);
          servo_rud.write(last_servo_rud);
          
          int getfive = 0;
          while (getfive < CHANNELS)
          { 
              if (Serial.available() > 0)           
              {
                int val = Serial.read();
                
#ifdef EN_DEBUG                
                DebugSerial.print(getfive);
                DebugSerial.print(":");
                DebugSerial.println(val);
#endif                  

                  // IF using Servo API
                  //int val_out = map(val, 0, 255, 0, 180);

                  // IF using ServoTimer2 API
                  int val_out = 1000;
                  if (getfive == 0) // Throttle range is always different
                    val_out = map(val, 0, 255, 1100, 1890);
                  else
                    val_out = map(val, 0, 255, 2100, 760);
                    
                  set_servo(getfive, val_out);
                  getfive++;

            }
           }
       }
  }
  
}
