/*
  NanoOS v6.0 - GUI Emulated Wrapper (Professional Edition)
  Novedades: PWM Control, Scan de Señales, Gestión de PIDs y Neofetch.
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

bool inGame = false;
char board[9];
char currentPlayer = 'X'; 

bool isLoggedIn = false;
int loginStep = 0; 
String sessionUser = "";
const String SYS_USER = "admin";
const String SYS_PASS = "1234";

// --- MOTOR DE RENDERIZADO (Simulación de Interfaz) ---
void drawUIHeader(String title) {
  Serial.println(F("\n.____________________________________________."));
  Serial.print(F("| [X] NanoOS v6.0 - "));
  Serial.print(title);
  int spaces = 24 - title.length();
  for(int i=0; i<spaces; i++) Serial.print(" ");
  Serial.println(F("|"));
  Serial.println(F("|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|"));
}

void drawUIFooter() {
  Serial.println(F("|____________________________________________|"));
  Serial.print(F(" RAM: ")); Serial.print(freeRam()); Serial.println(F(" bytes | CPU: 16MHz | v6.0 GUI"));
}

// --- 2. PROCESOS DEL SISTEMA ---
void proc_blink() {
  static bool s = false;
  s = !s;
  digitalWrite(13, s);
}
void proc_logger() {}
void proc_sensor() {
  int val = analogRead(A0);
  if (val > 800) {
    Serial.println(F("\n[!] ALERTA: Voltaje alto en A0"));
  }
}

// --- 3. LÓGICA DEL JUEGO (TTT) ---
void drawBoard() {
  drawUIHeader("TRES EN RAYA");
  Serial.print(F("|      ")); Serial.print(board[0]); Serial.print(F(" | ")); Serial.print(board[1]); Serial.print(F(" | ")); Serial.print(board[2]); Serial.println(F("      |"));
  Serial.println(F("|     -----------      |"));
  Serial.print(F("|      ")); Serial.print(board[3]); Serial.print(F(" | ")); Serial.print(board[4]); Serial.print(F(" | ")); Serial.print(board[5]); Serial.println(F("      |"));
  Serial.println(F("|     -----------      |"));
  Serial.print(F("|      ")); Serial.print(board[6]); Serial.print(F(" | ")); Serial.print(board[7]); Serial.print(F(" | ")); Serial.print(board[8]); Serial.println(F("      |"));
  drawUIFooter();
  if(inGame) {
     Serial.print(F("\nTurno: ")); Serial.print(currentPlayer); Serial.println(F(". (1-9) o 'exit':"));
  }
}

void checkWinner() {
  int wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  for(int i=0; i<8; i++) {
    if(board[wins[i][0]] != ' ' && board[wins[i][0]] == board[wins[i][1]] && board[wins[i][1]] == board[wins[i][2]]) {
      drawBoard();
      Serial.print(F(">>> GANADOR: ")); Serial.print(board[wins[i][0]]); Serial.println(F(" <<<"));
      inGame = false;
      return;
    }
  }
  bool tie = true;
  for(int i=0; i<9; i++) if(board[i] == ' ') tie = false;
  if(tie) { drawBoard(); Serial.println(F(">>> EMPATE <<<")); inGame = false; }
}

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
}

void playTTT(String input) {
  input.trim();
  if(input == "exit") { inGame = false; Serial.println(F("Saliendo...")); return; }
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
  }
}

// --- 4. NUEVAS FUNCIONES: MORSE & GPIO ---

void sendMorseBit(bool isDash) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(isDash ? 600 : 200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
}

void transmitMorse(String msg) {
  msg.toLowerCase();
  drawUIHeader("MORSE TX");
  Serial.print(F("| TX: ")); Serial.print(msg);
  int padding = 38 - msg.length();
  for(int i=0; i<padding; i++) Serial.print(" "); Serial.println("|");
  for(int i=0; i<msg.length(); i++) {
    char c = msg[i];
    if(c == 's') { sendMorseBit(0); sendMorseBit(0); sendMorseBit(0); }
    else if(c == 'o') { sendMorseBit(1); sendMorseBit(1); sendMorseBit(1); }
    delay(400);
  }
  drawUIFooter();
}

// --- 5. SHELL & COMANDOS (CLI) ---
void printPrompt() {
  if(isLoggedIn) {
    Serial.print(sessionUser); Serial.print(F("@nano:~$ "));
  } else {
    drawUIHeader("LOGIN REQUERIDO");
    if(loginStep == 0) Serial.print(F("Usuario: "));
    else Serial.print(F("Password: "));
  }
}

void handleLogin(String input) {
  input.trim();
  if(loginStep == 0) {
    if(input == SYS_USER) loginStep = 1;
  } else if(loginStep == 1) {
    if(input == SYS_PASS) {
      isLoggedIn = true;
      sessionUser = SYS_USER;
      Serial.println(F("\nAcceso Concedido."));
      drawUIHeader("ESCRITORIO");
      Serial.println(F("| Escribe 'help' para ver los comandos.      |"));
      drawUIFooter();
    } else {
      Serial.println(F("Acceso Denegado."));
      loginStep = 0;
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
    drawUIHeader("AYUDA DEL SISTEMA");
    Serial.println(F("| ttt    : Juego | ps     : Procesos         |"));
    Serial.println(F("| gpio   : Pines | morse [TXT] : Señal LED   |"));
    Serial.println(F("| pwm    : Pot.  | scan   : ADC Pins         |"));
    Serial.println(F("| start  : PID   | kill   : PID              |"));
    Serial.println(F("| neofetch: Info | logout : Salir            |"));
    drawUIFooter();
  }
  else if (cmd == "ttt") {
    for(int i=0; i<9; i++) board[i] = ' ';
    inGame = true; currentPlayer = 'X';
    drawBoard();
    return;
  }
  else if (cmd == "gpio") {
    int pin = arg1.toInt();
    drawUIHeader("GPIO CONTROL");
    if(arg2 == "on") {
      pinMode(pin, OUTPUT); digitalWrite(pin, HIGH);
      Serial.print(F("| PIN ")); Serial.print(pin); Serial.println(F(" establecido en HIGH        |"));
    } else if(arg2 == "off") {
      pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
      Serial.print(F("| PIN ")); Serial.print(pin); Serial.println(F(" establecido en LOW         |"));
    } else {
      int val = digitalRead(pin);
      Serial.print(F("| PIN ")); Serial.print(pin); Serial.print(F(" estado: ")); 
      Serial.print(val ? "HIGH" : "LOW "); Serial.println(F("             |"));
    }
    drawUIFooter();
  }
  else if (cmd == "pwm") {
    int pin = arg1.toInt();
    int val = arg2.toInt();
    analogWrite(pin, val);
    drawUIHeader("PWM ANALOG OUT");
    Serial.print(F("| PIN ")); Serial.print(pin); Serial.print(F(" -> ")); Serial.print(val); Serial.println(F("/255            |"));
    drawUIFooter();
  }
  else if (cmd == "scan") {
    drawUIHeader("ADC SCANNER");
    for(int i=0; i<6; i++) {
      Serial.print(F("| A")); Serial.print(i); Serial.print(F(": "));
      Serial.print(analogRead(i)); Serial.println(F("                          |"));
    }
    drawUIFooter();
  }
  else if (cmd == "kill") {
    int pid = arg1.toInt();
    if(pid >= 0 && pid < MAX_PIDS) processTable[pid].active = false;
    Serial.println(F("Proceso detenido."));
  }
  else if (cmd == "start") {
    int pid = arg1.toInt();
    if(pid >= 0 && pid < MAX_PIDS) processTable[pid].active = true;
    Serial.println(F("Proceso iniciado."));
  }
  else if (cmd == "neofetch") {
    Serial.println(F("                  admin@NanoOS"));
    Serial.println(F("  |\\   ||                       ------------"));
    Serial.println(F("  ||\\  ||       OS: NanoOS v6.0 Professional"));
    Serial.println(F("  || \\ ||             Kernel: Monolithic Sim"));
    Serial.println(F("  ||  \\||                 Board: Arduino Uno"));
            
  }
  else if (cmd == "morse") {
    transmitMorse(arg1);
  }
  else if (cmd == "ps") {
    drawUIHeader("ADMIN. TAREAS");
    for (int i = 0; i < MAX_PIDS; i++) {
      Serial.print(F("| ")); Serial.print(i); Serial.print(F(" | "));
      Serial.print(processTable[i].name); Serial.print(F(" | "));
      Serial.println(processTable[i].active ? "RUNNING |" : "STOPPED |");
    }
    drawUIFooter();
  }
  else if (cmd == "mem") {
    drawUIHeader("MEMORIA");
    Serial.print(F("| RAM Libre: ")); Serial.print(freeRam()); Serial.println(F(" bytes          |"));
    drawUIFooter();
  }
  else if (cmd == "cls") {
    for(int i=0; i<30; i++) Serial.println();
  }
  else if (cmd == "logout") {
    isLoggedIn = false; loginStep = 0;
    Serial.println(F("Cerrando..."));
    printPrompt();
    return;
  }
  else if (cmd == "reboot") {
    asm volatile ("jmp 0");
  }
  else {
    Serial.println(F("CMD Error."));
  }
  if(!inGame && isLoggedIn) printPrompt();
}

// --- 6. INITIALIZATION ---
void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  processTable[0] = {"Blinker", proc_blink, 1000, 0, true};
  processTable[1] = {"Logger ", proc_logger, 5000, 0, true};
  processTable[2] = {"Sensor ", proc_sensor, 2000, 0, true};
  Serial.println(F("\n\nBOOTING NanoOS..."));
  printPrompt();
}

// --- 7. MAIN LOOP ---
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
      if (!isLoggedIn) handleLogin(input);
      else if (inGame) playTTT(input);
      else executeCommand(input);
    }
  }
}