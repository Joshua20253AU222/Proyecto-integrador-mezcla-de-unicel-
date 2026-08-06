#include "HX711.h"
#include <Servo.h>


#define DT 7
#define SCK 6//PWM

HX711 celda;
Servo servoMotor;

bool estado = true;

void setup() {

Serial.begin(9600);
Serial.println("Balanza con celda de carga");
celda.begin(DT,SCK);



celda.set_scale(419);
celda.tare();

}

void actionServo(bool cambio) {

  Serial.print("PONER OBJETO A PESAR... 5 seg");
  delay(5000);
  Serial.print("Valor (gramos): ");
  //dividir el valor entreado por el valor real del objeto
  Serial.println(celda.get_units(10));
}
