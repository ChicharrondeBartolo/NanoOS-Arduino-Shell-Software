/*
  NanoOS v4.0 - Advanced Shell & Process Control + TicTacToe (PvE Mode)
  Arquitectura: Kernel Monolítico (Simulado)
*/

// --- 1. KERNEL & GESTIÓN DE MEMORIA ---

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

struct Process {
  String name;
  void (*function)();
  unsigned long interval;
  unsigned long lastRun;
  bool active;
};

const int MAX_PIDS = 3;
Process processTable[MAX_PIDS];

// Variables del Juego (Tres en Raya)
bool inGame = false;
char board[9];
char currentPlayer = 'X'; // X siempre es Humano, O es Bot

// --- 2. PROCESOS DEL SISTEMA (USER SPACE) ---

void proc_blink() {
  static bool s = false;
  s = !s;
  digitalWrite(13, s);
}

void proc_logger() {
  // PID 1: Servicio de registro
}

void proc_sensor() {
  int val = analogRead(A0);
  if (val > 800) Serial.println(F("\n[ALERT] Sensor A0 alto!"));
}

// --- 3. LÓGICA DEL JUEGO (TRES EN RAYA vs CPU) ---

void drawBoard() {
  Serial.println(F("\n 1 | 2 | 3 "));
  Serial.print(F(" ")); Serial.print(board[0]); Serial.print(F(" | ")); Serial.print(board[1]); Serial.print(F(" | ")); Serial.println(board[2]);
  Serial.println(F("-----------"));
  Serial.print(F(" ")); Serial.print(board[3]); Serial.print(F(" | ")); Serial.print(board[4]); Serial.print(F(" | ")); Serial.println(board[5]);
  Serial.println(F("-----------"));
  Serial.print(F(" ")); Serial.print(board[6]); Serial.print(F(" | ")); Serial.print(board[7]); Serial.print(F(" | ")); Serial.println(board[8]);
  if(inGame) {
     Serial.print(F("\nTurno de ")); Serial.print(currentPlayer); Serial.println(F(". Elige (1-9) o 'exit':"));
  }
}

void checkWinner() {
  int wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  for(int i=0; i<8; i++) {
    if(board[wins[i][0]] != ' ' && board[wins[i][0]] == board[wins[i][1]] && board[wins[i][1]] == board[wins[i][2]]) {
      drawBoard();
      Serial.print(F("¡GANADOR: ")); Serial.print(board[wins[i][0]]); Serial.println(F("!"));
      inGame = false;
      return;
    }
  }
  bool tie = true;
  for(int i=0; i<9; i++) if(board[i] == ' ') tie = false;
  if(tie) { drawBoard(); Serial.println(F("¡EMPATE!")); inGame = false; }
}

// --- LÓGICA DEL BOT (Compacta) ---
void botMove() {
  // 1. Definir líneas ganadoras (local para ahorrar global var space)
  int wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  int move = -1;

  // 2. Buscar victoria inmediata (O) o bloqueo necesario (X)
  // Prioridad k=0: Ganar yo (O), k=1: Bloquear al humano (X)
  for(int k=0; k<2; k++) {
    char target = (k == 0) ? 'O' : 'X';
    for(int i=0; i<8; i++) {
       int a = wins[i][0], b = wins[i][1], c = wins[i][2];
       // Contar fichas del target en la línea
       int count = (board[a] == target) + (board[b] == target) + (board[c] == target);
       int empty = (board[a] == ' ') + (board[b] == ' ') + (board[c] == ' ');
       
       if(count == 2 && empty == 1) {
          if(board[a] == ' ') move = a;
          else if(board[b] == ' ') move = b;
          else move = c;
          goto play; // Salto directo para ahorrar lógica
       }
    }
  }

  // 3. Estrategia: Centro -> Aleatorio/Secuencial
  if(board[4] == ' ') move = 4;
  else {
    for(int i=0; i<9; i++) if(board[i] == ' ') { move = i; break; } // Primer disponible
  }

play:
  board[move] = 'O';
  Serial.print(F("Bot juega en: ")); Serial.println(move + 1);
}

void playTTT(String input) {
  input.trim();
  if(input == "exit") { inGame = false; Serial.println(F("Juego cerrado.")); return; }

  // Turno HUMANO (X)
  int move = input.toInt() - 1;
  if(move >= 0 && move < 9 && board[move] == ' ') {
    board[move] = currentPlayer; // 'X'
    checkWinner();
    
    // Si el juego sigue, turno del BOT (O)
    if(inGame) {
      currentPlayer = 'O';
      delay(300); // Pequeña pausa para realismo
      botMove();
      checkWinner();
      if(inGame) {
        currentPlayer = 'X'; // Devuelve turno al humano
        drawBoard();
      }
    }
  } else {
    Serial.println(F("Movimiento inválido."));
  }
}

// --- 4. SHELL & COMANDOS (CLI) ---

void printPrompt() {
  Serial.print("root@uno:/# ");
}

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
    Serial.println(F("--- NanoOS v4.0 Commands ---"));
    Serial.println(F(" ttt        : Inicia el Tres en Raya (vs Bot)"));
    Serial.println(F(" ps         : Lista procesos"));
    Serial.println(F(" kill [pid] : Detiene un proceso"));
    Serial.println(F(" start [pid]: Inicia un proceso"));
    Serial.println(F(" nice [pid] [ms] : Cambia intervalo"));
    Serial.println(F(" mem        : Muestra RAM libre"));
    Serial.println(F(" gpio [pin] [0/1]: Escribe digital"));
    Serial.println(F(" adc [pin]  : Lee analogico"));
    Serial.println(F(" reboot     : Reinicia el sistema"));
  }
  else if (cmd == "ttt") {
    for(int i=0; i<9; i++) board[i] = ' ';
    inGame = true;
    currentPlayer = 'X';
    drawBoard();
    return;
  }
  else if (cmd == "ps") {
    Serial.println(F("PID\tNAME\t\tINTERVAL\tSTATUS"));
    for (int i = 0; i < MAX_PIDS; i++) {
      Serial.print(i); Serial.print("\t");
      Serial.print(processTable[i].name); Serial.print("\t\t");
      Serial.print(processTable[i].interval); Serial.print("ms\t\t");
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
  else if (cmd == "nice") {
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
  if(!inGame) printPrompt();
}

// --- 5. INITIALIZATION ---

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  processTable[0] = {"Blinker", proc_blink, 1000, 0, true};
  processTable[1] = {"Logger ", proc_logger, 5000, 0, true};
  processTable[2] = {"Sensor ", proc_sensor, 2000, 0, true};

  Serial.println(F("\nWelcome to NanoOS v4.0 "));
  Serial.println(F("Type 'help' for available commands."));
  printPrompt();
}

// --- 6. MAIN LOOP (SCHEDULER) ---

void loop() {
  unsigned long currentMillis = millis();

  for (int i = 0; i < MAX_PIDS; i++) {
    if (processTable[i].active) {
      if (currentMillis - processTable[i].lastRun >= processTable[i].interval) {
        processTable[i].lastRun = currentMillis;
        processTable[i].function();
      }
    }
  }

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      if(inGame) playTTT(input);
      else executeCommand(input);
    }
  }
}