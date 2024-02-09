#include <math.h>
#include <LedControl.h>
#include <Timer.h>
#include <TM1637Display.h>
#include <string.h>









const uint8_t digitCodeMap[] = {
  //DpGFEDCBA  Element  Char   7-segment map:
  B00111111,  // 0   "0"          AAA
  B00000110,  // 1   "1"         F   B
  B01011011,  // 2   "2"         F   B
  B01001111,  // 3   "3"          GGG
  B01100110,  // 4   "4"         E   C
  B01101101,  // 5   "5"         E   C
  B01111101,  // 6   "6"          DDD
  B00000111,  // 7   "7"
  B01111111,  // 8   "8"
  B01101111,  // 9   "9"
  B01110111,  // 10  'A'
  B01111100,  // 11  'b'
  B00111001,  // 12  'C'
  B01011110,  // 13  'd'
  B01111001,  // 14  'E'
  B01110001,  // 15  'F'
  B00111101,  // 16  'G'
  B01110110,  // 17  'H'
  B00000110,  // 18  'I'
  B00001110,  // 19  'J'
  B01110110,  // 20  'K'  Same as 'H'
  B00111000,  // 21  'L'
  B00000000,  // 22  'M'  NO DISPLAY
  B01010100,  // 23  'n'
  B00111111,  // 24  'O'
  B01110011,  // 25  'P'
  B01100111,  // 26  'q'
  B01010000,  // 27  'r'
  B01101101,  // 28  'S'
  B01111000,  // 29  't'
  B00111110,  // 30  'U'
  B00111110,  // 31  'V'  Same as 'U'
  B00000000,  // 32  'W'  NO DISPLAY
  B01110110,  // 33  'X'  Same as 'H'
  B01101110,  // 34  'y'
  B01011011,  // 35  'Z'  Same as '2'
  B00000000,  // 36  ' '  BLANK
  B01000000,  // 37  '-'  DASH
  B01110100,  // 38 M LEFT
  B01010110,  // 39 M RIGHT
};



// --------------------------------------------------------------- //
// --------------------------  Utils Variables -------------------------- //
// --------------------------------------------------------------- //
const uint8_t segmentLose[] = {
  digitCodeMap[21],
  digitCodeMap[28],
  digitCodeMap[24],
  digitCodeMap[29]
};

const uint8_t segmentScr[] = {
  digitCodeMap[28],
  digitCodeMap[12],
  digitCodeMap[27],
  0x0
};

const uint8_t games[3][4] = {
  { digitCodeMap[28],
    digitCodeMap[23],
    digitCodeMap[14],
    digitCodeMap[20] },
  { digitCodeMap[28],
    digitCodeMap[18],
    digitCodeMap[38],
    digitCodeMap[39] },
  { digitCodeMap[17],
    digitCodeMap[24],
    digitCodeMap[21],
    digitCodeMap[14] }
};

bool inGame = false;
bool selectGame = true;
bool isSelectDifficulty = true;
int game = 1;

bool lost = false;

bool runSnake = false;
bool runSimon = false;



// there are defined all the pins
struct Pin {
  static const short joystickX = A5;   // joystick X axis pin
  static const short joystickY = A4;   // joystick Y axis pin
  static const short joystickVCC = 1;  // virtual VCC for the joystick (Analog 1) (to make the joystick connectable right next to the arduino nano)
  static const short joystickGND = 2;  // virtual GND for the joystick (Analog 0) (to make the joystick connectable right next to the arduino nano)
  static const short joystickButton = 4;

  static const short potentiometer = A3;  // potentiometer for snake speed control

  static const short CLK = 8;   // clock for LED matrix 
  static const short CS = 9;    // chip-select for LED matrix
  static const short DIN = 10;  // data-in for LED matrix

  static const short TMDIO = 3;
  static const short TMCLK = 2;
};

// LED matrix brightness: between 0(darkest) and 15(brightest)
const short intensity = 8;

// lower = faster message scrolling
const short messageSpeed = 5;

// initial snake length (1...63, recommended 3)
const short initialSnakeLength = 4;


// --------------------------------------------------------------- //
// -------------------- supporting variables --------------------- //
// --------------------------------------------------------------- //

LedControl matrix(Pin::DIN, Pin::CLK, Pin::CS, 1);
TM1637Display tm(Pin::TMCLK, Pin::TMDIO);

int score = 0;
int difficulty = 3;

boolean gameOver = false;
boolean loseBlink = false;
boolean running = false;

Timer calcSnake;
Timer blinkFood;
Timer mainTimer;

struct Point {
  int row = 0, col = 0;
  Point(int row = 0, int col = 0)
    : row(row), col(col) {}
};

struct Coordinate {
  int x = 0, y = 0;
  Coordinate(int x = 0, int y = 0)
    : x(x), y(y) {}
};

bool win = false;

// primary snake head coordinates (snake head), it will be randomly generated
Point snake;

// food is not anywhere yet
Point food(-1, -1);

// construct with default values in case the user turns off the calibration
Coordinate joystickHome(500, 500);

// snake parameters
int snakeLength = initialSnakeLength;  // choosed by the user in the config section
int snakeDirection = 1;                // if it is 0, the snake does not move
int previousDirection = snakeDirection;


// direction constants
const short up = 1;
const short right = 2;
const short down = 3;  // 'down - 2' must be 'up'
const short left = 4;  // 'left - 2' must be 'right'

// threshold where movement of the joystick will be accepted
const int joystickThreshold = 160;



// snake body segments storage
int gameboard[8][8] = {};




// --------------------------------------------------------------- //
// --------------------------  SNAKE functions -------------------------- //
// --------------------------------------------------------------- //
void scanJoystick() {
  double joyX = analogRead(Pin::joystickX);
  double joyY = analogRead(Pin::joystickY);

  joyY < joystickHome.y - joystickThreshold&& joyY < joyX ? snakeDirection = up : 0;
  joyY > joystickHome.y + joystickThreshold&& joyY > joyX ? snakeDirection = down : 0;
  joyX < joystickHome.x - joystickThreshold&& joyX < joyY ? snakeDirection = left : 0;
  joyX > joystickHome.x + joystickThreshold&& joyX > joyY ? snakeDirection = right : 0;

  snakeDirection + 2 == previousDirection ? snakeDirection = previousDirection : snakeDirection;
  snakeDirection - 2 == previousDirection ? snakeDirection = previousDirection : snakeDirection;

  matrix.setLed(0, food.row, food.col, mainTimer.read() % 500 < 150 ? 1 : 0);
}


void InitializeSnake() {
  snake = Point(4, 4);
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      gameboard[row][col] = 0;
      matrix.setLed(0, row, col, 0);
    }
  }
  gameboard[snake.col][snake.row] = 1;
  food.row = -1;
  food.col = -1;
  score = 0;
  snakeLength = initialSnakeLength;
  mainTimer.stop();
  mainTimer.start();
  tm.setBrightness(2);
  matrix.clearDisplay(0);
  running = true;
  gameOver = false;
  Serial.println("snake init");
}

void endBlink(int arr[8][8]) {
  for (int i = 0; i < 5; i++) {
    matrix.clearDisplay(0);
    delay(350);
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        matrix.setLed(0, row, col, arr[row][col] == 0 ? 0 : 1);
      }
    }
    delay(250);
  }
  matrix.clearDisplay(0);
}

void generateFood() {
  if (food.row == -1 || food.col == -1) {
    if (snakeLength >= 64) {
      win = true;
      return;
    }
    do {
      food.row = random(8);
      food.col = random(8);
    } while (gameboard[food.row][food.col] != 0);
  }
}

void fixEdge() {
  snake.col < 0 ? snake.col += 8 : 0;
  snake.col > 7 ? snake.col -= 8 : 0;
  snake.row < 0 ? snake.row += 8 : 0;
  snake.row > 7 ? snake.row -= 8 : 0;
}

void calculateSnake() {
  previousDirection = snakeDirection;
  if (!gameOver) {
    switch (snakeDirection) {
      case up:
        snake.row--;
        fixEdge();
        //gameboard[snake.row][snake.col] += 1;
        break;

      case right:
        snake.col++;
        fixEdge();
        // gameboard[snake.row][snake.col] += 1;
        break;

      case down:
        snake.row++;
        fixEdge();
        //gameboard[snake.row][snake.col] += 1;
        break;

      case left:
        snake.col--;
        fixEdge();
        //gameboard[snake.row][snake.col] += 1;
        break;
    }

    if (snake.row == food.row && snake.col == food.col) {
      food.row = -1;
      snakeLength++;
      score++;
    }

    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (gameboard[row][col] > 0) {
          gameboard[row][col]++;
        }
        if (gameboard[row][col] > snakeLength) {
          gameboard[row][col] = 0;
        }
      }
    }
    gameboard[snake.row][snake.col] += 1;
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        matrix.setLed(0, row, col, gameboard[row][col] == 0 ? 0 : 1);
      }
    }
    if (gameboard[snake.row][snake.col] > 2) {
      loseBlink = true;
      gameOver = true;
      running = false;
      return;
    }
  }
}

void calculateSnaketimed() {
  if (calcSnake.state() != RUNNING) {
    calcSnake.start();
  }
  if (calcSnake.read() > 950 - (difficulty * 100)) {
    calcSnake.stop();
    calculateSnake();
  }
}



void gameMods() {
  if (loseBlink) {
    endBlink(gameboard);
    loseBlink = false;
    lost = true;
    running = false;
  }
  if (isSelectDifficulty) {
    selectDifficulty();
    isSelectDifficulty = false;
  }
}

void selectDifficulty() {
  tm.setSegments(games[game - 1], 4, 0);
  while (analogRead(Pin::joystickY) > 300 && analogRead(Pin::joystickY) < 800) {
    if (analogRead(Pin::joystickX) > 700) {
      difficulty != 9 ? difficulty++ : difficulty = 1;
      delay(250);
    } else if (analogRead(Pin::joystickX) < 300) {
      difficulty != 1 ? difficulty-- : difficulty = 9;
      delay(250);
    }
    displayDifficultyLevel(difficulty);
  }
  matrix.clearDisplay(0);
  if (analogRead(Pin::joystickY) > 300) {
    selectGame = true;
    isSelectDifficulty = false;
    runSnake = false;
  } else if (analogRead(Pin::joystickY) < 800) {
    isSelectDifficulty = false;
    selectGame = false;
    running = false;
  }
}

void snakeLoop() {
  if (!running) {
    Serial.println("inner init");
    InitializeSnake();
  }
  generateFood();         // if there is no food, generate one
  calculateSnaketimed();  // calculates snake parameters
  scanJoystick();         // watches joystick movements & blinks with food
  gameMods();
  tm.setSegments(&digitCodeMap[(score - score % 10) / 10], 1, 0);
  tm.setSegments(&digitCodeMap[score % 10], 1, 1);
  tm.setSegments(0x0, 1, 2);
  tm.setSegments(0x0, 1, 3);
}






// --------------------------------------------------------------- //
// --------------------------  SIMON Variables -------------------------- //
// --------------------------------------------------------------- //

const short Right = 1;
const short Left = 2;
const short Up = 1;
const short Down = 2;

int selectedCornerDisplay;

int lastIndex = 0;

int simonScore = 0;

int simonDifficulty = 1;

bool simonEndBlink = false;

bool gameInitializeSnake = false;
bool gameInitializeSimon = false;

char yossi[100] = { 0 };
char currentlySelected = 1;
// 1  2
// 4  3

bool simonGameOver = false;
bool simonRunning = false;

const uint8_t simonRPT[] = {
  digitCodeMap[27],
  digitCodeMap[25],
  digitCodeMap[29],
  0x00
};

const uint8_t simonLOOK[] = {
  digitCodeMap[21],
  digitCodeMap[24],
  digitCodeMap[24],
  digitCodeMap[20]
};


// --------------------------------------------------------------- //
// --------------------------  SIMON functions -------------------------- //
// --------------------------------------------------------------- //

void initializeSimon() {
  currentlySelected = 1;
}

void simonSelectDifficulty() {
  Serial.println("SIMON SELECTING");
  tm.setSegments(games[game - 1], 4, 0);
  while (analogRead(Pin::joystickY) > 300 && analogRead(Pin::joystickY) < 800) {
    if (analogRead(Pin::joystickX) > 700) {
      simonDifficulty != 9 ? simonDifficulty++ : simonDifficulty = 1;
      delay(250);
    } else if (analogRead(Pin::joystickX) < 300) {
      simonDifficulty != 1 ? simonDifficulty-- : simonDifficulty = 9;
      delay(250);
    }
    displayDifficultyLevel(simonDifficulty);
  }
  matrix.clearDisplay(0);
  if (analogRead(Pin::joystickY) > 300) {
    selectGame = true;
    isSelectDifficulty = false;
    simonRunning = false;
  } else if (analogRead(Pin::joystickY) < 800) {
    isSelectDifficulty = false;
    selectGame = false;
  }
}

void generateSequence() {
  yossi[lastIndex] = random(1, 5);
  lastIndex++;
}

void displaySequence() {
  tm.setSegments(simonLOOK, 4, 0);
  for (char* p = yossi; *p != 0; p++) {
    simonDisplay(*p - 1);
    delay(500);
    matrix.clearDisplay(0);
    delay(750);
  }
  delay(1000);
}

void zeroSimon() {
  int i = 0;
  while (yossi[i] != 0) {
    yossi[i] = 0;
    i++;
  }
}


void simonScanJoystick() {

  double joyX = analogRead(Pin::joystickX);
  double joyY = analogRead(Pin::joystickY);

  if (joyY < joystickHome.y - joystickThreshold) {
    if (currentlySelected == 3) {
      currentlySelected = 2;
    } else if (currentlySelected == 4) {
      currentlySelected = 1;
    }
  } else if (joyY > joystickHome.y + joystickThreshold) {
    if (currentlySelected == 1) {
      currentlySelected = 4;
    } else if (currentlySelected == 2) {
      currentlySelected = 3;
    }
  }

  if (joyX < joystickHome.x - joystickThreshold) {
    if (currentlySelected == 2) {
      currentlySelected = 1;
    } else if (currentlySelected == 3) {
      currentlySelected = 4;
    }
  } else if (joyX > joystickHome.x + joystickThreshold) {
    if (currentlySelected == 1) {
      currentlySelected = 2;
    } else if (currentlySelected == 4) {
      currentlySelected = 3;
    }
  }
}

void awaitAnswer() {
  tm.setSegments(simonRPT, 4, 0);
  for (int i = 0; i < lastIndex; i++) {
    Timer mytimer;
    mytimer.start();
    do {
      simonScanJoystick();
      simonDisplay(currentlySelected - 1);
    } while (mytimer.read() < 1500);
    mytimer.stop();
    if (currentlySelected != yossi[i]) {
      simonGameOver = true;
      simonRunning = false;
      runSimon = false;
      Serial.print(0);
      lost = true;
      return;
    } else {
      simonScore++;
      Serial.println("NICE!");
    }

    matrix.clearDisplay(0);
    delay(500);
  }
}

void simonInit() {
  tm.setBrightness(2, true);
  simonRunning = true;
  simonGameOver = false;
  simonSelectDifficulty();
  simonScore = 0;
  lastIndex = 0;
  zeroSimon();
  //memset(yossi, 0, 1000);
}


void simonGameModes() {
  if (isSelectDifficulty) {
    Serial.println("gamemods");
    simonSelectDifficulty();
    isSelectDifficulty = false;
  }
}

void simonLoop() {
  if (!simonRunning) {
    delay(150);
    simonInit();
  }

  if (simonRunning) {
    generateSequence();
    displaySequence();
    matrix.clearDisplay(0);
    delay(1000);
    awaitAnswer();
    simonGameModes();
    delay(1000);
  }
}













// --------------------------------------------------------------- //
// --------------------------  Hole Variables -------------------------- //
// --------------------------------------------------------------- //

bool holeRunning;
bool runHole;
bool holeGameOver = false;
bool holeLoseBlink;

int holeScore;
int holeGameBoard[8][8] = { 0 };
int holePlayerLocation = 0;
int holePlayerPrevLocation;
Timer holeTimer;
Timer holeGen;
Timer holeChangeLocation;
int holeTime = 500;



// --------------------------------------------------------------- //
// --------------------------  Hole Functions -------------------------- //
// --------------------------------------------------------------- //
void holeScanJoystick() {
  double joyX = analogRead(Pin::joystickX);
  if (joyX < joystickHome.x - joystickThreshold && holeChangeLocation.read() > 150) {
    holePlayerLocation = holePlayerLocation != 0 ? --holePlayerLocation : 7;
    holeChangeLocation.stop();
    holeChangeLocation.start();
  }

  if (joyX > joystickHome.x + joystickThreshold && holeChangeLocation.read() > 150) {
    holePlayerLocation = holePlayerLocation != 7 ? ++holePlayerLocation : 0;
    holeChangeLocation.stop();
    holeChangeLocation.start();
  }
}


void initializeHole() {
  holeRunning = true;
  holeTimer.start();
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      holeGameBoard[row][col] = 0;
      matrix.setLed(0, row, col, 0);
    }
  }
  holeGen.start();
  holeChangeLocation.start();
  holeScore = 0;
  tm.setBrightness(2);
  matrix.clearDisplay(0);
  holeRunning = true;
  holeGameOver = false;
  Serial.println("hole init");
  holePlayerLocation = 5;
  holePlayerPrevLocation = 4;
  holeLoseBlink = false;
}


void holeCalculate() {
  if (!holeGameOver) {

    if (holePlayerPrevLocation != holePlayerLocation) {
      Serial.println("HOLE CHANGE");
      Serial.println(holePlayerPrevLocation);
      Serial.println(holePlayerLocation);

      if (holeGameBoard[7][holePlayerLocation] == 0) {
        holeGameBoard[7][holePlayerLocation] = 1;
        holeGameBoard[7][holePlayerPrevLocation] = 0;
      } else {
        Serial.println("BBBBBBBBBBBBBBBBBBBB");
        holeGameOver = true;
        holeLoseBlink = true;
        holeRunning = false;
        runHole = false;
        lost = true;
        gameOver = true;
        return;
      }
    }

    if (holeTimer.read() > holeTime) {

      for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
          if (row < 7) {
            if (holeGameBoard[row][col] == 1 && holeGameBoard[row + 1][col] > 0) {
              Serial.println("GGGGGGGGGGG");
              holeGameOver = true;
              holeLoseBlink = true;
              holeRunning = false;
              runHole = false;
              lost = true;
              gameOver = true;
              return;
            }
          }
        }
      }



      for (int row = 7; row >= 0; row--) {
        for (int col = 0; col < 8; col++) {
          if (row < 7) {
            if (holeGameBoard[row][col] == 1) {
              holeGameBoard[row][col] = 0;
              holeGameBoard[row + 1][col] += 1;
            }
          } else {
            if (holeGameBoard[row][col] == 1 && col != holePlayerLocation) {
              holeGameBoard[row][col] = 0;
            }
          }
        }
      }
      holeTimer.stop();
      holeTimer.start();
    }

    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        matrix.setLed(0, row, col, holeGameBoard[row][col] == 0 ? 0 : 1);
      }
    }

    if (holeGameBoard[7][holePlayerLocation] > 1) {
      Serial.println("LLLLLLLLLL");
      holeGameOver = true;
      holeLoseBlink = true;
      holeRunning = false;
      runHole = false;
      lost = true;
      gameOver = true;
      return;
    }
  }

  holePlayerPrevLocation = holePlayerLocation;
}





void holeGameMods() {
  if (holeLoseBlink) {
    Serial.println("BLIMNK");
    endBlink(holeGameBoard);
    holeLoseBlink = false;
    lost = true;
    running = false;
  }
  if (isSelectDifficulty) {
    selectDifficulty();
    isSelectDifficulty = false;
  }
}


void holeGenerate() {
  if (holeGen.read() > holeTime + 2000) {
    holeScore++;
    int randomgen = random(0, 7);
    for (int i = 0; i < 8; i++) {
      holeGameBoard[0][i] = i == randomgen ? 0 : 1;
    }
    Serial.println("GEN");
    holeGen.stop();
    holeGen.start();
  }
}






void holeSelectDifficulty() {
  tm.setSegments(games[game - 1], 4, 0);
  while (analogRead(Pin::joystickY) > 300 && analogRead(Pin::joystickY) < 800) {
    if (analogRead(Pin::joystickX) > 700) {
      difficulty != 9 ? difficulty++ : difficulty = 1;
      delay(250);
    } else if (analogRead(Pin::joystickX) < 300) {
      difficulty != 1 ? difficulty-- : difficulty = 9;
      delay(250);
    }
    displayDifficultyLevel(difficulty);
  }
  matrix.clearDisplay(0);
  if (analogRead(Pin::joystickY) > 300) {
    selectGame = true;
    isSelectDifficulty = false;
    runSnake = false;
  } else if (analogRead(Pin::joystickY) < 800) {
    isSelectDifficulty = false;
    selectGame = false;
    running = false;
  }
}

void holeLoop() {
  if (!holeRunning) {
    //Serial.println("inner init hole");
    initializeHole();
  }

  holeGenerate();
  holeCalculate();     // calculates snake parameters
  holeScanJoystick();  // watches joystick movements & blinks with food
  holeGameMods();
  tm.setSegments(&digitCodeMap[(holeScore - holeScore % 10) / 10], 1, 0);
  tm.setSegments(&digitCodeMap[holeScore % 10], 1, 1);
  tm.setSegments(0x0, 1, 2);
  tm.setSegments(0x0, 1, 3);
}




















// --------------------------------------------------------------- //
// --------------------------  Tetris Variables -------------------------- //
// --------------------------------------------------------------- //

bool tetrisRunning;
bool runTetris;
bool tetrisGameOver = false;
bool tetrisLoseBlink;
bool tetrisIsGenerate = true;
bool tetrisClearRow;

bool tetrisLock = false;

double joyY;

int tetrisScore;
int tetrisGameBoard[8][8] = { 0 };
int tetrisPlayerLocation = 5;
int tetrisPlayerPrevLocation;
Timer tetrisTimer;
Timer tetrisGen;
Timer tetrisChangeLocation;
Timer tetrisAfterContact;
int tetrisTime = 500;
int tetrisAfterContactWait = 1000;

int tetrisX;
int tetrisY;


// --------------------------------------------------------------- //
// --------------------------  Tetris Functions -------------------------- //
// --------------------------------------------------------------- //
void tetrisScanJoystick() {
  double joyX = analogRead(Pin::joystickX);
   joyY = analogRead(Pin::joystickY);
  if (joyX < joystickHome.x - joystickThreshold && tetrisChangeLocation.read() > 150) {
    for (int row = 0; row < 8; row++) {
      if (tetrisGameBoard[row][0] > 1) {
        Serial.println("LEFT LIM");
        tetrisLock = true;
      }
    }
    if (!tetrisLock) {
      tetrisPlayerLocation = -1;
      Serial.println("NOT LEFT LIM");
    }
    tetrisLock = false;
    tetrisChangeLocation.stop();
    tetrisChangeLocation.start();
  }

  if (joyX > joystickHome.x + joystickThreshold && tetrisChangeLocation.read() > 150) {
    for (int row = 0; row < 8; row++) {
      if (tetrisGameBoard[row][7] > 1) {
        Serial.println("RIGHT LIM");
        tetrisLock = true;
      }
    }
    if (!tetrisLock) {
      tetrisPlayerLocation = 1;
    }
    tetrisLock = false;
    Serial.println("NOT RIGHT LIM");
    tetrisChangeLocation.stop();
    tetrisChangeLocation.start();
  }
}


void initializeTetris() {
  tetrisRunning = true;
  tetrisTimer.start();
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      tetrisGameBoard[row][col] = 0;
      matrix.setLed(0, row, col, 0);
    }
  }
  tetrisGen.start();
  tetrisChangeLocation.start();
  tetrisScore = 0;
  tm.setBrightness(2);
  matrix.clearDisplay(0);
  tetrisRunning = true;
  tetrisGameOver = false;
  Serial.println("tetris init");
  tetrisPlayerLocation = 5;
  tetrisPlayerPrevLocation = 4;
  tetrisLoseBlink = false;

  //DELETE TEST
  
}


void tetrisCalculate() {

  tetrisClearRow = true;

  if (!tetrisGameOver) {

    //////////////////////////////////////////////////////
    //////////CHECK AND UPDATE PIECE LOCATION////////////
    ////////////////////////////////////////////////////

    

    if (tetrisPlayerLocation == 1) {

      Serial.println("tetris CHANGE");

      for (int row = 0; row < 8; row++) {
        for (int col = 6; col >= 0; col--) {
          if (tetrisGameBoard[row][col] > 1 && tetrisGameBoard[row][col + 1] == 1) {
            Serial.println("RIGHT LIM CALC");
            tetrisLock = true;
          }
        }
      }

      if (!tetrisLock) {
        for (int row = 0; row < 8; row++) {
          for (int col = 6; col >= 0; col--) {
            if (tetrisGameBoard[row][col] > 1) {
              Serial.println("RIGHT MOVE CALC");
              tetrisGameBoard[row][col] = 0;
              tetrisGameBoard[row][col + 1] = 2;
            }
          }
        }
      }
      tetrisPlayerLocation = 0;
      tetrisLock = false;
    }

    if (tetrisPlayerLocation == -1) {

      Serial.println("tetris CHANGE");

      for (int row = 0; row < 8; row++) {
        for (int col = 1; col <= 7; col++) {
          if (tetrisGameBoard[row][col] > 1 && tetrisGameBoard[row][col - 1] == 1) {
            Serial.println("LEFT LIM CALC");
            tetrisLock = true;
          }
        }
      }

      if (!tetrisLock) {
        for (int row = 0; row < 8; row++) {
          for (int col = 1; col <= 7; col++) {
            if (tetrisGameBoard[row][col] > 1) {
              Serial.println("LEFT MOVE CALC");
              tetrisGameBoard[row][col] = 0;
              tetrisGameBoard[row][col - 1] = 2;
            }
          }
        }
      }

      tetrisPlayerLocation = 0;
      tetrisLock = false;
    }




    //////////////////////////////////////////////////////
    ////////////////CHECK AND MOVE DOWN//////////////////
    ////////////////////////////////////////////////////

    if(joyY > 950){
      tetrisTime = 50;
    }

    if (tetrisTimer.read() > tetrisTime) {
        //Serial.println("enterd");

      for (int row = 7; row >= 0; row --) {
        for (int col = 0; col <= 7; col++) {
          if (tetrisGameBoard[row][col] > 1 && tetrisGameBoard[row+1][col] == 1) {
            //Serial.println("DOWN LIM CALC");
            tetrisLock = true;
            if(!tetrisAfterContact.state()){
              tetrisAfterContact.start();
            }
          }
          if(tetrisGameBoard[row][col] > 1 && row == 7){
            Serial.println("DOWN LIM CALC 2");
            tetrisLock = true;
            if(!tetrisAfterContact.state()){
              tetrisAfterContact.start();
            }
          }
        }
      }

      if (!tetrisLock) {
        for (int row = 7; row >= 0; row --) {
          for (int col = 0; col <= 7; col++) {
            if (tetrisGameBoard[row][col] > 1) {
              //Serial.println("DOWN MOVE CALC");
              tetrisGameBoard[row][col] = 0;
              tetrisGameBoard[row+1][col] = 2;
            }
          }
        }
      }else if(tetrisAfterContact.read() > tetrisAfterContactWait){
        for (int row = 0; row < 8; row++) {
          for (int col = 7; col >= 0; col--) {
            if (tetrisGameBoard[row][col] > 1) {
              tetrisGameBoard[row][col] = 1;
            }
          }
        }
        tetrisIsGenerate = true;
        tetrisAfterContact.stop();
        tetrisAfterContact.start();
      }
      //Serial.println(tetrisAfterContact.read());
      tetrisPlayerLocation = 0;
      tetrisLock = false;

      tetrisTimer.stop();
      tetrisTimer.start();
      tetrisTime = 500;
    }

      tetrisPlayerLocation = 0;
      tetrisLock = false;


    //////////////////////////////////////////////////////
    /////////////////CHECK AND CLEAR ROW/////////////////
    ////////////////////////////////////////////////////

    int cleardRows = 0; 

    for(int row = 7; row>=0; row--){
      int emptyCols = 8;
      for(int col = 0; col<8; col++){
        if(tetrisGameBoard[row][col] == 1){
          emptyCols--;
          //Serial.println(emptyCols);
        }
      }

      if(emptyCols == 0){
        cleardRows++;
          Serial.println("EMPT COLS 0");
          for(int coln = 0; coln<8; coln++){
            tetrisGameBoard[row][coln] = 0;
          }
          for(int rown = row == 0? 0 : row-1; rown>=0; rown--){
            Serial.println("CLEAR LAYER SCANNING");
            Serial.println(rown);
            for(int coln = 0; coln<8; coln++){
              if(tetrisGameBoard[rown][coln] == 1){
                Serial.println("CLEAR LAYER MOVING DOWN A ROW");
                Serial.println(rown);
                Serial.println(coln);

                tetrisGameBoard[rown][coln] = 0;
                tetrisGameBoard[rown+1][coln] = 1;
              }
            }
          }
        }

    }



    //////////////////////////////////////////////////////
    //////////////////UPDATE DISPLAY/////////////////////
    ////////////////////////////////////////////////////


    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        matrix.setLed(0, row, col, tetrisGameBoard[row][col] == 0 ? 0 : 1);
      }
    }
  }
  tetrisPlayerPrevLocation = tetrisPlayerLocation;
}





void tetrisGameMods() {
  if (tetrisLoseBlink) {
    Serial.println("BLIMNK");
    endBlink(tetrisGameBoard);
    tetrisLoseBlink = false;
    lost = true;
    running = false;
  }
  if (isSelectDifficulty) {
    selectDifficulty();
    isSelectDifficulty = false;
  }
}


void tetrisGenerate() {
  if (tetrisIsGenerate) {
    tetrisGameBoard[0][4] = 2;
    tetrisGameBoard[0][5] = 2;
    tetrisGameBoard[1][4] = 2;
    tetrisGameBoard[1][5] = 2;
    //Serial.println("GEN");
    tetrisGen.stop();
    tetrisGen.start();
    tetrisIsGenerate = false;
  }
}






void tetrisSelectDifficulty() {
  tm.setSegments(games[game - 1], 4, 0);
  while (analogRead(Pin::joystickY) > 300 && analogRead(Pin::joystickY) < 800) {
    if (analogRead(Pin::joystickX) > 700) {
      difficulty != 9 ? difficulty++ : difficulty = 1;
      delay(250);
    } else if (analogRead(Pin::joystickX) < 300) {
      difficulty != 1 ? difficulty-- : difficulty = 9;
      delay(250);
    }
    displayDifficultyLevel(difficulty);
  }
  matrix.clearDisplay(0);
  if (analogRead(Pin::joystickY) > 300) {
    selectGame = true;
    isSelectDifficulty = false;
    runSnake = false;
  } else if (analogRead(Pin::joystickY) < 800) {
    isSelectDifficulty = false;
    selectGame = false;
    running = false;
  }
}

void tetrisLoop() {
  if (!tetrisRunning) {
    //Serial.println("inner init tetris");
    initializeTetris();
  }

  tetrisGameMods();
  tetrisGenerate();
  tetrisCalculate();     // calculates snake parameters
  tetrisScanJoystick();  // watches joystick movements & blinks with food
  tm.setSegments(&digitCodeMap[(tetrisScore - tetrisScore % 10) / 10], 1, 0);
  tm.setSegments(&digitCodeMap[tetrisScore % 10], 1, 1);
  tm.setSegments(0x0, 1, 2);
  tm.setSegments(0x0, 1, 3);
}


void setup() {
  Serial.begin(4800);         // set the same baud rate on your Serial Monitor
  matrix.shutdown(0, false);  //The MAX72XX is in power-saving mode on startup
  matrix.setIntensity(0, 5);  // Set the brightness to maximum value
  matrix.clearDisplay(0);
  tm.setBrightness(7, true);
  srand(1);
}

void loop() {
  // while (selectGame && analogRead(Pin::joystickY) > 300) {
  //   if (analogRead(Pin::joystickX) > 700) {
  //     game != 3 ? game++ : game = 1;
  //     delay(250);
  //   } else if (analogRead(Pin::joystickX) < 300) {
  //     game != 1 ? game-- : game = 3;
  //     delay(250);
  //   }
  //   displayDifficultyLevel(game);
  //   tm.setSegments(games[game - 1], 4, 0);
  //   isSelectDifficulty = true;
  // }
  // selectGame = false;

  // if (isSelectDifficulty && game == 1) {
  //     Serial.write("G1");
  //   runSnake = true;
  //   runSimon = false;
  //   runHole = false;
  //   matrix.clearDisplay(0);
  //   delay(700);
  // } else if (isSelectDifficulty && game == 2) {
  //   matrix.clearDisplay(0);
  //   runSimon = true;
  //   runSnake = false;
  //   runHole = false;
  //   delay(800);
  // } else if (isSelectDifficulty && game == 3) {
  //   matrix.clearDisplay(0);
  //   runSimon = false;
  //   runSnake = false;
  //   runHole = true;
  //   delay(800);
  // }

  // if (runSnake) {
  //   snakeLoop();
  // }

  // if (runSimon) {
  //   simonLoop();
  // }

  // if (runHole) {
  //   holeLoop();
  // }

  // if (lost) {
  //   switch (game){
  //     case 1:
  //       generalLoseBlink(score);
  //       break;
  //     case 2:
  //       generalLoseBlink(simonScore);
  //       break;
  //     case 3:
  //       generalLoseBlink(holeScore);
  //       break;
  //   }
  //   isSelectDifficulty = true;
  //   delay(750);
  // }
  // lost = false;
  tetrisLoop();
}

//////////////////////////////////////////////////////////////////////////shapes snake//////////////////////////////////////////////////////////////////////////////

const bool digits[][8][8] = {
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 0, 0, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 0, 0 },
    { 0, 0, 1, 0, 1, 1, 0, 0 },
    { 0, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 } }
};

const bool sadFace[8][8] = {
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  },
  {
    0,
    1,
    1,
    0,
    0,
    1,
    1,
    0,
  },
  {
    0,
    1,
    1,
    0,
    0,
    1,
    1,
    0,
  },
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  },
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  },
  {
    0,
    0,
    1,
    1,
    1,
    1,
    0,
    0,
  },
  {
    0,
    1,
    0,
    0,
    0,
    0,
    1,
    0,
  },
  {
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
  }
};


void displayDifficultyLevel(int difficultyLevel) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      matrix.setLed(0, row, col, digits[difficultyLevel][row][col]);
    }
  }
}

void displayBoolArray(bool array[8][8]) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      matrix.setLed(0, row, col, array[row][col]);
    }
  }
}

//////////////////////////////////////////////////////////////////////////shapes simon//////////////////////////////////////////////////////////////////////////////

const bool corners[][8][8] = {
  { { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 } },
  { { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 } }
};


void simonDisplay(int index) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      matrix.setLed(0, row, col, corners[index][row][col]);
    }
  }
}




// --------------------------------------------------------------- //
// --------------------------  Utils functions -------------------------- //
// --------------------------------------------------------------- //

void generalLoseBlink(int totalScore) {
  Timer tempT;
  tempT.start();
  do {
    if (tempT.read() == 0) {
      tm.setSegments(segmentLose, 4, 0);
      displayBoolArray(sadFace);
    }
    if (tempT.read() == 1000) {
      matrix.clearDisplay(0);
      tm.clear();
    }
    if (tempT.read() == 2000) {
      tm.setSegments(segmentScr, 4, 0);
      displayBoolArray(sadFace);
    }
    if (tempT.read() == 3000) {
      matrix.clearDisplay(0);
      tm.clear();
    }
    if (tempT.read() == 4000) {
      tm.setSegments(&digitCodeMap[(totalScore - score % 10) / 10], 1, 0);
      tm.setSegments(&digitCodeMap[totalScore % 10], 1, 1);
      displayBoolArray(sadFace);
    }
    if (tempT.read() == 5000) {
      matrix.clearDisplay(0);
      tm.clear();
    }
    if (tempT.read() == 6000) {
      tempT.stop();
      tempT.start();
    }
  } while (analogRead(Pin::joystickY) > 300);
  delay(750);
}
