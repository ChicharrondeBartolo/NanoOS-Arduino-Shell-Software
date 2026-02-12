/*
  NanoOS v1.0 - Un sistema operativo básico para Arduino Uno
  Características: Multitarea cooperativa y Consola Serial (Shell)
*/

// --- 1. ESTRUCTURAS DEL KERNEL ---

// Definimos qué es una "Tarea" (Proceso)
struct Tarea {
  void (*funcion)();          // Puntero a la función que hace el trabajo
  unsigned long intervalo;    // Cada cuántos milisegundos se ejecuta
  unsigned long ultimaEjecucion; // Cuándo se ejecutó por última vez
  bool activa;                // Si la tarea está corriendo o pausada
};

// --- 2. CONFIGURACIÓN DEL SISTEMA ---

// Definimos el máximo de tareas para no saturar la RAM
const int MAX_TAREAS = 5;
Tarea listaDeTareas[MAX_TAREAS];

// Pin para demostración de hardware
const int LED_PIN = 13;
bool ledState = false;

// --- 3. DEFINICIÓN DE PROCESOS (Tus "Apps") ---

// Proceso 1: Parpadeo de LED (Sistema visual)
void procesoLed() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  // No imprimimos aquí para no saturar la consola
}

// Proceso 2: Monitor del Sistema (Reporte de estado)
void procesoMonitor() {
  Serial.print("[SISTEMA] Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" segundos. Todo nominal.");
}

// Proceso 3: Simulación de lectura de sensor
void procesoSensor() {
  int simulacion = random(0, 100);
  // Solo avisamos si hay un valor "crítico" para simular una alerta
  if (simulacion > 90) {
    Serial.print("[ALERTA] Sensor detectó pico alto: ");
    Serial.println(simulacion);
  }
}

// --- 4. FUNCIONES DEL KERNEL (API) ---

void registrarTarea(int id, void (*func)(), long ms) {
  if (id >= 0 && id < MAX_TAREAS) {
    listaDeTareas[id].funcion = func;
    listaDeTareas[id].intervalo = ms;
    listaDeTareas[id].ultimaEjecucion = 0;
    listaDeTareas[id].activa = true;
  }
}

void ejecutarPlanificador() {
  unsigned long tiempoActual = millis();

  for (int i = 0; i < MAX_TAREAS; i++) {
    if (listaDeTareas[i].activa) {
      if (tiempoActual - listaDeTareas[i].ultimaEjecucion >= listaDeTareas[i].intervalo) {
        // Guardamos el tiempo antes de ejecutar para mantener el ritmo
        listaDeTareas[i].ultimaEjecucion = tiempoActual;
        
        // Ejecutamos la tarea (Context Switch simulado)
        listaDeTareas[i].funcion();
      }
    }
  }
}

// --- 5. LA SHELL (Intérprete de Comandos) ---

void procesarComando(String comando) {
  comando.trim(); // Quitar espacios extra
  
  if (comando == "help") {
    Serial.println("--- Comandos NanoOS ---");
    Serial.println(" help    : Muestra esta lista");
    Serial.println(" status  : Estado de la memoria y tareas");
    Serial.println(" led on  : Fuerza encendido del LED");
    Serial.println(" led off : Fuerza apagado del LED");
    Serial.println(" reboot  : Reinicia el sistema (software)");
  } 
  else if (comando == "status") {
    Serial.print("Tareas activas: ");
    Serial.println(MAX_TAREAS);
    Serial.println("Sistema corriendo correctamente.");
  }
  else if (comando == "led on") {
    // Pausamos el proceso automático para tomar control manual
    listaDeTareas[0].activa = false; 
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Control manual: LED Encendido.");
  }
  else if (comando == "led off") {
    listaDeTareas[0].activa = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("Control manual: LED Apagado.");
  }
  else if (comando == "reboot") {
    Serial.println("Reiniciando...");
    delay(500);
    asm volatile ("  jmp 0"); // Salta a la dirección 0 (Reset)
  }
  else {
    Serial.println("Comando desconocido: " + comando);
  }
  
  // Volvemos a mostrar el prompt
  Serial.print("user@nano-os:~$ ");
}

// --- 6. ARRANQUE Y BUCLE PRINCIPAL ---

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  
  // Boot sequence
  Serial.println("\nIniciando NanoOS v1.0...");
  Serial.println("Cargando kernel...");
  
  // Registrar las tareas en el planificador
  // ID 0: LED parpadea cada 1000ms (1 seg)
  registrarTarea(0, procesoLed, 1000);
  
  // ID 1: Monitor imprime estado cada 5000ms (5 seg)
  registrarTarea(1, procesoMonitor, 5000);
  
  // ID 2: Sensor lee cada 2000ms (2 seg)
  registrarTarea(2, procesoSensor, 2000);

  Serial.println("Sistema listo. Escribe 'help' para ayuda.");
  Serial.print("user@nano-os:~$ ");
}

void loop() {
  // 1. Ejecutar tareas en segundo plano
  ejecutarPlanificador();

  // 2. Escuchar input del usuario (Interrupción simulada)
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    procesarComando(input);
  }
}