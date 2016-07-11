// Author: Indranil Chandra and Vaibhav Yengul
// Linkit One code for Godrej Cooking Aid Probe
// The code has been released under MIT License
// Add all the below libraries and its dependancies for the code to work

///////////////////Libraries//////////////////////////////////////////////////////////////////////////////////////////
#include <OneWire.h> 
#include <Wire.h>
#include <LAudio.h>
#include <Suli.h>
#include <Seeed_LED_Bar_Arduino.h>
#include <String>

/////////////////Global Vairables/////////////////////////////////////////////////////////////////////////////////////
int TouchButton = 3; //TouchButton on Pin 3
int DS18S20_Pin = 7; //DS18S20 Signal on Pin 7
SeeedLedBar bar(5, 4);  // CLK, DTA -> Connect the Vcc pin of LED bar on 3.3V

OneWire ds(DS18S20_Pin); 
float temperature;
float prev_temp = 27.0;
float HighTemp = 40;
float LowTemp = 30;

volatile int buttonPress = 1;

//////////////////////////////////Struct recipeData//////////////////////////////////////////////////////////////
struct recipeData {
  int nSteps;
  int rTime[100];
  float rTemp[100];
};

///////////////////ISR///////////////////////////////////////////////////////////////////////////////////////////////
void buttonPressISR() {
  buttonPress = 0;
}

/////////////////PLAY_Audio//////////////////////////////////////////////////////////////////////////////////////////
void PLAY_Audio(char* song,int vol) {
  LAudio.playFile(storageFlash,song);
  LAudio.setVolume(vol);
  delay(2500);
}

////////////////Retrieve Temperature Function////////////////////////////////////////////////////////////////////////
float getTemp(float prevTemp){
  //returns the temperature from one DS18S20 in DEG Celsius
  byte data[12];
  byte addr[8];
  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      Serial.println("Device Address could not be found in search");
      return prevTemp;
  }
  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return prevTemp;
  }
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.println("Device is not recognized");
      return prevTemp;
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end
  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad
  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  ds.reset_search();
  byte MSB = data[1];
  byte LSB = data[0];
  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  if(TemperatureSum >= -25 && TemperatureSum <= 250){
    return TemperatureSum;
  }
  else{
    return prevTemp;
  } 
}


///////////////////Parse Data for Play Mode/////////////////////////////////////////////////////////////////////////
struct recipeData parseDataPlay(String receivedData){ 
  struct recipeData rData;
  
  int semicolonindex1,semicolonindex2,semicolonindex3,semicolonindex4;
  semicolonindex1 = receivedData.indexOf(";");
  semicolonindex2 = receivedData.indexOf(";",semicolonindex1+1);
  semicolonindex3 = receivedData.indexOf(";",semicolonindex2+1);
  semicolonindex4 = receivedData.indexOf(";",semicolonindex3+1);
  
  String mode = receivedData.substring(0, semicolonindex1);
  String stepCount = receivedData.substring(semicolonindex1+1, semicolonindex2);
  String stepsList = receivedData.substring(semicolonindex2+1, semicolonindex3);
  String temperatureList = receivedData.substring(semicolonindex3+1, semicolonindex4);
  String timeList = receivedData.substring(semicolonindex4+1);
  
  int count = stepCount.toInt();
  Serial.println("   ");
  Serial.print("No. of steps: ");
  Serial.println(count);
  Serial.println("   ");
  
  rData.nSteps = count;
  
  String stepsArray[count];
  stepsList.replace("["," ") ;
  stepsList.replace("]",",") ;
  Serial.println("Recipee Steps are: ");
  for(int i=0; i < count; i++){
     int commaIndex = stepsList.indexOf(",");
     stepsArray[i] = stepsList.substring(1, commaIndex);
     Serial.print("Step");
     Serial.print(i+1);
     Serial.print(": ");
     Serial.println(stepsArray[i]);
     if(commaIndex+1 != stepsList.length())
       stepsList = stepsList.substring(commaIndex+1, stepsList.length());
  }
  Serial.println("   ");
  
  float tempArray[count];
  temperatureList.replace("["," ") ;
  temperatureList.replace("]",",") ;
  Serial.println("Temperature Array: ");
  for(int i=0; i < count; i++){
     int commaIndex = temperatureList.indexOf(",");
     String x = temperatureList.substring(1, commaIndex);
     tempArray[i] = x.toFloat();
     rData.rTemp[i] = tempArray[i];
     Serial.print(tempArray[i]);
     Serial.print("  ");
     if(commaIndex+1 != temperatureList.length())
       temperatureList = temperatureList.substring(commaIndex+1, temperatureList.length());
  }
  Serial.println("   ");
  
  float timeArray[count];
  timeList.replace("["," ") ;
  timeList.replace("]",",") ;
  Serial.println("Time Array: ");
  for(int i=0; i < count; i++){
     int commaIndex = timeList.indexOf(",");
     String x = timeList.substring(1, commaIndex);
     timeArray[i] = x.toFloat();
     rData.rTime[i] = timeArray[i];
     Serial.print(timeArray[i]);
     Serial.print("  ");
     if(commaIndex+1 != timeList.length())
       timeList = timeList.substring(commaIndex+1, timeList.length());
  }
  Serial.println("   ");
  
  return rData;
}


/////////////////////////Parse Data for Record Mode////////////////////////////////////////////////////////////////
int parseDataRecord(String receivedData){ 
  int semicolonindex1,semicolonindex2,semicolonindex3,semicolonindex4;
  semicolonindex1 = receivedData.indexOf(";");
  semicolonindex2 = receivedData.indexOf(";",semicolonindex1+1);
  
  String mode = receivedData.substring(0, semicolonindex1);
  String stepCount = receivedData.substring(semicolonindex1 + 1, semicolonindex2);
  String stepsList = receivedData.substring(semicolonindex2 + 1) ;
  
  int count = stepCount.toInt();
  Serial.println("  ");
  Serial.print("No. of steps: ");
  Serial.println(count);
  Serial.println("   ");
  
String stepsArray[count];
  stepsList.replace("["," ") ;
  stepsList.replace("]",",") ;
  Serial.println("Recipee Steps are: ");
  for(int i=0; i < count; i++){
     int commaIndex = stepsList.indexOf(",");
     stepsArray[i] = stepsList.substring(1, commaIndex);
     Serial.print("Step");
     Serial.print(i);
     Serial.print(": ");
     Serial.println(stepsArray[i]);
     if(commaIndex+1 != stepsList.length())
       stepsList = stepsList.substring(commaIndex+1, stepsList.length());
  }
  Serial.println("   ");
  
  return count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////recordMode()////////////////////////////////////////////////////////////////
String recordMode(int numberOfSteps){
  int recipeFinishFlag = 0;
  int stepCounter = 1;
  int timeCounter = 0;
        
  while(recipeFinishFlag == 0){
    temperature = getTemp(prev_temp);
    String senddata = "#" + String(temperature) + "," + String(timeCounter) + "\n";
    char *s  = &senddata[0] ; 
    Serial.print("Sending Data: ");
    Serial.println(senddata);
    for(int i=0 ; i<senddata.length() ; i++){
      s  = &senddata[i] ;
      Serial1.write(*s);
    }
    prev_temp = temperature;       
          
    if(buttonPress == 0 && stepCounter < numberOfSteps){
      Serial.println("Touch Button Pressed");
      senddata = " Step " + String(stepCounter+1) + " started" + "\n";
      s  = &senddata[0] ; 
      Serial.print("Sending Data: ");
      Serial.println(senddata);
      for(int i=0 ; i<senddata.length() ; i++){
        s  = &senddata[i] ;
        Serial1.write(*s);
      }
      stepCounter++;
      buttonPress = 1;
    }
    else if((buttonPress == 0) && (stepCounter == numberOfSteps)){
      Serial.println("Touch Button Pressed");
      senddata = " Recipe Finished\n";
      s  = &senddata[0] ; 
      Serial.print("Sending Data: ");
      Serial.println(senddata);
      for(int i=0 ; i<senddata.length() ; i++){
        s  = &senddata[i] ;
        Serial1.write(*s);
      }
      recipeFinishFlag = 1;
      Serial.println("Recipe Finished");
      buttonPress = 1;
    }
    delay(1000);
    timeCounter += 1;
  } ///////////////////////////////////////while step finish loop ends here
  String recordModeACK = "Recording of the Recipee was finished succesfully!";;
  PLAY_Audio("recordModeACK.mp3",6);
  return recordModeACK;
}


////////////////////////playMode//////////////////////////////////////////////////////////////////////////
String playMode(struct recipeData values){
  int numberOfSteps = values.nSteps;
  int timeCounter = 0;
  int led_limit = 7;
  int currentTimeCount = 0;
        
  for(int currentStepNo=0; currentStepNo < numberOfSteps; currentStepNo++){
    String senddata = " Step " + String(currentStepNo+1) + " started" + "\n";
    char *s  = &senddata[0] ; 
    Serial.print("Sending Data: ");
    Serial.println(senddata);
    for(int i=0 ; i<senddata.length() ; i++){
      s  = &senddata[i] ;
      Serial1.write(*s);
    }
      
    int tempSetFlag = 0;
    int perfectTempFlag = 0;
    int randFlag = 0;
    while((tempSetFlag == 0) && (perfectTempFlag == 0)){
      //Read Temperature
      temperature = getTemp(prev_temp);
      String senddata = "#" + String(temperature) + "," + String(timeCounter) + "\n";
      char *s  = &senddata[0] ; 
      Serial.print("Sending Data: ");
      Serial.println(senddata);
      for(int i=0 ; i<senddata.length() ; i++){
        s  = &senddata[i] ;
        Serial1.write(*s);
      }
      prev_temp = temperature;
      //Temperature Reading Finished
                  
      //Compare with Ideal Temperature and give audio-visual output
      if((temperature >= (values.rTemp[currentStepNo] - 5)) && (temperature <= (values.rTemp[currentStepNo] + 5)) ){
        bar.setLevel(0);
        for(int i=0;i<10;i++){
          bar.singleLed(i,0);
        }
        bar.singleLed(1,1);
        PLAY_Audio("perfect.mp3",6);
        perfectTempFlag = 1;
      }
      else if(temperature < values.rTemp[currentStepNo]){
        bar.setLevel(0);
        for(int i=0;i<10;i++){
          bar.singleLed(i,0);
        }
        bar.singleLed(0,1);
        PLAY_Audio("less.mp3",6);
        }
        else if(temperature > values.rTemp[currentStepNo]){
          bar.setLevel(0);
          for(int i=0;i<10;i++){
            bar.singleLed(i,0);
          }     
          for(int i=2;i<=led_limit;i++){
            bar.singleLed(i,1);
          }
          PLAY_Audio("more.mp3",6);
        }
        timeCounter += 2;
        
        ////////////////////////
        randFlag++;
        if(randFlag == 15){
          tempSetFlag = 1;
        }
        ////////////////////////
      }// Perfect Temeprature for the Step has now been set
      
      if(tempSetFlag == 1){
        bar.setLevel(0);
        for(int i=0;i<10;i++){
          bar.singleLed(i,0);
        }
        bar.singleLed(1,1);
        PLAY_Audio("imperfect.mp3",6);
      }
        
          
      //then do wait for the counter to finish
      while(currentTimeCount != values.rTime[currentStepNo]){
        //Read Temperature
        temperature = getTemp(prev_temp);
        String senddata = "#" + String(temperature) + "," + String(timeCounter) + "\n";
        char *s  = &senddata[0] ; 
        Serial.print("Sending Data: ");
        Serial.println(senddata);
        for(int i=0 ; i<senddata.length() ; i++){
          s  = &senddata[i] ;
          Serial1.write(*s);
        }
        prev_temp = temperature;
        //Temperature Reading Finished
        delay(1000);
        currentTimeCount += 1;
        timeCounter += 1;
      }

    }
    
    String senddata = " Recipe was cooked succesfully!\n";
    char *s  = &senddata[0] ; 
    Serial.print("Sending Data: ");
    Serial.println(senddata);
    for(int i=0 ; i<senddata.length() ; i++){
      s  = &senddata[i] ;
      Serial1.write(*s);
    }
    
    for(int i=0;i<10;i++){
      bar.singleLed(i,0);
    }
    
    String playModeACK = "Recipe was cooked succesfully!";
    PLAY_Audio("playModeACK.mp3",6);
    return playModeACK;
}


///////////////////////////////////void setup()////////////////////////////////////////////////////////////
void setup() {
  // put your setup code here, to run once:
    Serial1.begin(9600);
    Serial.begin(9600);
    pinMode(TouchButton, INPUT_PULLUP);
    attachInterrupt(1, buttonPressISR, FALLING);
    LAudio.begin();
    bar.begin(5, 4);
    bar.setLevel(0);
    for(int i=0;i<10;i++){
      bar.singleLed(i,0);
    }
    while(!Serial);
    Serial.println("Welcome to Godrej Cookind Aid Application!");
    Serial.println("       ");
}

/////////////////////////////////void loop()//////////////////////////////////////////////////////////////
void loop() {
  String Data = "";
  while (Serial1.available()){
    char character = Serial1.read(); // Receive a single character from the software serial port
    Data.concat(character); // Add the received character to the receive buffer
    
    if (character == '*'){
      Serial.println(Data);
      int colonMode = Data.indexOf(';');
      String Mode = Data.substring(0, colonMode);
      
      if (Mode == "r"){
        int numberOfSteps = parseDataRecord(Data);
        String returnedString = recordMode(numberOfSteps);
        Serial.println("  ");
        Serial.println(returnedString); 
        Serial.println("  ");
       }
       else if(Mode == "p"){
         struct recipeData values = parseDataPlay(Data);
         String returnedString = playMode(values);
         Serial.println("  ");
         Serial.println(returnedString); 
         Serial.println("  ");
       }
       
    }///////////////////if char == * loop ends here
  }///////////////////////////////////while Serial1.available loop ends here
  Data = "";
}//void loop ends here
