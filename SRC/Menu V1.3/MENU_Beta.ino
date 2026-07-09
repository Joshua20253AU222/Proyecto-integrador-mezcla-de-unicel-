//#include <Adafruit_LiquidCrystal.h>
//#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Adafruit_LiquidCrystal lcd_1(0);
LiquidCrystal_I2C lcd_1(0x27,20,4);

int arriba = 0;
int abajo = 0;
int enter = 0;
int x = 0;
bool error = false;

const int stepPin = 0;
const int dirPin  = 1;

const int boton1 = 12;
const int boton2 = 13;


int tiempo[] =     {5000,4000,5000,6000};

String menuSel[] = {"->LED 1","->Motor","->Opcion 3","->Opcion 4"};
String menuDes[] = {"Motor","Opcion 3","Opcion 4",""};
int pines[] =      {2,3,1};

int cursor = 0;
 
 const int velocidad=800;
void motor(){

  lcd_1.clear();
  lcd_1.print("Controlando");
  lcd_1.setCursor(0,1);
  lcd_1.print("motor");
 delay(1000);

    digitalWrite(dirPin, HIGH);   
    digitalWrite(6, HIGH);   
   delay(1000);

 while(digitalRead(11)==HIGH){
  if(digitalRead(boton1)==LOW){    
    digitalWrite(dirPin, HIGH);

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(velocidad);       

    digitalWrite(stepPin, LOW);
    delayMicroseconds(velocidad);
    }

  if(digitalRead(boton2)==LOW){    

    digitalWrite(dirPin, LOW);

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(velocidad);       

    digitalWrite(stepPin, LOW);
    delayMicroseconds(velocidad);

  }
 }

        lcd_1.clear();
      lcd_1.setCursor(0,0);
      lcd_1.print(menuSel[cursor]);
      lcd_1.setCursor(0,1);
      lcd_1.print(menuDes[cursor]);
   delay(1500);

}

void setup()
{
  
  pinMode(13, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  
  
  //lcd_1.begin(16,2);
  lcd_1.init();
  lcd_1.backlight();

   lcd_1.print("Encendido...");
  lcd_1.setCursor(0,1);
  lcd_1.print("Beta (V1.3)");
  lcd_1.setCursor(0,0);
  delay(2000);
  
  lcd_1.clear();
  
  lcd_1.print( menuSel[0]);
  lcd_1.setCursor(0,1);
  lcd_1.print( menuDes[0]);
  
}

void loop()
{
  arriba = digitalRead(13);
  abajo = digitalRead(12);
  enter = digitalRead(11);
  
  
  
  if (arriba == LOW) {
    digitalWrite(10, HIGH);
    //lcd_1.print("ARRIBA"); //POR SI ACASO
    
    if (cursor > 0){
    	  cursor -=1;
      lcd_1.clear();
      lcd_1.setCursor(0,0);
      lcd_1.print(menuSel[cursor]);
      lcd_1.setCursor(0,1);
      lcd_1.print(menuDes[cursor]);
      delay(1000);
    }else{
      error = true;
    }
    
    
    
  } 
  else if (abajo == LOW) {
    digitalWrite(9, HIGH);
    //lcd_1.print("ABAJO"); //POR SI ACASO

        if (cursor < 3){
    	  cursor +=1;
          lcd_1.clear();
          lcd_1.setCursor(0,0);
          lcd_1.print(menuSel[cursor]);
          lcd_1.setCursor(0,1);
          lcd_1.print(menuDes[cursor]);
          delay(1000); 
    }else{
          error = true;
    }

  } 
  else if (enter == LOW) {
    digitalWrite(8, HIGH);
    //lcd_1.print("ENTER"); //POR SI ACASO
    lcd_1.clear();
      
    for(x=1; x<=3; x++){
        lcd_1.clear();
      	lcd_1.print(menuSel[cursor]);
        
        delay(200);
        
    }

    if(cursor==1){
      motor();
    }else{
        delay(2000);
      digitalWrite(pines[cursor],HIGH);
      for(x=(tiempo[cursor]/1000); x>=0; x--){
          lcd_1.clear();
          lcd_1.print(menuSel[cursor]);
          lcd_1.print (" (");
          lcd_1.print (x);
          lcd_1.print (")");
        digitalWrite(6,HIGH);
          delay(200);
          digitalWrite(6,LOW);
          delay(800);

      }
      
      digitalWrite(pines[cursor],LOW);
      lcd_1.clear();
      lcd_1.setCursor(0,0);
      lcd_1.print(menuSel[cursor]);
      lcd_1.setCursor(0,1);
      lcd_1.print(menuDes[cursor]);
      delay(1000);
    }
    


  }
  
  
  if(error == true){
    error = false;
    for(x=1; x<=3; x++){
        lcd_1.clear();
      	lcd_1.print("FIN DE LISTA");
        digitalWrite(7,HIGH);
        delay(200);
      	digitalWrite(7,LOW);
        
    }
   	lcd_1.clear();
    lcd_1.setCursor(0,0);
    lcd_1.print( menuSel[cursor]);
    lcd_1.setCursor(0,1);
    lcd_1.print( menuDes[cursor]);
    
  }

  digitalWrite(10, LOW);
  digitalWrite(9, LOW);
  digitalWrite(8, LOW);
}
