/*
  Arquitectura: Kernel Monolítico (Simulado)
*/

// --- 1. KERNEL & GESTIÓN DE MEMORIA ---

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Variables del Sistema
bool isLoggedIn = false;
int loginStep = 0; 
String sessionUser = "";
const String SYS_USER = "admin";
const String SYS_PASS = "1234";
int activeApp = 0; // 0: Shell, 1: TTT, 2: Battleship, 3: Chess

// --- 2. JUEGO 1: TRES EN RAYA (TTT) ---
char ttt_board[9];
char ttt_player = 'X';

void ttt_draw() {
  Serial.println(F("\n 1 | 2 | 3 "));
  Serial.print(F(" ")); Serial.print(ttt_board[0]); Serial.print(F(" | ")); Serial.print(ttt_board[1]); Serial.print(F(" | ")); Serial.println(ttt_board[2]);
  Serial.println(F("-----------"));
  Serial.print(F(" ")); Serial.print(ttt_board[3]); Serial.print(F(" | ")); Serial.print(ttt_board[4]); Serial.print(F(" | ")); Serial.println(ttt_board[5]);
  Serial.println(F("-----------"));
  Serial.print(F(" ")); Serial.print(ttt_board[6]); Serial.print(F(" | ")); Serial.print(ttt_board[7]); Serial.print(F(" | ")); Serial.println(ttt_board[8]);
  if(activeApp == 1) Serial.println(F("Tu turno (1-9) o 'exit':"));
}

void ttt_check() {
  int wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  for(int i=0; i<8; i++) {
    if(ttt_board[wins[i][0]] != ' ' && ttt_board[wins[i][0]] == ttt_board[wins[i][1]] && ttt_board[wins[i][1]] == ttt_board[wins[i][2]]) {
      ttt_draw();
      Serial.print(F("GANADOR: ")); Serial.println(ttt_board[wins[i][0]]);
      activeApp = 0; return;
    }
  }
  bool tie = true;
  for(int i=0; i<9; i++) if(ttt_board[i] == ' ') tie = false;
  if(tie) { ttt_draw(); Serial.println(F("EMPATE!")); activeApp = 0; }
}

void ttt_bot() {
  // Simple: Ganar > Bloquear > Random
  int wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  int move = -1;
  for(int k=0; k<2; k++) {
    char t = (k==0) ? 'O' : 'X';
    for(int i=0; i<8; i++) {
       int a=wins[i][0], b=wins[i][1], c=wins[i][2];
       if(((ttt_board[a]==t)+(ttt_board[b]==t)+(ttt_board[c]==t))==2 && ((ttt_board[a]==' ')+(ttt_board[b]==' ')+(ttt_board[c]==' '))==1) {
          if(ttt_board[a]==' ') move=a; else if(ttt_board[b]==' ') move=b; else move=c;
          goto play;
       }
    }
  }
  if(ttt_board[4]==' ') move=4;
  else { for(int i=0;i<9;i++) if(ttt_board[i]==' ') { move=i; break; } }
play:
  ttt_board[move] = 'O';
}

void ttt_loop(String input) {
  int m = input.toInt() - 1;
  if(m>=0 && m<9 && ttt_board[m]==' ') {
    ttt_board[m] = 'X';
    ttt_check();
    if(activeApp == 1) {
      ttt_bot();
      ttt_check();
      if(activeApp == 1) ttt_draw();
    }
  }
}

// --- 3. JUEGO 2: HUNDIR LA FLOTA (5x5 Nano) ---
// 0=Agua, 1=Barco(Oculto), 2=AguaDisparada(X), 3=Tocado(!)
byte bs_grid[25]; 
int bs_ships = 3;

void bs_init() {
  for(int i=0; i<25; i++) bs_grid[i] = 0;
  bs_ships = 3;
  // Colocar 3 barcos aleatorios
  int count = 0;
  while(count < 3) {
    int r = random(0, 25);
    if(bs_grid[r] == 0) { bs_grid[r] = 1; count++; }
  }
  Serial.println(F("=== BATTLESHIP 5x5 ==="));
  Serial.println(F("Formato: A1, B3, E5... (exit para salir)"));
}

void bs_draw() {
  Serial.println(F("  1 2 3 4 5"));
  for(int y=0; y<5; y++) {
    Serial.print((char)('A'+y)); Serial.print(" ");
    for(int x=0; x<5; x++) {
      byte cell = bs_grid[y*5+x];
      if(cell == 2) Serial.print("~ "); // Agua descubierta
      else if(cell == 3) Serial.print("! "); // Tocado
      else Serial.print(". "); // Niebla
    }
    Serial.println();
  }
}

void bs_loop(String input) {
  input.toUpperCase();
  if(input.length() < 2) return;
  int y = input.charAt(0) - 'A';
  int x = input.charAt(1) - '1';
  
  if(x>=0 && x<5 && y>=0 && y<5) {
    int idx = y*5+x;
    if(bs_grid[idx] == 1) {
      bs_grid[idx] = 3; // Tocado
      Serial.println(F("\n>>> IMPACTO! <<<"));
      bs_ships--;
    } else if(bs_grid[idx] == 0) {
      bs_grid[idx] = 2; // Agua
      Serial.println(F("\n>>> Agua. <<<"));
    } else {
      Serial.println(F("Ya disparaste ahi."));
    }
    
    if(bs_ships == 0) {
      Serial.println(F("¡VICTORIA! Flota enemiga hundida."));
      activeApp = 0;
      return;
    }
    
    // Turno Bot (Simulado: El bot dispara a una "grilla imaginaria" del jugador y falla/acierta random)
    // Para simplificar memoria, solo jugamos nosotros contra el mapa.
    bs_draw();
    Serial.print("Barcos restantes: "); Serial.println(bs_ships);
  } else {
    Serial.println(F("Coordenada invalida (Ej: A1, C4)"));
  }
}

// --- 4. JUEGO 3: NANO CHESS (Text Based) ---
// Tablero 8x8. Mayus=Blancas(User), Minus=Negras(Bot)
char chess_b[64];
const char* piece_order = "rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR";

void chess_init() {
  for(int i=0; i<64; i++) chess_b[i] = piece_order[i];
  Serial.println(F("=== NANO CHESS ==="));
  Serial.println(F("Mueve: e2e4, a7a5... (Blancas/Mayus eres TU)"));
}

void chess_draw() {
  Serial.println(F("  a b c d e f g h"));
  for(int y=0; y<8; y++) {
    Serial.print(8-y); Serial.print(" ");
    for(int x=0; x<8; x++) {
      char c = chess_b[y*8+x];
      if(c == '.') Serial.print("- ");
      else { Serial.print(c); Serial.print(" "); }
    }
    Serial.print(8-y);
    Serial.println();
  }
  Serial.println(F("  a b c d e f g h"));
}

void chess_bot() {
  // Bot muy simple: Busca una pieza negra y trata de moverla a un espacio valido o captura
  // Intentos limitados para no bloquear
  for(int tries=0; tries<50; tries++) {
    int src = random(0, 64);
    char p = chess_b[src];
    // Si es pieza negra (minúscula)
    if(p >= 'a' && p <= 'z') {
      int dst = -1;
      // Lógica super simple de movimiento (Peón avanza, otros random)
      if(p == 'p') { // Peón negro baja (+8)
         if(src+8 < 64 && chess_b[src+8]=='.') dst = src+8;
         else if(src+9 < 64 && chess_b[src+9] < 'a' && chess_b[src+9] != '.') dst=src+9; // Comer
      } else {
         // Cualquier otra pieza, movimiento aleatorio +/- 1,8,7,9 (vecinos)
         int offsets[] = {1,-1,8,-8,7,-7,9,-9};
         int off = offsets[random(0,8)];
         int cand = src + off;
         if(cand >= 0 && cand < 64) {
             if(chess_b[cand] == '.' || chess_b[cand] < 'a') dst = cand; // Vacio o Blanca
         }
      }
      
      if(dst != -1) {
        chess_b[dst] = p;
        chess_b[src] = '.';
        Serial.print(F("Bot mueve: ")); 
        Serial.print((char)('a' + (src%8))); Serial.print(8-(src/8));
        Serial.print("->");
        Serial.print((char)('a' + (dst%8))); Serial.println(8-(dst/8));
        return;
      }
    }
  }
}

void chess_loop(String input) {
  if(input.length() != 4) { Serial.println(F("Formato incorrecto. Usa e2e4")); return; }
  
  int x1 = input.charAt(0) - 'a';
  int y1 = 8 - (input.charAt(1) - '0');
  int x2 = input.charAt(2) - 'a';
  int y2 = 8 - (input.charAt(3) - '0');
  
  if(x1<0 || x1>7 || y1<0 || y1>7 || x2<0 || x2>7 || y2<0 || y2>7) {
    Serial.println(F("Fuera de rango.")); return;
  }
  
  int src = y1*8+x1;
  int dst = y2*8+x2;
  char p = chess_b[src];
  
  // Validación básica: Es mi pieza?
  if(p < 'A' || p > 'Z') { Serial.println(F("No es tu pieza o esta vacio.")); return; }
  
  // Movimiento (Sin validar reglas complejas para ahorrar espacio, confiamos en el usuario)
  chess_b[dst] = p;
  chess_b[src] = '.';
  
  chess_bot();
  chess_draw();
}

// --- 5. SHELL & SISTEMA ---

void printPrompt() {
  if(isLoggedIn) {
    if(activeApp == 0) { Serial.print(sessionUser); Serial.print("@uno:/# "); }
  } else {
    if(loginStep == 0) Serial.print(F("Login: ")); else Serial.print(F("Password: "));
  }
}

void executeCommand(String input) {
  input.trim();
  int sp = input.indexOf(' ');
  String cmd = (sp == -1) ? input : input.substring(0, sp);

  if (cmd == "help") {
    Serial.println(F("--- NanoOS v5.1 Commands ---"));
    Serial.println(F(" ttt        : Tres en Raya"));
    Serial.println(F(" bat        : Hundir la Flota (Battleship)"));
    Serial.println(F(" chess      : Ajedrez (MicroChess)"));
    Serial.println(F(" mem        : RAM Libre"));
    Serial.println(F(" logout     : Cerrar Sesion"));
    Serial.println(F(" reboot     : Reiniciar"));
    Serial.println(F(" cls        : Limpiar pantalla"));
  }
  else if (cmd == "ttt") {
    for(int i=0; i<9; i++) ttt_board[i] = ' ';
    activeApp = 1; ttt_player='X'; ttt_draw();
  }
  else if (cmd == "bat") {
    bs_init(); activeApp = 2; bs_draw();
  }
  else if (cmd == "chess") {
    chess_init(); activeApp = 3; chess_draw();
  }
  else if (cmd == "mem") {
    Serial.print("Free RAM: "); Serial.print(freeRam()); Serial.println(" B");
  }
  else if (cmd == "reboot") {
    asm volatile ("jmp 0");
  }
  else if (cmd == "logout") {
    isLoggedIn = false; loginStep = 0; Serial.println(F("Bye."));
  }
  else if (cmd == "cls") {
    for(int i=0;i<20;i++) Serial.println();
  }
  else {
    Serial.println("Error.");
  }
}

void handleLogin(String input) {
  input.trim();
  if(loginStep == 0) {
    if(input == SYS_USER) loginStep = 1; else Serial.println(F("Usuario invalido."));
  } else {
    if(input == SYS_PASS) {
      isLoggedIn = true; sessionUser = SYS_USER;
      Serial.println(F("Bienvenido a NanoOS v5.1 Game Station"));
      Serial.println(F("Escribe 'help' para ver los juegos."));
    } else {
      Serial.println(F("Password invalido.")); loginStep = 0;
    }
  }
}

// --- 6. SETUP & LOOP ---

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  Serial.println(F("\nNanoOS v5.1 Booting..."));
  printPrompt();
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      if (!isLoggedIn) {
        handleLogin(input);
      } else {
        if (input == "exit") {
          activeApp = 0;
          Serial.println(F("Saliendo al Shell..."));
        } else {
          if (activeApp == 0) executeCommand(input);
          else if (activeApp == 1) ttt_loop(input);
          else if (activeApp == 2) bs_loop(input);
          else if (activeApp == 3) chess_loop(input);
        }
      }
      printPrompt();
    }
  }
}