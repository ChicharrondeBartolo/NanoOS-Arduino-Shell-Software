#include <EEPROM.h>

// --- ESTRUCTURAS Y VARIABLES GLOBALES ---
struct Tarea {
  void (*funcion)();
  unsigned long intervalo;
  unsigned long ultimaEjecucion;
  bool activa;
};

const int MAX_TAREAS = 4;
Tarea listaDeTareas[MAX_TAREAS];

bool isRoot = false; // Estado de seguridad
int eepromAddr = 0;  // Dirección para "almacenamiento"

// --- TAREAS DEL SISTEMA ---

void tareaBlink() {
  static bool estado = false;
  estado = !estado;
  digitalWrite(13, estado);
}

// --- COMANDOS DE LA CONSOLA ---

void cmd_login(String pass) {
  if (pass == "1234") {
    isRoot = true;
    Serial.println("Acceso concedido. Bienvenido, ADMIN.");
  } else {
    Serial.println("Contrasena incorrecta.");
  }
}

void cmd_save(String dato) {
  if (!isRoot) {
    Serial.println("ERROR: Se requiere privilegios de ROOT.");
    return;
  }
  EEPROM.update(eepromAddr, dato.toInt());
  Serial.println("Dato guardado en /mnt/eeprom");
}

void cmd_read() {
  int valor = EEPROM.read(eepromAddr);
  Serial.print("Valor en disco: ");
  Serial.println(valor);
}

void cmd_ps() {
  Serial.println("PID\tTASK\t\tSTATUS");
  Serial.println("0\tBlink\t\t" + String(listaDeTareas[0].activa ? "RUNNING" : "STOPPED"));
  Serial.println("1\tShell\t\tRUNNING");
}

// --- INTERPRETE DE COMANDOS AVANZADO ---

void procesarComando(String input) {
  input.trim();
  int espacio = input.indexOf(' ');
  String cmd = (espacio == -1) ? input : input.substring(0, espacio);
  String arg = (espacio == -1) ? "" : input.substring(espacio + 1);

  if (cmd == "help") {
    Serial.println("Comandos: login [pass], save [num], read, ps, logout");
  } 
  else if (cmd == "login") {
    cmd_login(arg);
  }
  else if (cmd == "save") {
    cmd_save(arg);
  }
  else if (cmd == "read") {
    cmd_read();
  }
  else if (cmd == "ps") {
    cmd_ps();
  }
  else if (cmd == "logout") {
    isRoot = false;
    Serial.println("Sesion cerrada.");
  }
  else {
    Serial.println("Comando no reconocido.");
  }
  
  Serial.print(isRoot ? "root@nano:# " : "user@nano:$ ");
}

// --- KERNEL SETUP & LOOP ---

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  // Registrar tarea 0: Blink
  listaDeTareas[0] = {tareaBlink, 500, 0, true};

  Serial.println("--- NanoOS v2.0 Loaded ---");
  Serial.print("user@nano:$ ");
}

void loop() {
  // Planificador (Scheduler)
  unsigned long t = millis();
  for (int i = 0; i < MAX_TAREAS; i++) {
    if (listaDeTareas[i].activa && (t - listaDeTareas[i].ultimaEjecucion >= listaDeTareas[i].intervalo)) {
      listaDeTareas[i].ultimaEjecucion = t;
      listaDeTareas[i].funcion();
    }
  }

  // Shell
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    procesarComando(comando);
  }
}