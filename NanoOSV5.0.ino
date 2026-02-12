/*
  NanoOS v4.0 - Auth, Shell Extensions + TTT Bot
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

// Variables de Sesión (NUEVO)
bool isLoggedIn = false;
int loginStep = 0; // 0: User, 1: Pass
String sessionUser = "";
const String SYS_USER = "admin";
const String SYS_PASS = "1234";

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
  int wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  int move = -1;

  for(int k=0; k<2; k++) {
    char target = (k == 0) ? 'O' : 'X';
    for(int i=0; i<8; i++) {
       int a = wins[i][0], b = wins[i][1], c = wins[i][2];
       int count = (board[a] == target) + (board[b] == target) + (board[c] == target);
       int empty = (board[a] == ' ') + (board[b] == ' ') + (board[c] == ' ');
       
       if(count == 2 && empty == 1) {
          if(board[a] == ' ') move = a;
          else if(board[b] == ' ') move = b;
          else move = c;
          goto play; 
       }
    }
  }
  if(board[4] == ' ') move = 4;
  else {
    for(int i=0; i<9; i++) if(board[i] == ' ') { move = i; break; } 
  }

play:
  board[move] = 'O';
  Serial.print(F("Bot juega en: ")); Serial.println(move + 1);
}

void playTTT(String input) {
  input.trim();
  if(input == "exit") { inGame = false; Serial.println(F("Juego cerrado.")); return; }

  int move = input.toInt() - 1;
  if(move >= 0 && move < 9 && board[move] == ' ') {
    board[move] = currentPlayer;
    checkWinner();
    
    if(inGame) {
      currentPlayer = 'O';
      delay(300); 
      botMove();
      checkWinner();
      if(inGame) {
        currentPlayer = 'X'; 
        drawBoard();
      }
    }
  } else {
    Serial.println(F("Movimiento inválido."));
  }
}

// --- 4. SHELL & COMANDOS (CLI) ---

void printPrompt() {
  if(isLoggedIn) {
    Serial.print(sessionUser); Serial.print("@uno:/# ");
  } else {
    if(loginStep == 0) Serial.print(F("NanoOS Login: "));
    else Serial.print(F("Password: "));
  }
}

// Gestión de Login
void handleLogin(String input) {
  input.trim();
  if(loginStep == 0) {
    if(input == SYS_USER) {
      loginStep = 1;
    } else {
      Serial.println(F("Usuario incorrecto."));
    }
  } else if(loginStep == 1) {
    if(input == SYS_PASS) {
      isLoggedIn = true;
      sessionUser = SYS_USER;
      Serial.println(F("\nAcceso concedido. Bienvenido a NanoOS v4.0"));
      Serial.println(F("Escribe 'help' para comenzar.\n"));
    } else {
      Serial.println(F("Password incorrecto."));
      loginStep = 0; // Reiniciar intento
    }
  }
  printPrompt();
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
    Serial.println(F(" ttt        : Inicia el Tres en Raya"));
    Serial.println(F(" ps         : Lista procesos"));
    Serial.println(F(" kill [pid] : Detiene un proceso"));
    Serial.println(F(" start [pid]: Inicia un proceso"));
    Serial.println(F(" nice [pid] : Cambia intervalo"));
    Serial.println(F(" mem        : Muestra RAM libre"));
    Serial.println(F(" gpio/adc   : Control Hardware"));
    Serial.println(F(" whoami     : Usuario actual"));
    Serial.println(F(" uptime     : Tiempo de actividad"));
    Serial.println(F(" logout     : Cerrar sesion"));
    Serial.println(F(" reboot     : Reiniciar sistema"));
  }
  else if (cmd == "whoami") {
    Serial.println(sessionUser);
  }
  else if (cmd == "uptime") {
    Serial.print(millis() / 1000); Serial.println(F(" seg"));
  }
  else if (cmd == "ver") {
    Serial.println(F("NanoOS Kernel v4.0 (Simulado) - Build 2023"));
  }
  else if (cmd == "cls") {
    for(int i=0; i<10; i++) Serial.println();
  }
  else if (cmd == "logout") {
    isLoggedIn = false;
    loginStep = 0;
    sessionUser = "";
    Serial.println(F("Sesion cerrada."));
    printPrompt();
    return;
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
    }
  }
  else if (cmd == "start") {
    int pid = arg1.toInt();
    if (pid >= 0 && pid < MAX_PIDS) {
      processTable[pid].active = true;
      Serial.println("Process " + String(pid) + " started.");
    }
  }
  else if (cmd == "nice") {
    int pid = arg1.toInt();
    int interval = arg2.toInt();
    if (pid >= 0 && pid < MAX_PIDS && interval > 0) {
      processTable[pid].interval = interval;
      Serial.println("Priority changed.");
    }
  }
  else if (cmd == "mem") {
    Serial.print("Free RAM: "); Serial.print(freeRam()); Serial.println(" bytes");
  }
  else if (cmd == "gpio") {
    int pin = arg1.toInt();
    int val = arg2.toInt();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val);
    Serial.println("GPIO set.");
  }
  else if (cmd == "adc") {
    int pin = arg1.toInt();
    Serial.println("ADC: " + String(analogRead(pin)));
  }
  else if (cmd == "reboot") {
    Serial.println("Rebooting...");
    delay(100);
    asm volatile ("jmp 0");
  }
  else {
    Serial.println("Comando desconocido: " + cmd);
  }
  if(!inGame && isLoggedIn) printPrompt();
}

// --- 5. INITIALIZATION ---

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  processTable[0] = {"Blinker", proc_blink, 1000, 0, true};
  processTable[1] = {"Logger ", proc_logger, 5000, 0, true};
  processTable[2] = {"Sensor ", proc_sensor, 2000, 0, true};

  Serial.println(F("\nInicializando NanoOS v4.0..."));
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
      if (!isLoggedIn) {
        handleLogin(input);
      } 
      else if (inGame) {
        playTTT(input);
      } 
      else {
        executeCommand(input);
      }
    }
  }
}