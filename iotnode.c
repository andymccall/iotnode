/* Warning, what follows is some very messy code.  It has been developed purely as a proof of concept
   to see what can be done with the Arduino.  This will be completely rewritten in time.
*/

#include <Arduino.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <RTClib.h>
#include <Button.h>
#include <Ethernet2.h>

RTC_DS1307 RTC;
BME280I2C BME;

Button button(2);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

const int buttonPin = 2;
int displayPage=0;

byte mac[] = {
        0x00, 0xAA, 0xBB, 0xCC, 0xDA, 0x02
     };

EthernetServer server(80);

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void update_network() {

  char displayBuffer[20];
  
  u8g2.drawStr( 0, 0, "IoT Node: Office");
  u8g2.drawStr( 110, 0, "3/3");
  u8g2.drawHLine(0,11,128);

  /* Display the IP Address */
  sprintf(displayBuffer, "IP: %d.%d.%d.%d", Ethernet.localIP()[0], Ethernet.localIP()[1], Ethernet.localIP()[2], Ethernet.localIP()[3]);
  u8g2.drawStr( 0, 13, displayBuffer );
  memset(displayBuffer, 0, sizeof displayBuffer);

  /* Display the Subnet Mask */
  sprintf(displayBuffer, "SN: %d.%d.%d.%d", Ethernet.subnetMask()[0], Ethernet.subnetMask()[1], Ethernet.subnetMask()[2], Ethernet.subnetMask()[3]);
  u8g2.drawStr( 0, 23, displayBuffer );
  memset(displayBuffer, 0, sizeof displayBuffer);

  /* Display the Gateway IP Address */
  sprintf(displayBuffer, "GW: %d.%d.%d.%d", Ethernet.gatewayIP()[0], Ethernet.gatewayIP()[1], Ethernet.gatewayIP()[2], Ethernet.gatewayIP()[3]);
  u8g2.drawStr( 0, 33, displayBuffer );
  memset(displayBuffer, 0, sizeof displayBuffer);

  /* Display the DNS IP Address */
  sprintf(displayBuffer, "DNS: %d.%d.%d.%d", Ethernet.dnsServerIP()[0], Ethernet.dnsServerIP()[1], Ethernet.dnsServerIP()[2], Ethernet.dnsServerIP()[3]);
  u8g2.drawStr( 0, 43, displayBuffer );
  memset(displayBuffer, 0, sizeof displayBuffer);
  
}

void update_server() {
  
  u8g2.drawStr( 0, 0, "IoT Node: Office");
  u8g2.drawStr( 110, 0, "2/3");
  u8g2.drawHLine(0,11,128);

  u8g2.drawStr( 0, 13, "REST: OK" );
  u8g2.drawStr( 0, 23, "REST Pass: HU75TY3" );
}

void update_stats() {

  char floatBuffer[7];
  char displayBuffer[23];
  float bmeBuffer(NAN);
 
  DateTime now = RTC.now(); 

  u8g2.drawStr( 0, 0, "IoT Node: Office");
  u8g2.drawStr( 110, 0, "1/3");
  u8g2.drawHLine(0,11,128);
 
  sprintf(displayBuffer,"Date: %02hhu/%02hhu/%02hhu", now.year(), now.month(), now.day() );
  u8g2.drawStr( 0, 13, displayBuffer);
  memset(displayBuffer, 0, sizeof displayBuffer);

  sprintf(displayBuffer, "Time: %02hhu:%02hhu:%02hhu", now.hour(), now.minute(), now.second() );
  u8g2.drawStr( 0, 23, displayBuffer );
  memset(displayBuffer, 0, sizeof displayBuffer);

  bmeBuffer = BME.temp(true);
  dtostrf(bmeBuffer, 5, 2, floatBuffer);
  sprintf(displayBuffer, "Temperature: %s\xB0 C", floatBuffer);
  u8g2.drawStr( 0, 33, displayBuffer);
  memset(floatBuffer, 0, sizeof floatBuffer);
  memset(displayBuffer, 0, sizeof displayBuffer);

  bmeBuffer = BME.hum();
  dtostrf(bmeBuffer, 5, 2, floatBuffer);
  sprintf(displayBuffer, "Humidity: %s %%", floatBuffer);
  u8g2.drawStr( 0, 43, displayBuffer);
  memset(floatBuffer, 0, sizeof floatBuffer);
  memset(displayBuffer, 0, sizeof displayBuffer);

  bmeBuffer = BME.pres(0);
  dtostrf(bmeBuffer, 6, 0, floatBuffer);
  sprintf(displayBuffer, "Pressure: %s Pa", floatBuffer);
  u8g2.drawStr( 0, 53, displayBuffer);
  memset(floatBuffer, 0, sizeof floatBuffer);
  memset(displayBuffer, 0, sizeof displayBuffer);

}


void setup(void) {
  Wire.begin();
  Ethernet.begin(mac);
  server.begin();
  RTC.begin();
  button.begin();
  
  while(!BME.begin()){
    delay(1000);
  }

  u8g2.begin();
  
}

void temperature(String &dest)
{

  float bmeBuffer(NAN);
  char floatBuffer[7];
  char displayBuffer[24];
  bmeBuffer = BME.temp(true);
  dtostrf(bmeBuffer, 5, 2, floatBuffer);
  sprintf(displayBuffer, "\"temperature\" : \"%s\",", floatBuffer);

  dest = displayBuffer;
   
}

void humidity(String &dest)
{

  float bmeBuffer(NAN);
  char floatBuffer[7];
  char displayBuffer[24];
  bmeBuffer = BME.hum();
  dtostrf(bmeBuffer, 5, 2, floatBuffer);
  sprintf(displayBuffer, "\"humidity\" : \"%s\",", floatBuffer);

  dest = displayBuffer;
   
}

void pressure(String &dest)
{

  float bmeBuffer(NAN);
  char floatBuffer[7];
  char displayBuffer[25];
  bmeBuffer = BME.pres();
  dtostrf(bmeBuffer, 6, 0, floatBuffer);
  sprintf(displayBuffer, "\"pressure\" : \"%s\",", floatBuffer);

  dest = displayBuffer;
   
}

void loop(void) {

  String displayBuffer;

  EthernetClient client = server.available();
   if (client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
         // Send a standard http response header
         client.println("HTTP/1.1 200 OK");
         client.println("Content-Type: application/json");
         client.println("Connnection: close");
         client.println();
         client.println("{");
         client.print("\"iot_node\": \"Office\",");
         client.print("\"iot_node_version\": \"0.1\",");
         temperature(displayBuffer);
         client.print(displayBuffer);
         client.print("\"temperature_unit\": \"celsius\",");
         humidity(displayBuffer);
         client.print(displayBuffer);
         client.print("\"humidity_unit\": \"percent\",");
         pressure(displayBuffer);
         client.print(displayBuffer);
         client.print("\"pressure_unit\": \"pa\"");
         client.println("\n}");
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
  }

  u8g2.clearBuffer();
  u8g2_prepare();

  if (button.pressed()) {
    displayPage++;
    if (displayPage>3) {
      displayPage=1;
    }
  }

  switch (displayPage) {
    case 1:
      update_stats();
      break;
    case 2:
      update_server();
      break;
    case 3:
      update_network();
      break;
  }

 u8g2.sendBuffer();
 delay(1000);

}
