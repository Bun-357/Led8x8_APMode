/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "FS.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "LedControl.h"
#include <EEPROM.h>

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

/* Set these to your desired credentials. */
const char *ssid = "LED8X8";
const char *password = "thereisnospoon";

ESP8266WebServer server(80);
LedControl lc = LedControl(15, 0, 13, 1);
MPU6050 accelgyro;
//
int dataT[8] = {0, 252, 114, 243, 127, 243, 114, 252};
int data2D[8][8];
int data2D0R[8][8];
int data2D90R[8][8];
int data2D180R[8][8];
int data2D270R[8][8];
//

String data_file;
boolean keep_data = false;
int picN = 0;

String s_temp;
int ac = 0;
int file_size = 0;

//

int16_t ax, ay, az;
int16_t gx, gy, gz;
#define LED_PIN 2
#define ACCELEROMETER_SENSITIVITY 16384

bool blinkState = false;
unsigned long time_now, time_prev_control;
float accelRaw[3];


void readFiles();
void convertTo2D();
void rotea90();

void showPic();
void checkPicInEEPROM();
void setupMPU6050();

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/


void handle_led() {
  String state = server.arg("state");
  if (state.length() > 0) {

    //save data to eepom
    for (int i = 0; i < state.length(); i++) {
      String inChar = state.substring(i, i + 1);
      s_temp += inChar;
      if (inChar == ",") {
        dataT[ac] = s_temp.toInt();
        s_temp = "";
        ac++;
      }

      if (inChar == "!") {
        dataT[ac] = s_temp.toInt();
        //s_temp = "";
        ac = 0;
        Serial.println("data[]");
        s_temp = "";
        keep_data = true;
        Serial.println(dataT[0]);
        Serial.println(dataT[1]);
        Serial.println(dataT[2]);
        Serial.println(dataT[3]);
        Serial.println(dataT[4]);
        Serial.println(dataT[5]);
        Serial.println(dataT[6]);
        Serial.println(dataT[7]);
        Serial.println("$$$$data[]");

      }



    }//end slip string

    if (keep_data) {
      EEPROM.begin(512);
      int t = 0;
      Serial.print("statr add ");
      Serial.println(1 + (8 * picN));

      for (int i = 1 + (8 * picN); i < 9 + (8 * picN); i++) {
        EEPROM.write(i, dataT[i - (1 + (8 * picN))]);
        EEPROM.commit();
        t = i;
      }
      // EEPROM.end();
      Serial.print("save to eerom last :");
      Serial.println(t);
      //save num pic


      //
      for (int i = 1 + (8 * picN); i < 9 + (8 * picN ); i++) {
        dataT[i - (1 + (8 * picN ))] = EEPROM.read(i);
      }
      Serial.println("finish saved to eerom");
      for (int i = 0; i < 8; i++) {
        Serial.println(dataT[i]);
      }

      picN++;
      EEPROM.write(0, picN);
      EEPROM.commit();

      showPic();
      server.send(200, "text/html", data_file);
      keep_data = false;
      EEPROM.end();
    }

  }

}
void handleRoot() {
  String state = server.arg("pic");
  EEPROM.begin(512);
  int num = state.toInt();
  Serial.println(num);
  if (num > 0) {
    num -= 1;
    for (int i = 1 + (8 * num); i < 9 + (8 * num); i++) {
      dataT[i - (1 + (8 * num))] = EEPROM.read(i);
    }
    showPic();

  }
  /*

  */
  //int sizeF = file_size;
  //char temp[sizeF];
  // char c[sizeF];
  //data_file.toCharArray(c, sizeF);
  //snprintf(temp, sizeF,c,picN,555);
  server.send(200, "text/html", data_file);
  EEPROM.end();
}//end void handleRoot()

void handle_chagePic() {
  String state = server.arg("pic");
  EEPROM.begin(512);
  int num = state.toInt();
  Serial.println(num);
  if (num > 0) {
    num -= 1;
    for (int i = 1 + (8 * num); i < 9 + (8 * num); i++) {
      dataT[i - (1 + (8 * num))] = EEPROM.read(i);
    }
    showPic();
    EEPROM.end();
  }

  /*

  */
  char temp[1000];
  snprintf ( temp, 1000,

             "<html>\
  <head>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>have %d of pic</p>\
    <button onclick=\"myFunction2()\">send :)</button>\
   <input type=\"text\"  value=\"chose number pictrue\" id=\"numP\"/>\
   <form action=\"http://192.168.4.1\">\
    <input type=\"submit\" value=\"back\">\
   </form>\
   <form action=\"http://192.168.4.1/deletePic?d=\"1\"\">\
    <input type=\"submit\" value=\"clear all EEPROM\">\
   </form>\
  </body>\
  <script>\
  function myFunction2() {\
   sendTo = document.getElementById('numP').value;\
   var y = \"http://192.168.4.1/chagePic?pic=\"+sendTo;\
   var x = location.href = y;\
  }\
  </script>\
</html>", picN



           );

  server.send(200, "text/html", temp);
}

void handle_deletePic() {
  String state = server.arg("d");
  int d = state.toInt();
  /*

  */
  if (d == 1) {
    EEPROM.begin(512);
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 512; i++)
      EEPROM.write(i, 0);

    // turn the LED on when we're done

    digitalWrite(2, HIGH);
    EEPROM.end();
  }
  server.send(200, "text/html", data_file);
}

void setup() {


  // configure Arduino LED for
  pinMode(LED_PIN, OUTPUT);
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/led", handle_led);
  server.on("/chagePic", handle_chagePic);
  server.on("/deletePic", handle_deletePic);
  server.begin();
  Serial.println("HTTP server started");
  //

  readFiles();
  delay(1000);
  /*
    Setup MUP6050
  */
  setupMPU6050();
  /*
     End setup MUP6050
  */
  delay(1000);

  /*
     Setup EEPROM
  */
  checkPicInEEPROM();
  /*
     End setup EEPROM
  */
  delay(1000);

  /*
     Setup Led8x8
  */

  /*
    The MAX72XX is in power-saving mode on startup,
    we have to do a wakeup call
  */
  lc.shutdown(0, false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0, 8);
  /* and clear the display */
  lc.clearDisplay(0);
  //
  showPic();
  delay(500);
  /*
    End setup Led8x8
  */



  Serial.println("end void setup()");
}

void loop() {
  server.handleClient();

  // read raw accel/gyro measurements from device
  time_now = millis();

  if (time_now - time_prev_control >= 100) {
    time_prev_control = time_now;
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    accelRaw[0] = ax - 700;
    accelRaw[1] = ay - 100;
    accelRaw[2] = az + 1300;

    accelRaw[0]  /= ACCELEROMETER_SENSITIVITY;
    accelRaw[1] /= ACCELEROMETER_SENSITIVITY;
    accelRaw[2] /= ACCELEROMETER_SENSITIVITY;

    //    Serial.print(accelRaw[0]); Serial.print("\t");
    //    Serial.print(accelRaw[1]); Serial.print("\t");
    //    Serial.print(accelRaw[2]); Serial.println("\t");
    //check rotate led8x8 by accelRaw
    /*
       state

       angle   | accelRaw[0] | accelRaw[1] | accelRaw[2]
               |             |             |
        0      |      1      |       0     |      0
               |             |             |
        90'    |      0      |       1     |      0
               |             |             |
        180'   |      -1     |       0     |      0
               |             |             |
        270'   |       0     |      -1     |      0
               |             |             |
               |             |             |

    */
    if ((accelRaw[0] < 0.1) && (accelRaw[0] > -0.25) && (accelRaw[1] > 0.90) && (accelRaw[2] < 0.25) && (accelRaw[2] > -0.25)) {//90แก้ไข วางMPU6050บนPCBกลับหัว
      for (int col = 0; col < 8; col++) {
        for (int row = 0; row < 8; row++) {
          lc.setLed(0, col, row, data2D90R[col][row]);
        }
      }
    } else if ((accelRaw[0] < -0.8) && (accelRaw[1] <  0.25) && (accelRaw[1] > -0.25) && (accelRaw[2] < 0.25) && (accelRaw[2] > -0.25)) {//180  แก้ไข วางMPU6050บนPCBกลับหัว
      for (int col = 0; col < 8; col++) {
        for (int row = 0; row < 8; row++) {
          lc.setLed(0, col, row, data2D0R[col][row]);
        }
      }
    } else if ((accelRaw[0] < 0.1) && (accelRaw[0] >  -0.25) && (accelRaw[1] < -0.80) && (accelRaw[2] < 0.25) && (accelRaw[2] > -0.25)) { //270 แก้ไข วางMPU6050บนPCBกลับหัว
      for (int col = 0; col < 8; col++) {
        for (int row = 0; row < 8; row++) {
          lc.setLed(0, col, row, data2D270R[col][row]);
        }
      }
    } else if ((accelRaw[0] > 0.8) && (accelRaw[1] <  0.25) && (accelRaw[1] > -0.25) && (accelRaw[2] < 0.25) && (accelRaw[2] > -0.25)) {// 0แก้ไข วางMPU6050บนPCBกลับหัว
      for (int col = 0; col < 8; col++) {
        for (int row = 0; row < 8; row++) {
          lc.setLed(0, col, row, data2D180R[col][row]);
        }
      }
    }


    // blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
  }
  /*
     End loop check angle every 100ms
  */
}

void readFiles() {
  bool ok = SPIFFS.begin() ; // s t a r t the f i l e system
  if (ok) { // check i f the f i l e system works
    Serial.println(" ok ");
    bool exist = SPIFFS.exists ( "/index.html");

    if (exist) {
      Serial.println( "The f i l e e x i s t s ! " ) ;

      File f = SPIFFS.open( "/index.html", "r");
      if (!f) {
        Serial.println( " Something went wrong t r yi n g t o open the f i l e . . . " ) ;

      }
      else {
        int s = f.size() ; // s i z e o f the f i l e , not r e a l l y needed
        Serial.printf("Size = %d\ r \n", s) ;
        file_size = s;
        //Serial.println(file_size);
        data_file = f.readString() ;
        //Serial.println(data_file) ;

        f.close();
      }
    }
    else {
      Serial.println( "No such f i l e found . " ) ;
    }
  }
}//end function readFiles()

void convertTo2D() {


  for (int j = 0; j < 8; j++) {
    //#####

    for (int i = 0; i < 8; i++) {

      data2D[j][i] = bitRead(dataT[j], 7 - i);

      //

    }//i
  }//j

}

void rotea90() {
  //  for (int i = 0; i < 8; i++) {
  //    for (int j = 8 - 1; j >= 0; j--) {
  //      data2D[i][j] = data2D[j][i];
  //    }
  //  }
  //

  //  for (int j = 0; j < 8; ++j) {
  //        for (int i = 0; i < 8; ++i) {
  //            data2D[j][i] = data2D[7-i][j];
  //        }
  //    }

  // Transpose the matrix
  for ( int i = 0; i < 8; i++ ) {
    for ( int j = i + 1; j < 8; j++ ) {
      int tmp = data2D[i][j];
      data2D[i][j] = data2D[j][i];
      data2D[j][i] = tmp;
    }
  }

  // Swap the columns
  for ( int i = 0; i < 8; i++ ) {
    for ( int j = 0; j < 8 / 2; j++ ) {
      int tmp = data2D[i][j];
      data2D[i][j] = data2D[i][8 - 1 - j];
      data2D[i][8 - 1 - j] = tmp;
    }
  }
  //

  //
}//




void showPic() {
  //
  convertTo2D();//keep in data2D[][]
  //keep pic angle 0'
  for (int i = 0; i < 8; i++) {
    for (int j = 8 - 1; j >= 0; j--) {
      data2D0R[i][j] = data2D[i][j];

    }
  }
  //show pic in 0'
  for (int col = 0; col < 8; col++) {
    for (int row = 0; row < 8; row++) {
      lc.setLed(0, col, row, data2D0R[col][row]);
    }
  }

  //
  //

  for (int bo = 0; bo < 3 ; bo++) {
    rotea90();
    for (int i = 0; i < 8; i++) {
      for (int j = 8 - 1; j >= 0; j--) {
        if (bo == 0) {
          data2D90R[i][j] = data2D[i][j];
        }
        if (bo == 1) {
          data2D180R[i][j] = data2D[i][j];
        }
        if (bo == 2) {
          data2D270R[i][j] = data2D[i][j];
        }
      }
    }
  }
}

void checkPicInEEPROM() {
  EEPROM.begin(512);
  delay(500);
  picN = EEPROM.read(0);
  delay(100);
  Serial.print("I have ");
  Serial.print(picN);
  Serial.println(" pictrue in eerom");

  //read data
  if (picN > 0) {
    //
    int j = picN - 1;
    for (int i = 1 + ( 8 * j); i < 9 + ( 8 * j); i++) {
      dataT[i - (1 + ( 8 * j))] = EEPROM.read(i);
    }
  }
}


void setupMPU6050() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();

  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
}


