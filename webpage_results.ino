#include <SPI.h>
#include <Ethernet.h>
#include <FreqCounter.h>
#include <Wire.h>
#include <SFE_BMP180.h>

// You will need to create an SFE_BMP180 object, here called "pressure":
SFE_BMP180 pressure;
#define ALTITUDE 20.0 // Altitude of SparkFun's HQ in Boulder, CO. in meters

int time = 0;    // track of time the Arduino board is running, in milliseconds

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);

// TEMT6000 
int temt6000Pin = 0;
int initTEMT6000 = 2;     // variable to compare the running time, used to enable by 5 seconds a reading for the TEMT6000 sensor
int ambientLight = 0; // Brightness for the CSS Ambient ligh box

// Analog Reflectance Sensor
// Define constants and variables
const int LED = 13;                   // sets the LED on pin 13
const int  STATE = 2;                 // sets pin 2 for sensor reading
int r_state = 0;                      // reset to zero the variable used to read the state of the OUT pin of the sensor

// HHD10 constants and variables definition
int freq, offset, sens, HHD10 = 5000;

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  //Analog Reflectance Sensor Setup
  pinMode (LED, OUTPUT);               // sets pin 13 as digital output
  pinMode (STATE, INPUT);              // sets pin 2 as digital input 

  // Humidity sensor setup
  sens   =  i2cRead2bytes(81, 10); //Read sensitivity from EEPROM
  offset =  i2cRead2bytes(81, 12); //Same for offset

  // BMP180 sensor setup
  Serial.begin(9600);
  Serial.println("REBOOT");

  // Initialize the sensor (it is important to get calibration values stored on the device).

  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }
} // end of setup()


void loop() {
  time=millis();
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 1");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");          
          client.println("<style>");
          client.println(".obstacle {");  
          client.println("border: 1px solid green ;");
          client.println("width: 200px ;");
          client.println("text-align: center;");
          client.println("}");    
          client.println(".light {");  
          client.println("border: 1px solid blue ;");
          client.print("background: hsl(62,14%,");client.print(ambientLight);client.println("%);");
          client.println("width: 200px ;");
          client.println("text-align: center;");
          client.println("color: white;");
          client.println("}");
          client.println("</style>");
          client.println("<body bgcolor=#E6FAFA>");


          // Analog reflectance sensor main code
          r_state = digitalRead(STATE); // reads the status of the sensor
          client.print("<h2>Obstacle: </h2>");
          if(r_state == 0){              // if is there an obstacle (OUT = 0)
            digitalWrite (LED, HIGH);   // turn on the led
            client.print("<div class=\"obstacle\" style=\"background:red;\">");
            client.println("DETECTED</div><br />");
          }
          else{
            digitalWrite (LED, LOW);    // turn off the led
            client.print("<div class=\"obstacle\">");
            client.println("CLEAR</div><br />");
          }

          // Ambient light sensor (TMP 6000) main code
          //if ((time%initTEMT6000) == 0){
          ambientLight = analogRead(temt6000Pin);
          client.print("<h2>Ambient light: </h2><div class=\"light\">");          
          client.print(ambientLight);
          ambientLight /= 10;
          client.print("</div><br />");
          //}

          // Barometric Pressure & Temperature sensor (BMP180) main code
          client.println("<h2> Barometric & Pressure: </h2><br />");
          char status;
          double T,P,p0,a;

          // Loop here getting pressure readings every 10 seconds.

          // If you want sea-level-compensated pressure, as used in weather reports,
          // you will need to know the altitude at which your measurements are taken.
          // We're using a constant called ALTITUDE in this sketch:

          client.println();
          client.print("provided altitude: ");
          client.print(ALTITUDE,0);
          client.print(" meters, ");
          client.print(ALTITUDE*3.28084,0);
          client.println(" feet<br />");

          // If you want to measure altitude, and not pressure, you will instead need
          // to provide a known baseline pressure. This is shown at the end of the sketch.

          // You must first get a temperature measurement to perform a pressure reading.

          // Start a temperature measurement:
          // If request is successful, the number of ms to wait is returned.
          // If request is unsuccessful, 0 is returned.

          status = pressure.startTemperature();
          if (status != 0)
          {
            // Wait for the measurement to complete:
            delay(status);

            // Retrieve the completed temperature measurement:
            // Note that the measurement is stored in the variable T.
            // Function returns 1 if successful, 0 if failure.

            status = pressure.getTemperature(T);
            if (status != 0)
            {
              // Print out the measurement:
              client.print("temperature: ");
              client.print(T,2);
              client.print(" deg C, ");
              client.print((9.0/5.0)*T+32.0,2);
              client.println(" deg F<br />");

              // Start a pressure measurement:
              // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
              // If request is successful, the number of ms to wait is returned.
              // If request is unsuccessful, 0 is returned.

              status = pressure.startPressure(3);
              if (status != 0)
              {
                // Wait for the measurement to complete:
                delay(status);

                // Retrieve the completed pressure measurement:
                // Note that the measurement is stored in the variable P.
                // Note also that the function requires the previous temperature measurement (T).
                // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
                // Function returns 1 if successful, 0 if failure.

                status = pressure.getPressure(P,T);
                if (status != 0)
                {
                  // Print out the measurement:
                  client.print("absolute pressure: ");
                  client.print(P,2);
                  client.print(" mb, ");
                  client.print(P*0.0295333727,2);
                  client.println(" inHg<br />");

                  // The pressure sensor returns abolute pressure, which varies with altitude.
                  // To remove the effects of altitude, use the sealevel function and your current altitude.
                  // This number is commonly used in weather reports.
                  // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
                  // Result: p0 = sea-level compensated pressure in mb

                  p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
                  client.print("relative (sea-level) pressure: ");
                  client.print(p0,2);
                  client.print(" mb, ");
                  client.print(p0*0.0295333727,2);
                  client.println(" inHg<br />");

                  // On the other hand, if you want to determine your altitude from the pressure reading,
                  // use the altitude function along with a baseline pressure (sea-level or other).
                  // Parameters: P = absolute pressure in mb, p0 = baseline pressure in mb.
                  // Result: a = altitude in m.

                  a = pressure.altitude(P,p0);
                  client.print("computed altitude: ");
                  client.print(a,0);
                  client.print(" meters, ");
                  client.print(a*3.28084,0);
                  client.println(" feet<br />");
                }
                else Serial.println("error retrieving pressure measurement\n");
              }
              else Serial.println("error starting pressure measurement\n");
            }
            else Serial.println("error retrieving temperature measurement\n");
          }
          else Serial.println("error starting temperature measurement\n");

          // HDD10 Humidity sensor main code
          client.println("<h2>Humidity: ");
          //Get Frequency
          FreqCounter::f_comp= 8;             // Set compensation to 12
          FreqCounter::start(1);            // Start counting with gatetime of 1ms
          while (FreqCounter::f_ready == 0)         // wait until counter ready 
            freq=FreqCounter::f_freq;            // read result
          //Calculate RH
          float RH =  (offset-freq)*sens/4096; //Sure, you can use int - depending on what do you need          
          client.print(RH);
          client.println("<br />");

          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
} // end of loop

// Function for HDD10 humidity sensor
int i2cRead2bytes(int deviceaddress, byte address)  
{
  // SET ADDRESS
  Wire.beginTransmission(deviceaddress);
  Wire.write(address); // address for sensitivity
  Wire.endTransmission();
  // REQUEST RETURN VALUE
  Wire.requestFrom(deviceaddress, 2);
  // COLLECT RETURN VALUE
  int rv = 0;
  for (int c = 0; c < 2; c++ )
    if (Wire.available()) rv = rv * 256 + Wire.read();
  return rv;

} // End of i2cReard2bytes()


