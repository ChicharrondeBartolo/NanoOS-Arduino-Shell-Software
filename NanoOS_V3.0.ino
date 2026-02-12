/*
  NanoOS v3.0 - Advanced Shell & Process Control
  Arquitectura: Kernel Monolítico (Simulado)
*/

// --- 1. KERNEL & GESTIÓN DE MEMORIA ---

// Función mágica para calcular memoria RAM libre
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

struct Process {
  String name;              // Nombre del proceso
  void (*function)();       // Puntero a la función
  unsigned long interval;   // Intervalo de ejecución (ms)
  unsigned long lastRun;    // Última ejecución
  bool active;              // Estado (Running/Stopped)
};

const int MAX_PIDS = 3;
Process processTable[MAX_PIDS];

// --- 2. PROCESOS DEL SISTEMA (USER SPACE) ---

// PID 0: System Heartbeat (Led interno)
void proc_blink() {
  static bool s = false;
  s = !s;
  digitalWrite(13, s);
}

// PID 1: Logger (Simula un servicio de registro)
void proc_logger() {
  // Solo imprime un punto para no ensuciar, a menos que activemos verbose
  // Serial.print(".");
}

// PID 2: Sensor Watchdog (Simula lectura de datos)
void proc_sensor() {
  int val = analogRead(A0);
  if (val > 800) Serial.println("\n[ALERT] Sensor A0 alto!");
}

// --- 3. SHELL & COMANDOS (CLI) ---

void printPrompt() {
  Serial.print("root@uno:/# ");
}

// Ayuda para separar comando de argumentos
String getArg(String data, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == ' ' || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void executeCommand(String input) {
  input.trim();
  String cmd = getArg(input, 0);
  String arg1 = getArg(input, 1);
  String arg2 = getArg(input, 2);

  if (cmd == "help") {
    Serial.println(F("--- NanoOS v3.0 Commands ---"));
    Serial.println(F(" ps         : Lista procesos (PID, Nombre, Estado)"));
    Serial.println(F(" kill [pid] : Detiene un proceso"));
    Serial.println(F(" start [pid]: Inicia un proceso"));
    Serial.println(F(" nice [pid] [ms] : Cambia intervalo de ejecucion"));
    Serial.println(F(" mem        : Muestra RAM libre"));
    Serial.println(F(" gpio [pin] [0/1]: Escribe en pin digital"));
    Serial.println(F(" adc [pin]  : Lee pin analogico (0-5)"));
    Serial.println(F(" reboot     : Reinicia el sistema"));
  }
  else if (cmd == "ps") {
    Serial.println(F("PID\tNAME\t\tINTERVAL\tSTATUS"));
    for (int i = 0; i < MAX_PIDS; i++) {
      Serial.print(i);
      Serial.print("\t");
      Serial.print(processTable[i].name);
      Serial.print("\t\t");
      Serial.print(processTable[i].interval);
      Serial.print("ms\t\t");
      Serial.println(processTable[i].active ? "RUNNING" : "STOPPED");
    }
  }
  else if (cmd == "kill") {
    int pid = arg1.toInt();
    if (pid >= 0 && pid < MAX_PIDS) {
      processTable[pid].active = false;
      Serial.println("Process " + String(pid) + " killed.");
    } else Serial.println("Error: PID invalido");
  }
  else if (cmd == "start") {
    int pid = arg1.toInt();
    if (pid >= 0 && pid < MAX_PIDS) {
      processTable[pid].active = true;
      Serial.println("Process " + String(pid) + " started.");
    } else Serial.println("Error: PID invalido");
  }
  else if (cmd == "nice") { // Cambiar prioridad/velocidad
    int pid = arg1.toInt();
    int interval = arg2.toInt();
    if (pid >= 0 && pid < MAX_PIDS && interval > 0) {
      processTable[pid].interval = interval;
      Serial.println("Priority changed for PID " + String(pid));
    }
  }
  else if (cmd == "mem") {
    Serial.print("Free RAM: ");
    Serial.print(freeRam());
    Serial.println(" bytes");
  }
  else if (cmd == "gpio") {
    int pin = arg1.toInt();
    int val = arg2.toInt();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val);
    Serial.println("GPIO " + String(pin) + " set to " + String(val));
  }
  else if (cmd == "adc") {
    int pin = arg1.toInt();
    int val = analogRead(pin);
    Serial.println("ADC A" + String(pin) + ": " + String(val));
  }
  else if (cmd == "reboot") {
    Serial.println("Rebooting...");
    delay(100);
    asm volatile ("jmp 0");
  }
  else {
    Serial.println("Command not found: " + cmd);
  }
  printPrompt();
}

// --- 4. INITIALIZATION ---

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  // Inicializar tabla de procesos
  processTable[0] = {"Blinker", proc_blink, 1000, 0, true};
  processTable[1] = {"Logger ", proc_logger, 5000, 0, true};
  processTable[2] = {"Sensor ", proc_sensor, 2000, 0, true};

  Serial.println(F("\nWelcome to NanoOS v3.0"));
  Serial.println(F("Type 'help' for available commands."));
  printPrompt();
}

// --- 5. MAIN LOOP (SCHEDULER) ---

void loop() {
  unsigned long currentMillis = millis();

  // 1. Scheduler (Round Robin cooperativo)
  for (int i = 0; i < MAX_PIDS; i++) {
    if (processTable[i].active) {
      if (currentMillis - processTable[i].lastRun >= processTable[i].interval) {
        processTable[i].lastRun = currentMillis;
        processTable[i].function();
      }
    }
  }

  // 2. Shell Input
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      executeCommand(input);
    }
  }
}