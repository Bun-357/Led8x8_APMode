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
//keep pic in eeprom
byte dataPack[12][8];//row//col
//
byte dataT[8] = {0, 252, 114, 243, 127, 243, 114, 252};
byte data2D[8][8];
byte data2D0R[8][8];
byte data2D90R[8][8];
byte data2D180R[8][8];
byte data2D270R[8][8];
//

//String data_file = "";
boolean keep_data = false;
byte picN = 0;

String s_temp;
byte ac = 0;
byte file_size = 0;

//

int16_t ax, ay, az;
int16_t gx, gy, gz;
#define LED_PIN 2
#define ACCELEROMETER_SENSITIVITY 16384

bool blinkState = false;
unsigned long time_now, time_prev_control;
float accelRaw[3];


String readFiles();
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
      Serial.println((8 * picN));

      for (int i = (8 * picN); i < 8 + (8 * picN); i++) {
        EEPROM.write(i, dataT[i - ( (8 * picN))]);
        EEPROM.commit();
        t = i;
      }
      // EEPROM.end();
      Serial.print("save to eerom last :");
      Serial.println(t);
      //save num pic


      //
      for (int i = (8 * picN); i < 8 + (8 * picN ); i++) {
        dataT[i - ((8 * picN ))] = EEPROM.read(i);
      }
      Serial.println("finish saved to eerom");
      for (int i = 0; i < 8; i++) {
        Serial.println(dataT[i]);
      }

      picN++;
      //EEPROM.write(0, picN);
      // EEPROM.commit();

      showPic();
      String data_file = readFiles();
      server.send(200, "text/html", data_file);
      //data_file = "";
      keep_data = false;
      EEPROM.end();
    }

  }

}
void handleRoot() {
  //  String state = server.arg("pic");
  //  EEPROM.begin(512);
  //  int num = state.toInt();
  //  Serial.println(num);
  //  if (num > 0) {
  //    num -= 1;
  //    for (int i = 1 + (8 * num); i < 9 + (8 * num); i++) {
  //      dataT[i - (1 + (8 * num))] = EEPROM.read(i);
  //    }
  //    showPic();
  //     EEPROM.end();
  //  }
  /*

  */
  //int sizeF = file_size;
  //char temp[sizeF];
  // char c[sizeF];
  //data_file.toCharArray(c, sizeF);
  //snprintf(temp, sizeF,c,picN,555);
  //delay(100);
  String data_file = readFiles();
  server.send(200, "text/html", data_file);
  //data_file = "";

}//end void handleRoot()

void handle_chagePic() {
  //String state = server.arg("pic");
  EEPROM.begin(512);
  int num = server.arg("pic").toInt();
  Serial.println(num);
  if (num > 0) {
    num -= 1;
    for (int i = (8 * num); i < 8 + (8 * num); i++) {
      dataT[i - ((8 * num))] = EEPROM.read(i);
    }
    showPic();
    EEPROM.end();
  }

  /*

  */
  char temp[1020];
  snprintf ( temp, 1020,

             "<html>\
  <head>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>have %d of pic</p>\
    <button onclick=\"myFunction2()\">Show picture</button>\
   <input type=\"text\"  placeholder=\"chose number pictrue\" id=\"numP\"/>\
   <form action=\"http://192.168.4.1\">\
    <input type=\"submit\" value=\"back\">\
   </form>\
    <button onclick=\"myFunctionD()\">Delete all picture</button>\
  </body>\
  <script>\
  function myFunction2() {\
   sendTo = document.getElementById('numP').value;\
   var y = \"http://192.168.4.1/chagePic?pic=\"+sendTo;\
   var x = location.href = y;\
  }\
  function myFunctionD() {\
   var y = \"http://192.168.4.1/deletePic?delete=1\";\
   var x = location.href = y;\
  }\
  </script>\
</html>", picN



           );

  server.send(200, "text/html", temp);
  for (int i = 0; i < 1020; i++) {
    temp[i] = ' ';
  }
}

void handle_deletePic() {
  //String state = server.arg("delete");
  int d = server.arg("delete").toInt();
  Serial.print("revice ");
  Serial.println(d);
  /*

  */
  if (d == 1) {
    Serial.println("Start delete all picture");
    EEPROM.begin(512);
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0);
      EEPROM.commit();

      // turn the LED on when we're done
      // blink LED to indicate activity
      blinkState = !blinkState;
      digitalWrite(LED_PIN, blinkState);

    }
    delay(20);
    EEPROM.end();
    delay(500);
    // EEPROM.begin(512);
    //delay(500);
    // picN = EEPROM.read(0);
    checkPicInEEPROM();
  }
  String data_file = readFiles();
  server.send(200, "text/html", data_file);
  //data_file = "";
}

void handle_showPic() {
  Serial.println("handle_showPic");
  //String state = server.arg("pic");
  int num = server.arg("pic").toInt();
  EEPROM.begin(512);
  Serial.println(num);
  if (num > 0) {
    num -= 1;
    for (int i = (8 * num); i < 8 + (8 * num); i++) {
      dataT[i - ((8 * num))] = EEPROM.read(i);
    }
    showPic();
    EEPROM.end();
  }

  /*

  */
  char temp[2820];
  snprintf ( temp, 2820, "<html>\
             <body>\
             <style>\
             .intro {\
             border:1px solid #d3d3d3;\
             transform: rotate(90deg);\
           }\
             </style>\
             <p>have %d of pic</p>\
<canvas class=\"intro\" id=\"myCanvas1\" width=\"88\" height=\"88\"onclick=\"showPicture(1)\"></canvas>\
<canvas class=\"intro\" id=\"myCanvas2\" width=\"88\" height=\"88\"onclick=\"showPicture(2)\"></canvas>\
<canvas class=\"intro\" id=\"myCanvas3\" width=\"88\" height=\"88\"onclick=\"showPicture(3)\"></canvas>\
<canvas class=\"intro\" id=\"myCanvas4\" width=\"88\" height=\"88\"onclick=\"showPicture(4)\"></canvas>\
<canvas class=\"intro\" id=\"myCanvas5\" width=\"88\" height=\"88\"onclick=\"showPicture(5)\"></canvas>\
<canvas class=\"intro\" id=\"myCanvas6\" width=\"88\" height=\"88\"onclick=\"showPicture(6)\"></canvas>\
<form action=\"http://192.168.4.1\">\
    <input type=\"submit\" value=\"back\">\
   </form>\
<script>\
var z = 1;\
var zz = 0;\
var str = 0;\
var debug = 0;\
var keepDebug = 0;\
var nameIdC = [\"myCanvas1\",\"myCanvas2\",\"myCanvas3\",\"myCanvas4\",\"myCanvas5\",\
\"myCanvas6\",\"myCanvas7\",\"myCanvas8\",\"myCanvas9\",\"myCanvas10\"];\
var dataS1 = [%d,%d,%d,%d,%d,%d,%d,%d];\
var dataS2 = [%d,%d,%d,%d,%d,%d,%d,%d];\
var dataS3 = [%d,%d,%d,%d,%d,%d,%d,%d];\
var dataS4 = [%d,%d,%d,%d,%d,%d,%d,%d];\
var dataS5 = [%d,%d,%d,%d,%d,%d,%d,%d];\
var dataS6 = [%d,%d,%d,%d,%d,%d,%d,%d];\
copyX(dataS1,nameIdC[0]);\
copyX(dataS2,nameIdC[1]);\
copyX(dataS3,nameIdC[2]);\
copyX(dataS4,nameIdC[3]);\
copyX(dataS5,nameIdC[4]);\
copyX(dataS6,nameIdC[5]);\
function copyX(data,nameId) {\
var name = nameId;\
for(j= 0;j < 8;j++){\
   var xy = data[j];\
   zz = j;\
   var base1 = (xy).toString(2);\
   var b = reverseArr(base1);\
if(debug == 0){\
  for( v = 0;v<8;v++){\
    if((b[v] == 0) && (b[v+1] == 1)){\
        b[v] = 1;\
        v = 8;\
        debug = 1;\
    }\
  }\
}\
   for(i = 0;i<8;i++){\
     if((b[i] == 1)){\
        makePic(name);\
    }\
     if((b[i] == 0)){\
            z = z+1;\
            if(z > 7){\
              z = 0;\
             }\
      }\
    if(b[i] === undefined){\
      z = z+1;\
      if(z > 7){\
        z = 0;\
      }\
    }\
if((j == 0) && (i == 0)){\
   keepDebug = b[0];\
}\
   }\
 }\
}\
function makePic(name) {\
var c = document.getElementById(name);\
var ctx = c.getContext(\"2d\");\
ctx.fillStyle = \"#F00\";\
ctx.fillRect(0,0, 10, 10);\
var imgData = ctx.getImageData(0, 0, 10, 10);\
    if(str == 0){\
str++;\
    }else{\
    ctx.putImageData(imgData, (z*11), (zz*11));\
     if(keepDebug == 0){\
         ctx.clearRect(0,0, 10, 10);\
      }\
    z = z+1;\
   if(z > 7){\
       z = 0;\
    }\
  }\
}\
function reverseArr(input) {\
    var ret = new Array;\
    for(var i = input.length-1; i >= 0; i--){\
        ret.push(input[i]);\
    }\
    return ret;\
}\
 function showPicture(num) {\
   var y = \"http://192.168.4.1/showPic?pic=\"+num;\
   var x = location.href = y;\
  }\
</script>\
</body>\
</html>", picN,
             dataPack[0][7], dataPack[0][6], dataPack[0][5], dataPack[0][4], dataPack[0][3], dataPack[0][2], dataPack[0][1], dataPack[0][0],
             dataPack[1][7], dataPack[1][6], dataPack[1][5], dataPack[1][4], dataPack[1][3], dataPack[1][2], dataPack[1][1], dataPack[1][0],
             dataPack[2][7], dataPack[2][6], dataPack[2][5], dataPack[2][4], dataPack[2][3], dataPack[2][2], dataPack[2][1], dataPack[2][0],
             dataPack[3][7], dataPack[3][6], dataPack[3][5], dataPack[3][4], dataPack[3][3], dataPack[3][2], dataPack[3][1], dataPack[3][0],
             dataPack[4][7], dataPack[4][6], dataPack[4][5], dataPack[4][4], dataPack[4][3], dataPack[4][2], dataPack[4][1], dataPack[4][0],
             dataPack[5][7], dataPack[5][6], dataPack[5][5], dataPack[5][4], dataPack[5][3], dataPack[5][2], dataPack[5][1], dataPack[5][0]);

  server.send(200, "text/html", temp);

  for (int i = 0; i < 2820; i++) {
    temp[i] = ' ';
  }
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
  server.on("/showPic", handle_showPic);
  server.begin();
  Serial.println("HTTP server started");
  //

  //readFiles();
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

String readFiles() {
  String data = "";
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
        data = f.readString() ;
        //Serial.println(data_file) ;

        f.close();
      }
    }
    else {
      Serial.println( "No such f i l e found . " ) ;
    }
  }
  return data;
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
  //  picN = EEPROM.read(0);
  //  delay(100);
  //  Serial.print("I have ");
  //  Serial.print(picN);
  //  Serial.println(" pictrue in eerom");

  //read data
  //  if (picN > 0) {
  //    //
  //    int j = picN - 1;
  //    for (int i = 1 + ( 8 * j); i < 9 + ( 8 * j); i++) {
  //      dataT[i - (1 + ( 8 * j))] = EEPROM.read(i);
  //    }
  //  }
  int l = 0;
  for (int j = 0; j < 96; j += 8) {
    for (int i = 0; i < 8; i++) {
      dataPack[l][i] = EEPROM.read(i + j);
      if (j == 0) {
        dataT[i] = dataPack[l][i];
      }
    }
    //check have pic
    if ((dataPack[l][0] == 0) && (dataPack[l][1] == 0) && (dataPack[l][2] == 0) &&
        (dataPack[l][3] == 0) && (dataPack[l][4] == 0) && (dataPack[l][5] == 0) &&
        (dataPack[l][6] == 0) && (dataPack[l][7] == 0) ) {
      //pic number l has not

    } else {
      //have pic

      picN++;
    }
    l++;//0-12
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


