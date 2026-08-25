#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "HX711.h"

// ========================================================
// 1. PINES DE LA INTERFAZ (BOTONES Y PANTALLA)
// ========================================================
LiquidCrystal_I2C lcd(0x27, 16, 2); 

const int pinAceptar = 18; 
const int pinArriba = 19;  
const int pinAbajo = 17;   

int cursorMenu = 0;
const int totalOpciones = 7; 
bool actualizarPantalla = true;
volatile bool aceptar = false;
volatile unsigned long ultimoTiempoBoton = 0; 

// ========================================================
// 2. CONFIGURACIÓN DEL MOTOR NEMA
// ========================================================
const int dirPin = 9;    
const int stepPin = 8;  
const int stopPin = 3; 
const int enablePin = 10; 
const int pasoSuave = 3000;

int posicionNema = 0; 

// ========================================================
// 3. CONFIGURACIÓN DE LOS SERVOMOTORES (TAZA Y TOLVAS)
// ========================================================
Servo servoIzq;
Servo servoDer;
const int PIN_SERVO_IZQ = 4;
const int PIN_SERVO_DER = 5;

int anguloPlano = 180;   
int anguloVertido = 50;  
int anguloActual = 180;  
const int VELOCIDAD_MS = 25; 

Servo servoTolva1; 
Servo servoTolva2; 
const int PIN_TOLVA1 = 11;
const int PIN_TOLVA2 = 12;

int tolvaCerrada = 180;  
int tolvaAbierta = 140; 

// --- COMPUERTA UNICEL ---
Servo servoExtra;
const int PIN_SERVO_EXTRA = 13; 
int compuertaExtraCerrada = 0;  
int compuertaExtraAbierta = 90;

// ========================================================
// 4. CONFIGURACIÓN DE LA BÁSCULA Y RELÉS
// ========================================================
#define DT 7
#define SCK 6 
HX711 celda;

float factorEscala = 418.0; 

// --- RELÉS ---
const int pinRele1 = 22; // Bomba de Agua
const int pinRele2 = 23; // Mezcladora


// --- INTERRUPCIÓN DEL BOTÓN "ACEPTAR" ---
void isrBotonAceptar() {
  if (millis() - ultimoTiempoBoton > 500) { 
    aceptar = true;
    ultimoTiempoBoton = millis();
  }
}

// ========================================================
// ESCUDO ANTI-REBOTES (SOLUCIÓN AL SALTO DE MENÚS)
// ========================================================
void esperarLiberacionBoton() {
  // 1. Espera a que quites el dedo físicamente del botón
  while (digitalRead(pinAceptar) == LOW) { 
    delay(10); 
  }
  // 2. Espera a que el metal interno deje de vibrar
  delay(50); 
  
  // 3. ¡LA MAGIA! Reinicia el tiempo para que el ISR no detecte el rebote de soltado
  ultimoTiempoBoton = millis(); 
  aceptar = false; 
}


// --- PASO DEL NEMA ---
void darPaso(int tiempoMicrosegundos) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(tiempoMicrosegundos);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(tiempoMicrosegundos);
}

// --- FUNCIÓN DE HOMING ---
void realizarHoming() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrando...");
  lcd.setCursor(0, 1);
  lcd.print("Buscando Cero");

  digitalWrite(enablePin, LOW); 
  delay(10);

  if (digitalRead(stopPin) == LOW) {
    digitalWrite(dirPin, HIGH); 
    while (digitalRead(stopPin) == LOW) { darPaso(pasoSuave); }
    delay(500); 
  }

  digitalWrite(dirPin, LOW); 
  while (digitalRead(stopPin) == HIGH) { 
    darPaso(5000); 
  }
  
  delay(500);
  digitalWrite(dirPin, HIGH); 
  for (int i = 0; i < 20; i++) { darPaso(pasoSuave); }
  
  posicionNema = 0; 
}

// --- MOVIMIENTO DEL NEMA ---
void moverNemaAbsoluto(int posicionDestino) {
  if (posicionDestino == posicionNema) return;

  digitalWrite(enablePin, LOW); 
  delay(10); 

  int pasosAMover = posicionDestino - posicionNema;
  if (pasosAMover > 0) {
    digitalWrite(dirPin, HIGH); 
  } else {
    digitalWrite(dirPin, LOW);  
    pasosAMover = -pasosAMover; 
  }

  for (int i = 0; i < pasosAMover; i++) { darPaso(pasoSuave); }
  posicionNema = posicionDestino; 
}

// --- MOVIMIENTO DE LA TAZA ---
void moverSuaveYDesconectar(int anguloDestino) {
  servoIzq.attach(PIN_SERVO_IZQ, 500, 2500);
  servoDer.attach(PIN_SERVO_DER, 500, 2500);

  int paso = (anguloDestino >= anguloActual) ? 1 : -1;
  for (int a = anguloActual; a != anguloDestino; a += paso) {
    servoIzq.write(a);
    servoDer.write(180 - a);
    delay(VELOCIDAD_MS);
  }
  servoIzq.write(anguloDestino);
  servoDer.write(180 - anguloDestino);
  delay(100);
  servoIzq.detach();
  servoDer.detach();
  anguloActual = anguloDestino;
}

// --- CONTROL DE RECIPIENTES ---
void controlarTolva(int numeroRecipiente, bool abrir) {
  int anguloDestino = abrir ? tolvaAbierta : tolvaCerrada;
  
  if (numeroRecipiente == 1) {
    servoTolva1.attach(PIN_TOLVA1);
    servoTolva1.write(anguloDestino);
    delay(400); 
    servoTolva1.detach(); 
  } 
  else if (numeroRecipiente == 2) {
    servoTolva2.attach(PIN_TOLVA2);
    servoTolva2.write(anguloDestino);
    delay(400); 
    servoTolva2.detach(); 
  }
}

// ========================================================
// MÓDULO DE AUTOMATIZACIÓN Y DOSIFICACIÓN
// ========================================================

void vaciarMaterial() {
  lcd.setCursor(0, 1);
  lcd.print("Vaciando...     "); 
  
  moverSuaveYDesconectar(anguloVertido); 
  delay(1500); 

  servoIzq.attach(PIN_SERVO_IZQ, 500, 2500);
  servoDer.attach(PIN_SERVO_DER, 500, 2500);
  int anguloGolpe = anguloVertido + 20; 

  for (int sacudida = 0; sacudida < 4; sacudida++) {
    servoIzq.write(anguloGolpe);
    servoDer.write(180 - anguloGolpe);
    delay(150); 
    servoIzq.write(anguloVertido);
    servoDer.write(180 - anguloVertido);
    delay(150); 
  }
  servoIzq.detach();
  servoDer.detach();
  
  delay(1500); 
  moverSuaveYDesconectar(anguloPlano); 
}

float esperarLlenado(float pesoObjetivo) {
  float pesoActual = 0.0;

  while (pesoActual < pesoObjetivo) {
    pesoActual = celda.get_units(3); 
    
    lcd.setCursor(0, 1);
    lcd.print("Carga: ");
    lcd.print(pesoActual, 1);
    lcd.print(" g   "); 
    
    delay(10); 
  }
  return pesoActual; 
}

void dosificarPorLotes(int estacionNema, int numeroTolva, float pesoTotalMeta, float capacidadTaza) {
  float pesoAcumulado = 0.0;

  while (pesoAcumulado < pesoTotalMeta) {
    float pesoFaltante = pesoTotalMeta - pesoAcumulado;
    float pesoEsteViaje = (pesoFaltante > capacidadTaza) ? capacidadTaza : pesoFaltante;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mat "); lcd.print(numeroTolva); 
    lcd.print(" "); lcd.print((int)pesoAcumulado); lcd.print("/"); lcd.print((int)pesoTotalMeta); lcd.print("g");

    moverNemaAbsoluto(estacionNema);
    
    lcd.setCursor(0, 1);
    lcd.print("Asentando peso..");
    delay(1500); 
    celda.tare();
    controlarTolva(numeroTolva, true);   
    float servido = esperarLlenado(pesoEsteViaje);      
    controlarTolva(numeroTolva, false);  
    
    pesoAcumulado += servido; 

    realizarHoming(); 
    vaciarMaterial(); 
  }
}

void ejecutarRutinaDosificacion(int multiplicador) {
  digitalWrite(enablePin, LOW); 

  float recetaMaterial1 = 1500.0 * multiplicador;
  float recetaMaterial2 = 1000.0 * multiplicador;

  // ========================================================
  // ¡CORRECCIÓN AQUÍ! Regresamos a las estaciones 50 y 125
  // ========================================================
  dosificarPorLotes(50, 1, recetaMaterial1, 200); 
  dosificarPorLotes(125, 2, recetaMaterial2, 150); 

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CICLO COMPLETO");
  lcd.setCursor(0, 1);
  lcd.print("Estabilizando..");
  
  delay(1500); 
  digitalWrite(enablePin, HIGH); 
  
  delay(2000); 
  actualizarPantalla = true; 
  aceptar = false; 
}

// ========================================================
// SISTEMA DE CALIBRACIÓN EN VIVO
// ========================================================
void calibrarBascula() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vacia la taza..");
  delay(2000);
  
  celda.tare(); 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pon peso exacto");
  lcd.setCursor(0, 1);
  lcd.print("(Ej. una moneda)");
  delay(3000);

  lcd.clear();
  esperarLiberacionBoton(); // Bloqueo anti-rebote

  while (!aceptar) {
    celda.set_scale(factorEscala);
    float pesoActual = celda.get_units(2);

    lcd.setCursor(0, 0);
    lcd.print("Factor: ");
    lcd.print(factorEscala, 1);
    lcd.print("  "); 
    
    lcd.setCursor(0, 1);
    lcd.print("Peso: ");
    lcd.print(pesoActual, 1);
    lcd.print(" g   ");

    if (digitalRead(pinArriba) == LOW) {
      factorEscala += 1.0; 
      delay(50); 
    }
    if (digitalRead(pinAbajo) == LOW) {
      factorEscala -= 1.0; 
      delay(50);
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Guardado OK");
  delay(1500);
  
  esperarLiberacionBoton();
  actualizarPantalla = true;
}

// ========================================================
// VISUALIZACIÓN DEL MENÚ
// ========================================================
void mostrarMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MENU PRINCIPAL");
  
  lcd.setCursor(0, 1);
  switch (cursorMenu) {
    case 0: lcd.print("> Agregar Unicel"); break; 
    case 1: lcd.print("> Vertir Material"); break; 
    case 2: lcd.print("> Agregar Agua"); break; 
    case 3: lcd.print("> Mezclar"); break; 
    case 4: lcd.print("> Calibrar Brazo"); break; 
    case 5: lcd.print("> Calibrar Peso"); break; 
    case 6: lcd.print("> Tarar Bascula"); break; 
  }
}

// ========================================================
// SETUP Y LOOP
// ========================================================

void setup() {
  Serial.begin(9600);
  
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, LOW); 

  pinMode(pinArriba, INPUT_PULLUP);
  pinMode(pinAbajo, INPUT_PULLUP);
  pinMode(pinAceptar, INPUT_PULLUP);

  pinMode(pinRele1, OUTPUT);
  digitalWrite(pinRele1, LOW); 
  
  pinMode(pinRele2, OUTPUT);
  digitalWrite(pinRele2, LOW); 
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando Sist.");

  celda.begin(DT, SCK);
  celda.set_scale(factorEscala); 
  celda.tare();

  servoIzq.write(anguloPlano);
  servoDer.write(180 - anguloPlano);
  servoIzq.attach(PIN_SERVO_IZQ, 500, 2500);
  servoDer.attach(PIN_SERVO_DER, 500, 2500);
  delay(500);
  servoIzq.detach();
  servoDer.detach();

  servoTolva1.write(tolvaCerrada);
  servoTolva2.write(tolvaCerrada);
  servoTolva1.attach(PIN_TOLVA1);
  servoTolva2.attach(PIN_TOLVA2);
  delay(500);
  servoTolva1.detach();
  servoTolva2.detach();

  servoExtra.write(compuertaExtraCerrada);
  servoExtra.attach(PIN_SERVO_EXTRA);
  delay(500);
  servoExtra.detach();

  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(stopPin, INPUT_PULLUP);
  
  realizarHoming(); 
  actualizarPantalla = true; 

  delay(1500); 
  digitalWrite(enablePin, HIGH); 

  attachInterrupt(digitalPinToInterrupt(pinAceptar), isrBotonAceptar, FALLING);
}

void loop() {
  
  if (actualizarPantalla) {
    mostrarMenu();
    actualizarPantalla = false;
  }

  if (digitalRead(pinArriba) == LOW) {
    if (cursorMenu > 0) { 
      cursorMenu--; 
      actualizarPantalla = true; 
    }
    delay(200); 
  }

  if (digitalRead(pinAbajo) == LOW) {
    if (cursorMenu < (totalOpciones - 1)) { 
      cursorMenu++; 
      actualizarPantalla = true; 
    }
    delay(200);
  }

  if (aceptar) {
    aceptar = false; 

    switch (cursorMenu) {
      
      // =========================================
      // OPCIÓN 1: AGREGAR UNICEL + HOMING AL FINAL
      // =========================================
      case 0:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Moviendo a 125..");
        
        moverNemaAbsoluto(125); 
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Unicel ABIERTO");
        lcd.setCursor(0, 1);
        lcd.print("Pulsa para parar");

        servoExtra.attach(PIN_SERVO_EXTRA);
        servoExtra.write(compuertaExtraAbierta);
        
        esperarLiberacionBoton(); // Bloqueo anti-rebote
        
        while(!aceptar) { 
          delay(10); 
        }
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cerrando Unicel.");
        servoExtra.write(compuertaExtraCerrada);
        delay(1000);
        servoExtra.detach();

        realizarHoming();
        
        delay(1000);
        digitalWrite(enablePin, HIGH); 
        esperarLiberacionBoton();
        actualizarPantalla = true;
        break;

      // =========================================
      // OPCIÓN 2: VERTIR MATERIAL (CON PORCIONES)
      // =========================================
      case 1:
        { 
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Porciones:");
          
          esperarLiberacionBoton(); // ¡LA MAGIA AQUÍ! Te permite soltar el botón en paz.
          
          int porcionesElegidas = 1;
          const int maxPorciones = 3; 
          
          while(!aceptar) { 
            lcd.setCursor(0, 1);
            lcd.print("-> ");
            lcd.print(porcionesElegidas);
            lcd.print("             "); 
            
            if (digitalRead(pinArriba) == LOW) {
              if (porcionesElegidas < maxPorciones) {
                porcionesElegidas++;
              }
              delay(200); 
            }
            
            if (digitalRead(pinAbajo) == LOW) {
              if (porcionesElegidas > 1) {
                porcionesElegidas--;
              }
              delay(200);
            }
          }
          
          esperarLiberacionBoton(); // Limpiamos nuevamente antes de arrancar
          ejecutarRutinaDosificacion(porcionesElegidas);
        }
        break;

      // =========================================
      // OPCIÓN 3: AGREGAR AGUA (RELÉ 1)
      // =========================================
      case 2:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Control de Agua");
        lcd.setCursor(0, 1);
        lcd.print("Pulsa para ON");
        
        esperarLiberacionBoton(); 
        
        while(!aceptar) { 
          delay(10); 
        }
        
        digitalWrite(pinRele1, HIGH); 
        lcd.setCursor(0, 1);
        lcd.print("Agua: ENCENDIDA ");
        
        esperarLiberacionBoton(); 
        
        while(!aceptar) { 
          delay(10); 
        }
        
        digitalWrite(pinRele1, LOW); 
        lcd.setCursor(0, 1);
        lcd.print("Agua: APAGADA   ");
        
        delay(1500); 
        esperarLiberacionBoton(); 
        actualizarPantalla = true; 
        break;

      // =========================================
      // OPCIÓN 4: MEZCLAR (RELÉ 2)
      // =========================================
      case 3:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Control Mezcla");
        lcd.setCursor(0, 1);
        lcd.print("Pulsa para ON");
        
        esperarLiberacionBoton(); 
        
        while(!aceptar) { 
          delay(10); 
        }
        
        digitalWrite(pinRele2, HIGH); 
        lcd.setCursor(0, 1);
        lcd.print("Mezcla: ENCENDIDA");
        
        esperarLiberacionBoton(); 
        
        while(!aceptar) { 
          delay(10); 
        }
        
        digitalWrite(pinRele2, LOW); 
        lcd.setCursor(0, 1);
        lcd.print("Mezcla: APAGADA  ");
        
        delay(1500); 
        esperarLiberacionBoton(); 
        actualizarPantalla = true; 
        break;

      // =========================================
      // OPCIÓN 5: CALIBRAR BRAZO (HOMING)
      // =========================================
      case 4:
        realizarHoming();
        delay(1500);
        digitalWrite(enablePin, HIGH); 
        actualizarPantalla = true;
        break;

      // =========================================
      // OPCIÓN 6: CALIBRAR PESO (FACTOR BÁSCULA)
      // =========================================
      case 5: 
        calibrarBascula();
        break;

      // =========================================
      // OPCIÓN 7: TARAR BÁSCULA (CERO)
      // =========================================
      case 6:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Tarando bascula");
        celda.tare();
        delay(1000);
        actualizarPantalla = true;
        break;
    }
  }
}