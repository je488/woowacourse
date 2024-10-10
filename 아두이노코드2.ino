#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Servo.h>

LiquidCrystal_I2C tv(0x27, 16, 2);

Servo myservo;

boolean connectWiFi() {    
   String cmd="AT+CWJAP=\"";  
//   cmd+="WIN-UVJR09CDUB0 5489";
   cmd+="iptime";
   cmd+="\",\"";
//   cmd+="4294W0m%";
   cmd+="00000000";
   cmd+="\"";
   Serial1.println(cmd);
   //Serial.println(cmd);
   //delay(500);  

   if(Serial1.find("OK"))
   {
     Serial.println("OK, Connected to WiFi.");
     return true;
   }
   else
   {
     Serial.println("Can not connect to the WiFi.");
     return false;
   }
}

boolean connectServer(String cmd) {
    String ap = "AT+CIPSTART=\"TCP\",\"";
    ap += "13.124.245.104";
    ap += "\",80";
    Serial.println(ap);
    Serial1.println(ap);
    
    if(Serial1.find("Error"))
    {
        Serial.println( "TCP connect error" );
        return false;
    }

    Serial1.print("AT+CIPSEND=");
    Serial1.println(cmd.length());
    Serial.print(cmd);
    delay(300);
    
    if(Serial1.find(">"))
    {
      Serial.println("find >");
      Serial1.println(cmd);
    } else {
       Serial1.println("AT+CIPCLOSE");
       Serial.println("connect timeout");
       //delay(500);
       return false;
    }

    return true;
}

void setup() {
  Serial.begin(9600); 
  Serial1.begin(9600);  
  Serial1.setTimeout(200);
  tv.init();
  tv.noBacklight();
  tv.setCursor(0,0);
  tv.print("   Smart Home");

  Serial.println ("Setup started.");
  
  boolean connected=false;
  
  while(connected == false){
     if(connectWiFi()){
       connected = true;
       break;
     }
  }
  
  if (!connected){while(1);}
  //delay(500);
  
  Serial1.println("AT+CIPMUX=0");
}//setupEnd

void loop() {
  delay(100);
  
  String cmd = "GET /update_tv2.php";
  cmd += "\r\n";
  boolean check = connectServer(cmd);
  if(check){
    String t;
    if(Serial1.available()){
      t = Serial1.readString();
      int in1 = t.indexOf(":");
      int in2 = t.indexOf("end");
      String t1 = t.substring(in1+1,in2+3);
      int index1 = t1.indexOf("<br>");
      String t2 = t1.substring(0,index1);//tv 상태 파싱값
      int index2 = t1.indexOf("v");
      String t3 = t1.substring(index2,t1.length());
      int index3 = t3.indexOf("<br>");
      String t4 = t3.substring(0,index3);//valve 상태 파싱값
      
      if(t2.equals("tON")) {
        tv.backlight();
      }
      if(t2.equals("tOFF")) {
        tv.noBacklight();
      }
      if(t4.equals("vON")) {
        myservo.attach(9);
        myservo.write(90);
        delay(100);
        //myservo.detach();
      }
      if(t4.equals("vOFF")) {
        myservo.attach(9);
        myservo.write(-90);
        delay(100);
        //myservo.detach();
      }
      
    } else{
      Serial.println("serial not available");
    }
  }
  delay(300);
  
  if(digitalRead(4) == 1){
    String cmd = "GET /update_door.php?door=OPEN\r\n";
    boolean tf = connectServer(cmd);
    Serial.println(tf);
  }else{
    String cmd = "GET /update_door.php?door=CLOSE\r\n";
    boolean tf = connectServer(cmd);
    Serial.println(tf);
  }
  while (Serial1.available()) {
    Serial1.read(); 
  }
  delay(200);
  

  Serial.println("============1circle============");
}
