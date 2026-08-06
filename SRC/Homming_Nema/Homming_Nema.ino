#include <avr/interrupt.h>

const int dirPin = 9;    
const int stepPin = 8;  
const int stopPin = 3; // Limit Switch / Botón en INT1 (Pin 3)

// Flag volatile para detectar cuando la estructura física toca el botón
volatile bool origenEncontrado = false;

// Función dedicada a dar 1 solo paso al motor
void darPaso(int tiempoMicrosegundos) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(tiempoMicrosegundos);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(tiempoMicrosegundos);
}

void setup() {
  Serial.begin(9600);

  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(stopPin, INPUT_PULLUP); // Pin 3 en HIGH por defecto, pasa a LOW al topar

  // --- CONFIGURACIÓN DE REGISTROS INT1 (Pin 3) ---
  EICRA = 0b00001000;   // Detecta flanco de bajada (FALLING)
  EIFR  |= (1 << INTF1); // Limpia interrupciones fantasmas pendientes
  EIMSK = 0b00000010;   // Habilita interrupción INT1 (Pin 3)

  sei(); // Habilitar interrupciones globales

  Serial.println("Iniciando busqueda de cero (Homing)...");

  // Definimos dirección de búsqueda hacia el botón
  digitalWrite(dirPin, HIGH); 

  // --- FASE 1: BÚSQUEDA A CIEGAS HASTA TOPAR ---
  // El motor gira paso a paso hasta que la interrupción ponga 'origenEncontrado' en true
  while (!origenEncontrado) {
    darPaso(1500); // Gira buscando el tope físico
  }

  // Deshabilitamos la interrupción INT1 para que el botón no interfiera después de calibrar
  EIMSK &= ~(1 << INT1);

  Serial.println("¡Origen (Cero) detectado!");
  Serial.println("Iniciando secuencias en el loop...");
  delay(1000); // Pausa de 1 segundo antes de iniciar el loop
}

void loop() {
  // --- FASE 2: SECUENCIA DE 300 PASOS (1.5 VUELTAS) ---
  
  // 1. Giro a la derecha (300 pasos)
  digitalWrite(dirPin, HIGH);
  Serial.println("Moviendo 300 pasos a la derecha...");
  for (int i = 0; i < 300; i++) {
    darPaso(1500);
  }

  delay(2000); // Pausa de 2 segundos en posición alcanzada

  // 2. Giro a la izquierda de regreso (300 pasos)
  digitalWrite(dirPin, LOW);
  Serial.println("Regresando 300 pasos...");
  for (int i = 0; i < 300; i++) {
    darPaso(1500);
  }

  delay(2000); // Pausa antes de repetir
}

// ISR para la interrupción INT1 (Pin 3)
ISR(INT1_vect) {
  origenEncontrado = true; // Notifica que el botón fue tocado físicamente
}