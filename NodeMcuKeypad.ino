
#include <EMailSender.h>
#include <ESP8266WiFi.h>
#include <Keypad.h>
#include <EEPROM.h>

#include "Gsender.h"

#pragma region Globals
const char* ssid = "YOUR_WIFI_SSID";                           // WIFI network name
const char* password = "YOUR_WIFI_PASSWORD";                       // WIFI network password
uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals

uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
    static uint16_t attempt = 0;
    Serial.print("Connecting to ");
    if(nSSID) {
        WiFi.begin(nSSID, nPassword);  
        Serial.println(nSSID);
    } else {
        WiFi.begin(ssid, password);
        Serial.println(ssid);
    }

    uint8_t i = 0;
    while(WiFi.status()!= WL_CONNECTED && i++ < 50)
    {
        delay(200);
        Serial.print(".");
    }
    ++attempt;
    Serial.println("");
    if(i == 51) {
        Serial.print("Connection: TIMEOUT on attempt: ");
        Serial.println(attempt);
        if(attempt % 2 == 0)
            Serial.println("Check if access point available or SSID and Password\r\n");
        return false;
    }
    Serial.println("Connection: ESTABLISHED");
    Serial.print("Got IP address: ");
    Serial.println(WiFi.localIP());
    return true;
}

void Awaits()
{
    uint32_t ts = millis();
    while(!connection_state)
    {
        delay(50);
        if(millis() > (ts + reconnect_interval) && !connection_state){
            connection_state = WiFiConnect();
            ts = millis();
        }
    }
}


//Keymap for 4x4 Keypad
const byte numRows= 4;          //number of rows on the keypad
const byte numCols= 4;          //number of columns on the keypad

char keymap[numRows][numCols]= 
{
{'1', '2', '3', 'A'}, 
{'4', '5', '6', 'B'}, 
{'7', '8', '9', 'C'},
{'*', '0', '#', 'D'}
};

char keypressed;                 //Where the keys are stored it changes very often
char code[]= {'7','8','9','9'};  //The default code, you can change it or make it a 'n' digits one

char code_buff1[sizeof(code)];  //Where the new key is stored - sizeof function gives the size of the character array
char code_buff2[sizeof(code)];  //Where the new key is stored again so it's compared to the previous one

int a=0,i=0,s=0,j=0;          //Variables used later

byte rowPins[numRows] = {16,5,4,0}; //Rows 0 to 3 //if you modify your pins you should modify this too
byte colPins[numCols]= {2,14,12,13}; //Columns 0 to 3

int access=0;

Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);


void setup()
         {
           Serial.begin(115200);
    connection_state = WiFiConnect();
    if(!connection_state)  // if not connected to WIFI
        Awaits();          // constantly trying to connect

         Serial.println("Standby");     
         yield();
         }




void loop()
{

  keypressed = myKeypad.getKey();   
  yield();//Constantly waiting for a key to be pressed
    if(keypressed == '*'){                      // * to type the code
            
            Serial.print("Enter code: ");            //Message to show
            GetCode();   //Getting code function   
            yield();
                  if(a==sizeof(code))        {   //The GetCode function assign a value to a (it's correct when it has the size of the code array)
                  OpenDoor();   access=0;
                  
                  yield();//Open lock function if code is correct
                  }
                  else{
                      access=access+1;  
                     Serial.println("Access Denied"); 
                     
                  if(access==3){
                    SendMail("Unauthorized access of Secure facility","Someone tried to access facility. \n Code was entered incorrectly three times. \n Secure facility immediately.");//Message to print when the code is wrong
                  }
                  }
                  yield();
            delay(2000);
            Serial.println("Standby"); //Return to standby mode it's the message do display when waiting
            yield();
        }

     if(keypressed == '#'){                  //To change the code it calls the changecode function
      ChangeCode();
      Serial.println("Standby");
      yield();//When done it returns to standby mode
     }
       yield();  
}

void GetCode(){                  //Getting code sequence
       i=0;                      //All variables set to 0
       a=0;
                             
     while(keypressed != 'A'){                                     //The user press A to confirm the code otherwise he can keep typing
           keypressed = myKeypad.getKey();                         
             if(keypressed != NO_KEY && keypressed != 'A' ){       //If the char typed isn't A and neither "nothing"
                                              
              Serial.print("*");
              
            if(keypressed == code[i]&& i<sizeof(code)){            //if the char typed is correct a and i increments to verify the next caracter
                 a++;                                              
                 i++;
                 }
            else
                a--;                                               //if the character typed is wrong a decrements and cannot equal the size of code []
            }
            yield();
            }
            Serial.println();
    keypressed = NO_KEY;
    yield();
}

void ChangeCode(){                      //Change code sequence
      Serial.println("Changing code");
      delay(1000);
      
      Serial.print("Enter old code:");
      GetCode();                      //verify the old code first so you can change it
      
            if(a==sizeof(code)){      //again verifying the a value
            
            Serial.println("Changing code");
            GetNewCode1();            //Get the new code
            yield();
            GetNewCode2();         
            yield();//Get the new code again to confirm it
            s=0;
              for(i=0 ; i<sizeof(code) ; i++){     //Compare codes in array 1 and array 2 from two previous functions
              if(code_buff1[i]==code_buff2[i])
              s++;                                //again this how we verifiy, increment s whenever codes are matching
              }
                  if(s==sizeof(code)){            //Correct is always the size of the array
                  
                   for(i=0 ; i<sizeof(code) ; i++){
                  code[i]=code_buff2[i];         //the code array now receives the new code
                  EEPROM.put(i, code[i]);        //And stores it in the EEPROM
                  
                  }
                  Serial.println("Code Changed");
                  SendMail("Code Changed Successfully","The passcode for the Secure facility was changed successfully.");
                  delay(2000);
                  yield();
                  }
                  else{               
                  //In case the new codes aren't matching
                  Serial.println("Codes are not matching");
                  delay(2000);
                  yield();
                  }
            
          }
          
          else{                     //In case the old code is wrong you can't change it
          SendMail("Unauthorized access of Secure facility" ,"Level 1 breach. Code change attempt denied.");
          Serial.println("Wrong");
          delay(2000);
          }
          yield();
}

void GetNewCode1(){                      
  i=0;
  j=0;
  
  Serial.println("Enter new code and press A");   //tell the user to enter the new code and press A
  delay(2000);
 
             
         while(keypressed != 'A'){            //A to confirm and quits the loop
             keypressed = myKeypad.getKey();
               if(keypressed != NO_KEY && keypressed != 'A' ){
                Serial.print("*"); 
                code_buff1[i]=keypressed;     //Store caracters in the array
                i++;
                j++;                    
                }
                yield();
                }
                Serial.println();
keypressed = NO_KEY;
}

void GetNewCode2(){                         //This is exactly like the GetNewCode1 function but this time the code is stored in another array
  i=0;
  j=0;
  
  Serial.println("Confirm code and press A");
 
         while(keypressed != 'A'){
             keypressed = myKeypad.getKey();
               if(keypressed != NO_KEY && keypressed != 'A' ){
                
                Serial.print("*");
                code_buff2[i]=keypressed;
                i++;
                j++;                    
                }
                yield();
                }
                Serial.println();
keypressed = NO_KEY;
yield();
}

void OpenDoor(){             
  Serial.println("Access Granted");
 
}
void SendMail(String subject,String message)
{
    

    Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
    if(gsender->Subject(subject)->Send("RECIPIENT_EMAIL_ID", message)) {
        Serial.println("Message send.");
    } 
    else {
        Serial.print("Error sending message: ");
        Serial.println(gsender->getError());
    }
}

