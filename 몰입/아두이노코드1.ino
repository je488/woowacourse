#include <Timer.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <Servo.h>
#include <Nextion.h>

#define dpin 2
#define dtype DHT11
DHT dht(dpin,dtype);
Servo myservo;
Timer t;

int vmotor = 11;//진동모터
int room_led_r = 32;//내방 30(B),31(R)
int room_led_g = 34;//임의(바꿀것)
int room_led_b = 36;
int bath_led_r = 25;//화장실 34(R), 35(B)
int bath_led_g = 27;//임의(바꿀것)
int bath_led_b = 29;
int din_led_r = 41;//부엌 38(R), 39(B)
int din_led_g = 43;//임의(바꿀것)
int din_led_b = 45;
int liv_led_r = 48;//거실 44(R), 45(B)
int liv_led_g = 50;//임의(바꿀것)
int liv_led_b = 52;

int f_r,f_g,f_b,d_r,d_g,d_b = 225; //화재, 초인종 알림 색상
int vib = 200; //진동세기

boolean flag = true; //화재알림이 한번만 발생하도록

NexPage page0 = NexPage(0, 0, "page0");
NexPage page1 = NexPage(1, 0, "page1");
NexPage page2 = NexPage(2, 0, "page2");
NexButton bfire = NexButton(1, 2, "b0");
NexButton bdoor = NexButton(2, 2, "b0");

boolean connectWiFi();
boolean connectServer(String cmd);
void checkTemp();
void checkColor();
void tempnhumi();
void stopBlink();

// Nextion 객체중 터치이벤트를 발생시키는 객체는 터치이벤트 리스트에 등록
NexTouch *nex_event_list[] = {
  &bfire, &bdoor,  NULL
};

int stop_done_d = 0; //nextion에서 확인버튼 누르면 진동모터 멈추고, 전구깜박임 멈춤
int stop_done_f = 0;

void bfire_Callback(void *ptr) {
    page0.show();
    //전구깜박임 멈추고 진동울림 멈추기
    stopBlink();
    digitalWrite(vmotor,LOW);
    stop_done_f = 1;
}

void bdoor_Callback(void *ptr) {
    page0.show();
    stopBlink();
    digitalWrite(vmotor,LOW);
    stop_done_d = 1;
}

void setup() 
{
  nexInit();
  Serial.begin(9600);
  Serial1.begin(9600);  
  Serial1.setTimeout(200);

  pinMode(vmotor, OUTPUT);

  pinMode(room_led_b, OUTPUT);
  pinMode(bath_led_b, OUTPUT);
  pinMode(liv_led_b, OUTPUT);
  pinMode(din_led_b, OUTPUT);
  pinMode(room_led_r, OUTPUT);
  pinMode(bath_led_r, OUTPUT);
  pinMode(liv_led_r, OUTPUT);
  pinMode(din_led_r, OUTPUT);

  boolean connected=false;
    while(connected == false)  
    {  
       if(connectWiFi())  
       {  
         connected = true;  
         break;  
       }  
    }
    
  if (!connected){while(1);}  
  //delay(500);
  
  Serial1.println("AT+CIPMUX=0");
  
  t.every(13000,checkTempnHumi); //10초에 한번씩 화재 및 온습도 체크
  t.every(7000,checkColor); //1분에 한번씩 알림색상체크

  bfire.attachPop(bfire_Callback, &bfire);
  bdoor.attachPop(bdoor_Callback, &bdoor);
}

void loop()
{
  nexLoop(nex_event_list);
  t.update();
  //초인종
  if(analogRead(A0) >= 400){
    doorbell();
  }

  //전구제어
  String cmd = "GET /get_data.php";
  cmd += "\r\n";
  boolean check = connectServer(cmd);
  if(check) {
    String t;
    if(Serial1.available())  
    {
      t = Serial1.readString();
//      Serial.println(t);
      int in1 = t.indexOf(":");
      int in2 = t.indexOf("end");
      String t1 = t.substring(in1+1,in2+3);
      int index1 = t1.indexOf("<br>");
      String t2 = t1.substring(0,index1);//1번째 led 상태 파싱값
      int index2 = t1.indexOf("2");
      String t3 = t1.substring(index2,t1.length());
      int index3 = t3.indexOf("<br>");
      String t4 = t3.substring(0,index3);//2번째 led 상태 파싱값
      int index4 = t1.indexOf("3");
      String t5 = t1.substring(index4,t1.length());
      int index5 = t5.indexOf("<br>");
      String t6 = t5.substring(0,index5);//3번째 led 상태 파싱값
      int index6 = t1.indexOf("4");
      String t7 = t1.substring(index6,t1.length());
      int index7 = t7.indexOf("<br>");
      String t8 = t7.substring(0,index7);//4번째 led 상태 파싱값

      if(t2.equals("1ON")) {
        analogWrite(room_led_b,128);
        analogWrite(room_led_r,128);
      }
      if(t2.equals("1OFF")) {
        digitalWrite(room_led_b,LOW);
        digitalWrite(room_led_r,LOW);
      }
      if(t4.equals("2ON")) {
        analogWrite(bath_led_b,128);
        analogWrite(bath_led_r,128);
      }
      if(t4.equals("2OFF")) {
        digitalWrite(bath_led_b,LOW);
        digitalWrite(bath_led_r,LOW);
      }
      if(t6.equals("3ON")) {
        analogWrite(din_led_b,128);
        analogWrite(din_led_r,128);
      }
      if(t6.equals("3OFF")) {
        digitalWrite(din_led_b,LOW);
        digitalWrite(din_led_r,LOW);
      }
      if(t8.equals("4ON")) {
        analogWrite(liv_led_b,128);
        analogWrite(liv_led_r,128);
      }
      if(t8.equals("4OFF")) {
        digitalWrite(liv_led_b,LOW);
        digitalWrite(liv_led_r,LOW);
      }
      
    }
    else {
      Serial.println("serial not available");
    } 
  }

  delay(150);

  if(analogRead(A0) >= 400){
    doorbell();
  }
  Serial.println("============1circle============");
}

void doorbell() {
    String cmd = "GET /update_switch.php?sw=ON\r\n";
    boolean tf = connectServer(cmd);
    analogWrite(vmotor, vib); //진동 울림
    page2.show();
    //delay(10000);
    for(int i=0;i<10;i++) {
          analogWrite(room_led_r,d_r);
          analogWrite(bath_led_r,d_r);
          analogWrite(din_led_r,d_r);
          analogWrite(liv_led_r,d_r);
          analogWrite(room_led_g,d_g);
          analogWrite(bath_led_g,d_g);
          analogWrite(din_led_g,d_g);
          analogWrite(liv_led_g,d_g);
          analogWrite(room_led_b,d_b);
          analogWrite(bath_led_b,d_b);
          analogWrite(din_led_b,d_b);
          analogWrite(liv_led_b,d_b);
          delay(500);
          stopBlink();
          delay(500);
          nexLoop(nex_event_list);
          if (stop_done_d == 1) {
            stop_done_d = 0;
            break;
          }
    }
    digitalWrite(vmotor,LOW);
    delay(150);
}

void checkTempnHumi()
{
   float temperature, humidity;
   String temp, humi;
   temperature = dht.readTemperature();
   humidity = dht.readHumidity();
   temp = (String)temperature;
   humi = (String)humidity;
   //Serial.println("temp : " + temp);
   //Serial.println("humi : " + humi);
   

   String cmd = "GET /insert_data2.php?temp=" + temp + "&humi=" + humi;
   cmd += "\r\n";
   boolean check = connectServer(cmd);
   
   delay(350);
   
   if(flag == true) {
    if(temperature >= 35) {
      Serial.println("FIRE!!");
      String cmd = "GET /insert_data.php?temp=" + temp;
      cmd += "\r\n";    
      boolean check = connectServer(cmd);
      page1.show();
      analogWrite(vmotor, vib); //진동 울림
      for(int i=0;i<10;i++) {
          analogWrite(room_led_r,f_r);
          analogWrite(bath_led_r,f_r);
          analogWrite(din_led_r,f_r);
          analogWrite(liv_led_r,f_r);
          analogWrite(room_led_g,f_g);
          analogWrite(bath_led_g,f_g);
          analogWrite(din_led_g,f_g);
          analogWrite(liv_led_g,f_g);
          analogWrite(room_led_b,f_b);
          analogWrite(bath_led_b,f_b);
          analogWrite(din_led_b,f_b);
          analogWrite(liv_led_b,f_b);
          delay(500);
          stopBlink();
          delay(500);
          nexLoop(nex_event_list);
          if (stop_done_f == 1) {
            stop_done_f = 0;
            break;
          }
      }
      digitalWrite(vmotor,LOW);
      flag = false;
    }
   }
   else {
    if(temperature < 35) {
      flag = true;
    }
   }

   delay(350);
}

void checkColor() {
  String cmd = "GET /get_color.php";
  cmd += "\r\n";
  boolean check = connectServer(cmd);
  if(check) {
    String t;
    if(Serial1.available())  
    {
      t = Serial1.readString();
//      Serial.println(t);
      int in1 = t.indexOf(":");
      int in2 = t.indexOf("end");
      String t1 = t.substring(in1+1,in2+3);//get_color 출력 값 파싱
      int index1 = t1.indexOf("<br>");
      String t2 = t1.substring(1,index1);//화재 RGB값 파싱
      int rin = t2.indexOf(",");
      f_r = t2.substring(0,rin).toInt();//화재 R
      String fire_gb = t2.substring(rin+1,t2.length());//화재 GB
      int gin = fire_gb.indexOf(",");
      f_g = fire_gb.substring(0,gin).toInt();//화재 G
      f_b = fire_gb.substring(gin+1,t2.length()).toInt();
//      Serial.println(f_r);
//      Serial.println(f_g);
//      Serial.println(f_b);
      
      int index2 = t1.indexOf("d");
      String t3 = t1.substring(index2,t1.length());
      int index3 = t3.indexOf("<br>");
      String t4 = t3.substring(1,index3);//초인종 RGB값 파싱
      rin = t4.indexOf(",");
      d_r = t4.substring(0,rin).toInt();//초인종 R
      String door_gb = t4.substring(rin+1,t4.length());//초인종 GB
      gin = door_gb.indexOf(",");
      d_g = door_gb.substring(0,gin).toInt();//초인종 G
      d_b = door_gb.substring(gin+1,t4.length()).toInt();

//      Serial.println(d_r);
//      Serial.println(d_g);
//      Serial.println(d_b);

      int index4 = t1.indexOf("v");
      String t5 = t1.substring(index4+1,t1.length());
      int index5 = t5.indexOf("<br>");
      String t6 = t5.substring(0,index5);//진동세기
      vib = t6.toInt();
    }
  }
  //delay(100);
}

void stopBlink() {
  digitalWrite(room_led_r,LOW);
  digitalWrite(bath_led_r,LOW);
  digitalWrite(din_led_r,LOW);
  digitalWrite(liv_led_r,LOW);
  digitalWrite(room_led_g,LOW);
  digitalWrite(bath_led_g,LOW);
  digitalWrite(din_led_g,LOW);
  digitalWrite(liv_led_g,LOW);
  digitalWrite(room_led_b,LOW);
  digitalWrite(bath_led_b,LOW);
  digitalWrite(din_led_b,LOW);
  digitalWrite(liv_led_b,LOW);
}

boolean connectWiFi() {    
   String cmd="AT+CWJAP=\"";  
   //cmd+="WIN-UVJR09CDUB0 5489";
   //cmd+="iptime";
   cmd+="Galaxy Note20 5G9434";
   //cmd+="U+Net85AB";
   cmd+="\",\"";  
   //cmd+="4294W0m%";
   //cmd+="00000000";
   cmd+="12345678";
   //cmd+="DDD4003923";
   cmd+="\"";  
   Serial1.println(cmd);   
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
