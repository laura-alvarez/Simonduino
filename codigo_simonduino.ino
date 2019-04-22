#include <Arduino.h>
#include <Wire.h>
//LIBRERIAS PROPIAS I2C PARA EL LCD
#include <LiquidCrystal_I2C.h>

#include <Chrono.h>

//Declaramos los componentes
//LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
//VALORES PARA CARACTERES ESPECIALES LCD
char RIGHT_ARROW = 0x7E;
char LEFT_ARROW = 0x7F;
uint8_t check[8] = { 0x0, 0x1, 0x3, 0x16, 0x1c, 0x8, 0x0 };
uint8_t cross[8] = { 0x0, 0x1b, 0xe, 0x4, 0xe, 0x1b, 0x0 };
uint8_t skull[8] = { 0x00, 0x0E, 0x15, 0x1B, 0x0E, 0x0E, 0x00, 0x00 };
uint8_t up_arrow[8] = { 0x00, 0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x00 };
uint8_t down_arrow[8] = { 0x00, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00 };

//OPCIONES DEL MENÚ
char* opcionesMenu[1];
int currentOptionMenu;

//VALORES DE EVENTO PARA LOS PULSADORES
const int PRESSED = 0;
const int RELEASED = 1;

//PULSADORES Y LEDS DE LOS PULSADORES
//Botón Verde
const int GREEN = 0;
const int BUTTON_GREEN = 3;
const int LED_GREEN = 2;
int buttonGreenLastState = RELEASED;
//Botón rojo
const int RED = 1;
const int BUTTON_RED = 10;
const int LED_RED = 9;
int buttonRedLastState = RELEASED;
//Botón Azul
const int BLUE = 2;
const int BUTTON_BLUE = 13;
const int LED_BLUE = 12;
int buttonBlueLastState = RELEASED;
//Botón Amarillo
const int YELLOW = 3;
const int BUTTON_YELLOW = 5;
const int LED_YELLOW = 4;
int buttonYellowLastState = RELEASED;
//Botón Blanco
const int BUTTON_WHITE = 7;
const int LED_WHITE = 6;
int buttonWhiteLastState = RELEASED;

//Matriz correspondecia color -> pin led
int colorLedMatrix[4];

// Instanciate a metro object and set the interval to 500 milliseconds (0.5 seconds).
Chrono buttonWhiteChrono;
Chrono buttonTurnOffChrono;
Chrono buttonTurnOnChrono;
Chrono stanbyChrono;

//BUZZER
const int BUZZER = 11;

//Variable que almacena la secuencia de botones
int* secuence;

//Variable del turno actual
int maxLevel = 20;
int currentLevel;
int index;
int currentTimeOn;
int currentTimeOff;
int currentTimeSound;
const int MAXTIMEON = 800;
const int MAXTIMEOFF = 400;

const int ON = 1;
const int OFF = 0;

int whiteLedState = OFF;
int simonTurnLedState = OFF;

const char* BLANK_LINE = "                ";

//Variable que indica los ESTADOS
//Posibles estados de juego
int currentSimonduinoState, lastSimonduinoState;
const int INSIDE_MENU = 0;
const int ARDUINO_TURN = 1;
const int PLAYER_TURN = 2;
const int STANDBY = 3;
const int WAITING_ACTION = 4;
const int LEVELUP_OPTION = 5;
//VALORES JUEGOS

int center(const char* text) {
  if (strlen(text) > 16) {
    return 0;
  } else {
    return (16 - strlen(text)) / 2;
  }
}

void LoadMenuOptions() {
  currentOptionMenu = 0;
  opcionesMenu[0] = ("Level up!");
}

void menu() {
  lcd.clear();
  lcd.setCursor(center(opcionesMenu[currentOptionMenu]), 0);
  lcd.print(opcionesMenu[currentOptionMenu]); // @suppress("Ambiguous problem")
  lcd.setCursor(0, 1);
  lcd.print(RIGHT_ARROW); // @suppress("Ambiguous problem")
  lcd.print("(?) Push white"); // @suppress("Ambiguous problem")
  lcd.print(LEFT_ARROW); // @suppress("Ambiguous problem")
}

void menuHelp() {
  lcd.clear();
  lcd.setCursor(center("- green red -"), 0);
  lcd.write(1); // @suppress("Ambiguous problem")
  lcd.print(" green red "); // @suppress("Ambiguous problem")
  lcd.write(2); // @suppress("Ambiguous problem")
  lcd.setCursor(0, 1); // @suppress("Ambiguous problem")
  lcd.print(LEFT_ARROW); // @suppress("Ambiguous problem")
  lcd.print(" yellow blue "); // @suppress("Ambiguous problem")
  lcd.print(RIGHT_ARROW); // @suppress("Ambiguous problem")
  delay(1500);
  menu();
}

/*
 * Método que genera la secuencia de 20 pulsaciones para el modo clásico
 * VERDE = 0; ROJO = 1; AZUL = 2; AMARILLO=3
 */
void ramdomSecuenceGenerator() {
  secuence = (int*) malloc(sizeof(int) * 20);
  for (int i = 0; i < 20; i++) {
    secuence[i] = rand() % 4;
    Serial.print(i);
    Serial.print(" = ");
    Serial.print(secuence[i]);
    Serial.print("; ");
  }
  Serial.println("");
}

void freeSecuence() {
  free(secuence);
}

/*
 * Inicialización pines y componentes
 */
void initArduino() {
  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_YELLOW, INPUT_PULLUP);
  pinMode(BUTTON_WHITE, INPUT_PULLUP);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_WHITE, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  Serial.begin(9600);

  //Indicar a la libreria que tenemos conectada una pantalla de 16x2
  lcd.begin(16, 2);

  //Se crean los carácteres personalizados NO incluidos en el charset del lcd
  lcd.createChar(1, check);
  lcd.createChar(2, cross);
  lcd.createChar(3, skull);
  lcd.createChar(4, up_arrow);
  lcd.createChar(5, down_arrow);
  //Se inicializa la matriz de correspondencia entre el color del led y el pin
  colorLedMatrix[0] = 2;
  colorLedMatrix[1] = 9;
  colorLedMatrix[2] = 12;
  colorLedMatrix[3] = 4;

  LoadMenuOptions();
}

/*Método que inicializa el estado STANBY*/
void initSTANBY() {
  if (stanbyChrono.hasPassed(1000)) {
    lcd.setCursor(0, 1);
    lcd.print(BLANK_LINE); // @suppress("Ambiguous problem")
    if (whiteLedState == ON) {
      digitalWrite(LED_WHITE, LOW);
      whiteLedState = OFF;
      lcd.setCursor(center("Short-New game"), 1);
      lcd.print("Short"); // @suppress("Ambiguous problem")
      lcd.print(RIGHT_ARROW); // @suppress("Ambiguous problem")
      lcd.print("New game"); // @suppress("Ambiguous problem")
    } else {
      digitalWrite(LED_WHITE, HIGH);
      whiteLedState = ON;
      lcd.setCursor(center("Long-Menu"), 1);
      lcd.print("Long"); // @suppress("Ambiguous problem")
      lcd.print(RIGHT_ARROW); // @suppress("Ambiguous problem")
      lcd.print("Menu"); // @suppress("Ambiguous problem")
    }
    stanbyChrono.restart();
  }
}

/*
 * MÉTODOS PARA PINTAR LCD
 */

void welcomeLCD() {
  //Mover el cursor a la primera posición de la pantalla (0, 0)
  lcd.clear();
  lcd.setCursor(center("SIMONDUINO"), 0);
  lcd.print("SIMONDUINO"); // @suppress("Ambiguous problem")
  lcd.setCursor(center("URJC-2019-SEyTR"), 1);
  lcd.print("URJC-2019-SEyTR)"); // @suppress("Ambiguous problem")
}

void initArduinoTurnLCD() {
  lcd.clear();
  lcd.setCursor(center("Level    "), 0);
  lcd.print("Level "); // @suppress("Ambiguous problem")
  lcd.print(currentLevel); // @suppress("Ambiguous problem")
  lcd.setCursor(center("Simonduino turn"), 1);
  lcd.print("Simonduino turn"); // @suppress("Ambiguous problem")
}

void initPlayerTurnLCD() {
  lcd.clear();
  lcd.setCursor(center("Level    "), 0);
  lcd.print("Level "); // @suppress("Ambiguous problem")
  lcd.print(currentLevel); // @suppress("Ambiguous problem")
  lcd.setCursor(center("Player turn"), 1);
  lcd.print("Player turn"); // @suppress("Ambiguous problem")
}

void initStandByLCD() {
  lcd.setCursor(center("Press white button!"), 0);
  lcd.print("Press white button!"); // @suppress("Ambiguous problem")
}

void initLevelUpLCD(){
  if(currentLevel<10){
    lcd.setCursor(center("Start level X"),0);
  }else{
    lcd.setCursor(center("Start level XX"),0);
  }
  lcd.print("Start level ");  // @suppress("Ambiguous problem")
  lcd.print(currentLevel);  // @suppress("Ambiguous problem")
  lcd.setCursor(center("X yellow blue X"),1);
  lcd.write(4); // @suppress("Ambiguous problem")
  lcd.print(" yellow blue "); // @suppress("Ambiguous problem")
  lcd.write(5); // @suppress("Ambiguous problem")
}

void calculateTimes() {
  if ((currentLevel - 1) % 5 == 0 && currentLevel < 20) {
    currentTimeOn = currentTimeOn / 2;
    currentTimeOff = currentTimeOff / 2;
    currentTimeSound = currentTimeOff;
  }
}

void changeState(int previousState, int nextState) {
  if (previousState == -1) {
    initStandByLCD();
    currentSimonduinoState = STANDBY;
    return;
  }
  if (previousState == STANDBY) {
    if (nextState == ARDUINO_TURN) {
      currentLevel = 1;
      index = 0;
      currentTimeOn = MAXTIMEON;
      currentTimeOff = MAXTIMEOFF;
      currentTimeSound = MAXTIMEON;
      ramdomSecuenceGenerator();
      initArduinoTurnLCD();
      currentSimonduinoState = ARDUINO_TURN;
      return;
    }
    if (nextState == INSIDE_MENU) {
      currentOptionMenu = 0;
      menu();
      currentSimonduinoState = INSIDE_MENU;
    }
  }
  if (previousState == ARDUINO_TURN) {
    //ESTADO MENU, PLAYER_TURN
    if (nextState == PLAYER_TURN) {
      initPlayerTurnLCD();
      currentSimonduinoState = PLAYER_TURN;
    }
  }
  if (previousState == PLAYER_TURN) {
    if (nextState == ARDUINO_TURN) {
      currentLevel++;
      index = 0;
      initArduinoTurnLCD();
      calculateTimes();
      currentSimonduinoState = ARDUINO_TURN;
      return;
    }
    if (nextState == STANDBY) {
      whiteLedState = OFF;
      stanbyChrono.restart();
      lcd.clear();
      initStandByLCD();
      freeSecuence();
      currentSimonduinoState = STANDBY;

    }
  }
  if (previousState == WAITING_ACTION) {
    if (nextState == ARDUINO_TURN) {
      lcd.clear();
      initArduinoTurnLCD();
      index = 0;
      simonTurnLedState = OFF;
      currentSimonduinoState = ARDUINO_TURN;
      return;
    }
    if (nextState == STANDBY) {
      lcd.clear();
      initStandByLCD();
      freeSecuence();
      currentSimonduinoState = STANDBY;
      return;
    }
  }

  if (previousState == INSIDE_MENU) {
    if (nextState == STANDBY) {
      lcd.clear();
      initStandByLCD();
      freeSecuence();
      currentSimonduinoState = STANDBY;
      return;
    }
    if (nextState == LEVELUP_OPTION) {
      lcd.clear();
      initLevelUpLCD();
    }
  }

}

/*
 * Método que restablece la partida por gameOver
 */
void gameOver() {
  lcd.clear();
  lcd.setCursor(center("GAME OVER"), 0);
  lcd.print("GAME OVER"); // @suppress("Ambiguous problem")
  lcd.setCursor(center("XXX"), 1);
  lcd.write(3); // @suppress("Ambiguous problem")
  lcd.write(3); // @suppress("Ambiguous problem")
  lcd.write(3); // @suppress("Ambiguous problem")
  tone(BUZZER, 1000, 500);
  delay(2000);
  changeState(PLAYER_TURN, STANDBY);
}

/*
 * Método de ganar
 */
void win() {
  lcd.clear();
  lcd.setCursor(center("YOU WIN!"), 0);
  lcd.print("YOU WIN!"); // @suppress("Ambiguous problem")
  //win music
  tone(BUZZER, 500, 200);
  delay(200);
  tone(BUZZER, 500, 200);
  delay(200);
  tone(BUZZER, 500, 200);
  delay(200);
  tone(BUZZER, 800, 150);
  delay(150);
  tone(BUZZER, 500, 500);
  delay(500);
  tone(BUZZER, 600, 1000);
  delay(2000);
  changeState(PLAYER_TURN, STANDBY);
}

void arduinoTurn() {
  if (index < currentLevel) {
    if (buttonTurnOffChrono.hasPassed(currentTimeOn)
        && simonTurnLedState == ON) {
      //Se apaga el botón que esta encendido
      digitalWrite(colorLedMatrix[secuence[index]], LOW);
      //Se incrementa el índice
      index++;
      simonTurnLedState = OFF;
      buttonTurnOnChrono.restart();
    }
    if (buttonTurnOnChrono.hasPassed(currentTimeOff)
        && simonTurnLedState == OFF) {
      //Ilumino boton y tono
      switch (secuence[index]) {
      case GREEN:
        tone(BUZZER, 466, currentTimeSound);
        digitalWrite(LED_GREEN, HIGH);
        Serial.println("VERDE ENCENCIDO");
        break;
      case RED:
        tone(BUZZER, 349, currentTimeSound);
        digitalWrite(LED_RED, HIGH);
        Serial.println("ROJO ENCENCIDO");
        break;
      case BLUE:
        tone(BUZZER, 233, currentTimeSound);
        digitalWrite(LED_BLUE, HIGH);
        Serial.println("AZUL ENCENCIDO");
        break;
      case YELLOW:
        tone(BUZZER, 277, currentTimeSound);
        digitalWrite(LED_YELLOW, HIGH);
        Serial.println("AMARILLO ENCENCIDO");
        break;
      }
      //Se marca el led como encendido
      simonTurnLedState = ON;
      //Se hace reinicia el cronometro
      buttonTurnOffChrono.restart();
    }
  } else {
    index = 0;
    changeState(ARDUINO_TURN, PLAYER_TURN);
  }
}

//HAY QUE CALIBRARLO
void midi() {
  int calibracion = 1.4;
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 277, 61.0465116279 * calibracion);
  delay(67.8294573643 * calibracion);
  delay(87.2093023256 * calibracion);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 277, 26.1627906977 * calibracion);
  delay(29.0697674419 * calibracion);
  delay(29.0697674419 * calibracion);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, HIGH);
  tone(BUZZER, 277, 52.3255813953 * calibracion);
  delay(58.1395348837 * calibracion);
  delay(38.7596899225 * calibracion);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  tone(BUZZER, 277, 61.0465116279 * calibracion);
  delay(67.8294573643 * calibracion);
  delay(77.519379845 * calibracion);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 277, 61.0465116279 * calibracion);
  delay(67.8294573643 * calibracion);
  delay(87.2093023256 * calibracion);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  tone(BUZZER, 277, 348.837209302 * calibracion);
  delay(387.596899225 * calibracion);
  delay(96.8992248062 * calibracion);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 277, 61.0465116279 * calibracion);
  delay(67.8294573643 * calibracion);
  delay(87.2093023256 * calibracion);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, HIGH);
  tone(BUZZER, 311, 69.7674418605 * calibracion);
  delay(77.519379845 * calibracion);
  delay(77.519379845 * calibracion);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 311, 61.0465116279 * calibracion);
  delay(67.8294573643 * calibracion);
  delay(9.68992248062 * calibracion);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  tone(BUZZER, 311, 78.488372093 * calibracion);
  delay(87.2093023256 * calibracion);
  delay(67.8294573643 * calibracion);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 311, 52.3255813953 * calibracion);
  delay(58.1395348837 * calibracion);
  delay(19.3798449612 * calibracion);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 311, 226.744186047 * calibracion);
  delay(251.937984496 * calibracion);
  delay(58.1395348837 * calibracion);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 207, 218.023255814 * calibracion);
  delay(242.248062016 * calibracion);
  delay(9.68992248062 * calibracion);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 233, 43.6046511628 * calibracion);
  delay(48.4496124031);
  delay(9.68992248062);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  tone(BUZZER, 277, 130.813953488 * calibracion);
  delay(145.348837209 * calibracion);
  delay(9.68992248062 * calibracion);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_BLUE, HIGH);
  tone(BUZZER, 349, 209.302325581 * calibracion);
  delay(232.558139535 * calibracion);
  delay(9.68992248062 * calibracion);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 554, 69.7674418605 * calibracion);
  delay(77.519379845 * calibracion);
  delay(67.8294573643 * calibracion);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 698, 69.7674418605 * calibracion);
  delay(77.519379845 * calibracion);
  delay(9.68992248062 * calibracion);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);
  tone(BUZZER, 698, 244.186046512 * calibracion);
  delay(271.317829457 * calibracion);
  digitalWrite(LED_BLUE, LOW);

}

void setup() {
  initArduino();
  welcomeLCD();
  midi();
  delay(1000);
  lcd.clear();
  //El estado de simonduino a 0 porque empezamos por el menú
  changeState(-1, STANDBY);
  //Después del delay empieza el Menú (ESTADO = 0)
}

void loop() {
  if (currentSimonduinoState == STANDBY) {
    initSTANBY();
  }

  if (digitalRead(BUTTON_WHITE) == PRESSED) { // BLANCO
    if (buttonWhiteLastState == RELEASED) {
      digitalWrite(LED_WHITE, HIGH);
      buttonWhiteLastState = PRESSED;
      Serial.println("BLANCO PRESIONADO");
      buttonWhiteChrono.restart();
      delay(1);
    }
  } else if (buttonWhiteLastState == PRESSED) {
    digitalWrite(LED_WHITE, LOW);
    buttonWhiteLastState = RELEASED;
    if (buttonWhiteChrono.hasPassed(500)
        && currentSimonduinoState == STANDBY) {
      //Vamos al menú
      changeState(STANDBY, INSIDE_MENU);
      Serial.println("BLANCO LIBERADO LARGO");
    } else {
      Serial.println("BLANCO LIBERADO CORTO");
      if (currentSimonduinoState == STANDBY) {
        changeState(STANDBY, ARDUINO_TURN);
      } else if (currentSimonduinoState == INSIDE_MENU) {
        menuHelp();
      } else if (currentSimonduinoState == ARDUINO_TURN
          || currentSimonduinoState == PLAYER_TURN) {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
        digitalWrite(LED_YELLOW, LOW);
        lcd.clear();
        lcd.setCursor(center("Restart game?"), 0);
        lcd.print("Restart game?"); // @suppress("Ambiguous problem")
        currentSimonduinoState = WAITING_ACTION;
      }
    }
  }

  if (currentSimonduinoState == ARDUINO_TURN) {
    arduinoTurn();
    //Serial.println("ARDUINO_TURN");
  } else if (currentSimonduinoState == INSIDE_MENU
      || (currentSimonduinoState == PLAYER_TURN && index < currentLevel)
      || currentSimonduinoState == WAITING_ACTION) {

    if (digitalRead(BUTTON_RED) == PRESSED) { //ROJO
      if (buttonRedLastState == RELEASED) {
        tone(BUZZER, 349, 250);
        digitalWrite(LED_RED, HIGH);
        buttonRedLastState = PRESSED;
        delay(1);
      }
    } else if (buttonRedLastState == PRESSED) {
      digitalWrite(LED_RED, LOW);
      buttonRedLastState = RELEASED;
      Serial.println("ROJO");
      switch (currentSimonduinoState) {
      case INSIDE_MENU:
        changeState(INSIDE_MENU, STANDBY);
        break;
      case PLAYER_TURN:
        if (secuence[index] != RED) {
          gameOver();
        } else {
          index++;
          buttonTurnOnChrono.restart();
        }
        break;
      case WAITING_ACTION:
        changeState(WAITING_ACTION, ARDUINO_TURN);
        break;
      }
    }

    if (digitalRead(BUTTON_GREEN) == PRESSED) { //VERDE
      if (buttonGreenLastState == RELEASED) {
        tone(BUZZER, 466, 250);
        digitalWrite(LED_GREEN, HIGH);
        buttonGreenLastState = PRESSED;
        delay(1);
      }
    } else if (buttonGreenLastState == PRESSED) {
      digitalWrite(LED_GREEN, LOW);
      buttonGreenLastState = RELEASED;
      Serial.println("VERDE");
      switch (currentSimonduinoState) {
      case INSIDE_MENU:
        changeState(INSIDE_MENU, LEVELUP_OPTION);
        break;
      case PLAYER_TURN:
        if (secuence[index] != GREEN) {
          gameOver();
        } else {
          index++;
          buttonTurnOnChrono.restart();
        }
        break;
      case WAITING_ACTION:
        changeState(WAITING_ACTION, STANDBY);
        break;
      }
    }

    if (digitalRead(BUTTON_BLUE) == PRESSED) { //AZUL
      if (buttonBlueLastState == RELEASED) {
        tone(BUZZER, 233, 250);
        digitalWrite(LED_BLUE, HIGH);
        buttonBlueLastState = PRESSED;
        delay(1);
      }
    } else if (buttonBlueLastState == PRESSED) {
      digitalWrite(LED_BLUE, LOW);
      buttonBlueLastState = RELEASED;
      Serial.println("AZUL");
      switch (currentSimonduinoState) {
      case INSIDE_MENU:
        break;
      case PLAYER_TURN:
        if (secuence[index] != BLUE) {
          gameOver();
        } else {
          index++;
          buttonTurnOnChrono.restart();
        }
        break;
      }
    }

    if (digitalRead(BUTTON_YELLOW) == PRESSED) { //AMARILLO
      if (buttonYellowLastState == RELEASED) {
        tone(BUZZER, 277, 250);
        digitalWrite(LED_YELLOW, HIGH);
        buttonYellowLastState = PRESSED;
        delay(1);
      }
    } else if (buttonYellowLastState == PRESSED) {
      digitalWrite(LED_YELLOW, LOW);
      buttonYellowLastState = RELEASED;
      Serial.println("AMARILLO");
      switch (currentSimonduinoState) {
      case INSIDE_MENU:
        break;
      case PLAYER_TURN:
        if (secuence[index] != YELLOW) {
          gameOver();
        } else {
          index++;
          buttonTurnOnChrono.restart();
        }
        break;
      }
    }

  } else if (currentSimonduinoState == PLAYER_TURN && index == currentLevel) {
    //El juegador ha completado la secuencia miramos si ha ganado o debemos subir de nivel
    if (buttonTurnOnChrono.hasPassed(1500)) {
      if (currentLevel == maxLevel) {
        win();
      } else {
        //Subimos de nivel
        changeState(PLAYER_TURN, ARDUINO_TURN);
      }
    }
  }
}
