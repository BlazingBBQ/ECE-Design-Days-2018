#include "ArduinoMotorShieldR3.h"
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

#define TIMER_MAX 781 //OCR1A = [16 MHz / (2 * N * fDesired)] - 1, N is prescalar (1024)
//I put in a timer interrupt if you want one. Use the equation above and set TIMER_MAX to get fDesired.
//That is, it will call ISR(TIMER1_COMPA_vect) every 1/fDesired seconds. The default value gives 10 Hz.

ArduinoMotorShieldR3 md;
// Input from serial port
float inputByte;

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

void setup()
{
    Bridge.begin();

    // Listen for incoming connection only from localhost
    // (no one from the external network could connect)
    server.listenOnLocalhost();
    server.begin();

    md.init();
    md.setSpeed2(0, 0);
    md.clearBrake2();
    pinMode(ENCODER_1, INPUT); // set ENCODER_1 to input
    pinMode(ENCODER_2, INPUT); // set ENCODER_2 to input
    InitializeInterrupt();
    interrupts();
    // Serial.begin(115200); //115200 baud, 8 data bits, 1 stop bit, no parity, XON/XOFF flow control
    // while (!Serial) {
    //     ; // wait for serial port to connect. Needed for native USB port only
    // }
    // Serial.println("");
    // Serial.println("UW ECE Ideas Clinic Pitching Machine");
}

void loop()
{
    // Get clients coming from server
    YunClient client = server.accept();

    if (client) {
        process(client);

        client.stop();
    }

    delay(50); // Poll every 50ms
    return;
}

void InitializeInterrupt() //Don't mess with this function - it sets up the control registers for the IR sensor and timer interrupts
{
    cli();    // switch interrupts off while messing with their settings
    
    PCICR   = 0x02;   // Enable PCINT1 interrupt
    PCMSK1  = 0b00001100;
    
    PRR    &= ~0x04;   //Enable Timer/Counter1
    TCCR1A  = 0b00000000; //No output pins, WGM11 = 0, WGM10 = 0
    TCCR1B  = 0b00001101; //WGM12 = 1, WGM13 = 0, CS1 = 0b101 (clk/1024)
    OCR1A   = TIMER_MAX; //Set count
    TIMSK1 |= 0b00000010; //OCIE1A = 1 (interrupt enable for OCR1A)
    
    sei();    // turn interrupts back on
}

ISR(PCINT1_vect) //Encoder Interrupt Service Routine
{
//This will trigger each time either of the IR sensors experiences any change in state
}

ISR(TIMER1_COMPA_vect) //Timer Interrupt Service Routine
{
//This will trigger at a frequency determined by TIMER_MAX
}

void process(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');

  // is "digital" command?
  if (command == "digital") {
    digitalCommand(client);
  }
}

void digitalCommand(YunClient client) {
  int speed1, speed2;

  speed1 = client.parseInt();

  // Check for second '/'
  if (client.read() == '/') {
    speed2 = client.parseInt();
  } 
  else {
    speed2 = 0;
  }

  if (speed1 > 100) {
      speed1 = 100;
  } else if (speed1 < -100) {
      speed1 = -100;
  }

  if (speed2 > 100) {
      speed2 = 100;
  } else if (speed2 < -100) {
      speed2 = -100;
  }

  // Send feedback to client
  client.print(F("Speed 1: "));
  client.print(speed1);
  client.print(F(", Speed 2:  "));
  client.println(speed2);

  md.setSpeed2((float) (speed1 / 100), (float) (speed2 / 100));

  // Update datastore key with the current pin value
  String key = "D";
  key += speed1;
  Bridge.put(key, String(speed2));
}
