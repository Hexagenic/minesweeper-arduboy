#include <Arduboy2.h>
#include <ArduboyPlaytune.h>

Arduboy2 arduboy;
ArduboyPlaytune tunes(arduboy.audio.enabled);

const unsigned char PROGMEM sprites[] =
{
// hidden
0xff, 0x55, 0xab, 0x55, 0xab, 0x55, 0xab, 0x55, 
0xab, 0x55, 0xab, 0x55, 0xab, 0x55, 0xab, 0x00, 
0x7f, 0x55, 0x2a, 0x55, 0x2a, 0x55, 0x2a, 0x55, 
0x2a, 0x55, 0x2a, 0x55, 0x2a, 0x55, 0x2a, 0x00, 
// revealed
0x00, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 
0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 
0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
// mine
0x00, 0xfe, 0x7e, 0x66, 0x06, 0x0e, 0x0e, 0x02, 
0x02, 0x0e, 0x0e, 0x06, 0x66, 0x7e, 0xfe, 0xfe, 
0x00, 0xff, 0xfe, 0xe6, 0xe0, 0xf0, 0xf0, 0xc0, 
0xc0, 0xf0, 0xf0, 0xe0, 0xe6, 0xfe, 0xff, 0xff, 
// flag
0xff, 0x55, 0xab, 0x55, 0xff, 0x05, 0x07, 0x05, 
0x07, 0x05, 0x07, 0x05, 0xff, 0x55, 0xab, 0x00, 
0x7f, 0x55, 0x2a, 0x55, 0x3f, 0x60, 0x3f, 0x55, 
0x2b, 0x55, 0x2b, 0x55, 0x2b, 0x55, 0x2a, 0x00,
};

const byte winScore[] PROGMEM = {
  0x90, 60, 
  0, 255, 
  0x90, 64, 
  0, 255, 
  0x90, 67, 
  0, 255, 
  0x80, 
  0xF0
};

const byte loseScore[] PROGMEM = {
  0x90, 57, 
  0, 255, 
  0x90, 53, 
  0, 255, 
  0x90, 50, 
  0, 255, 
  0x80, 
  0xF0
};


const signed char IS_MINE = -1;
const unsigned char IS_HIDDEN = 0;
const unsigned char IS_REVEALED = 1;
const unsigned char IS_FLAGGED = 2;

const unsigned char STATE_PLAYING = 0;
const unsigned char STATE_WINNING = 1;
const unsigned char STATE_LOSING = 2;

// State
unsigned char gameState = STATE_PLAYING;
signed char gridCount[9][9] = {0};
unsigned char gridStatus[9][9] = {0};

int offsetX = 0;
int offsetY = 0;
int markerX = 0;
int markerY = 0;
bool firstClick = true;
uint16_t pauseEnded = 0;

void setup() {
  arduboy.begin();

  tunes.initChannel(PIN_SPEAKER_1);
  tunes.initChannel(PIN_SPEAKER_2);
  
  arduboy.clear();
}

const unsigned char* getSprite(int x, int y) {
  if (gridStatus[x][y] == IS_FLAGGED) {
    return sprites + 32 * 3;
  } else if (gridStatus[x][y] == IS_HIDDEN) {
    return sprites + 32 * 0;
  } else if (gridCount[x][y] == IS_MINE) {
    return sprites + 32 * 2;
  }
  return sprites + 32 * 1;
}

void resetGame() {
  gameState = STATE_PLAYING;
  memset(gridCount, 0, sizeof(gridCount));
  memset(gridStatus, 0, sizeof(gridStatus));
  firstClick = true;
  
  offsetX = 0;
  offsetY = 0;
  markerX = 0;
  markerY = 0;

  pauseEnded = arduboy.frameCount + 2;
}

void moveMarker() {
  if(arduboy.justPressed(LEFT_BUTTON))  markerX--;
  if(arduboy.justPressed(RIGHT_BUTTON)) markerX++;
  if(arduboy.justPressed(UP_BUTTON))    markerY--;
  if(arduboy.justPressed(DOWN_BUTTON))  markerY++;

  // clamp cursor to grid
  if (markerX < 0)  markerX = 0;
  if (markerY < 0)  markerY = 0;
  if (markerX >= 9) markerX = 8;
  if (markerY >= 9) markerY = 8;
}

bool isOutside(int x, int y) {
  return x < 0 || y < 0 || x >= 9 || y>= 9;
}

void revealNeighbours(int x, int y) {
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      int x2 = x + i;
      int y2 = y + j;
      
      if (i == 0 && j == 0 || isOutside(x2, y2)) continue;

      if (gridStatus[x2][y2] == IS_HIDDEN && gridCount[x2][y2] != IS_MINE) {
        gridStatus[x2][y2] = IS_REVEALED;

        if (gridCount[x2][y2] == 0) {
          revealNeighbours(x2, y2);
        }
      }
    }
  }
}

void addMineCountAround(int x, int y) {
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      int x2 = x + i;
      int y2 = y + j;
      
      if (i == 0 && j == 0 || isOutside(x2, y2) || gridCount[x2][y2] == IS_MINE) continue;

      gridCount[x2][y2]++;
    }
  }
}

void spawnMines() {
  int mines = 0;

  while (mines < 10) {
    int x = random(0, 9);
    int y = random(0, 9);
    
    if ((x == markerX && y == markerY) || gridCount[x][y] == IS_MINE) {
      continue;
    }

    gridCount[x][y] = IS_MINE;
    mines++;

    addMineCountAround(x, y);
  }
}

int countHidden() {
  int count = 0;
  for (int x = 0; x < 9; x++) {
    for (int y = 0; y < 9; y++) {
      if (gridStatus[x][y] != IS_REVEALED) {
        count++;
      }
    }
  }
  return count;
}

void clickA() {
  if (firstClick) {
    firstClick = false;
    arduboy.initRandomSeed();
    spawnMines(); 
  }

  if (gridStatus[markerX][markerY] == IS_HIDDEN) {
    gridStatus[markerX][markerY] = IS_REVEALED;

    if (gridCount[markerX][markerY] == IS_MINE) {
      gameState = STATE_LOSING;
      pauseEnded = arduboy.frameCount + 60 * 2;
      tunes.playScore(loseScore);
      return;
    }
    
    if (gridCount[markerX][markerY] == 0) {
      revealNeighbours(markerX, markerY);
    }

    if (countHidden() == 10) {
      gameState = STATE_WINNING;
      pauseEnded = arduboy.frameCount + 60 * 2;
      tunes.playScore(winScore);
      return;
    }
  }
}

void clickB() {
  if (gridStatus[markerX][markerY] == IS_HIDDEN) gridStatus[markerX][markerY] = IS_FLAGGED;
  else if (gridStatus[markerX][markerY] == IS_FLAGGED) gridStatus[markerX][markerY] = IS_HIDDEN;
}

void gameLoop() {
  moveMarker();

  if(arduboy.justReleased(A_BUTTON)) {
    clickA();
  }
  else if(arduboy.justReleased(B_BUTTON)) {
    clickB();
  }

  arduboy.clear();

  arduboy.setTextColor(BLACK);
  arduboy.setTextBackground(WHITE);

  if (offsetX + markerX*16 + 15 >= 128) offsetX--;
  if (offsetX + markerX*16 < 0) offsetX++;
  if (offsetY + markerY*16 + 15 >= 64) offsetY--;
  if (offsetY + markerY*16 < 0) offsetY++;

  for (int x = 0; x < 9; x++) {
    for (int y = 0; y < 9; y++) {
      int pixelX = offsetX + x*16;
      int pixelY = offsetY + y*16;
      
      if (pixelX + 16 < 0 || pixelX >= 128 || pixelY + 16 < 0 || pixelY >= 64) {
        continue;
      }
      
      int spriteNo = (x + y) % 4;
      arduboy.drawBitmap(pixelX, pixelY, getSprite(x,y), 16, 16, WHITE);

      if (gridStatus[x][y] == IS_REVEALED && gridCount[x][y] > 0) {
        arduboy.setCursor(pixelX + 6, pixelY + 5);
        arduboy.print(gridCount[x][y]);
      }
    }
  }

  arduboy.drawRect(offsetX + markerX*16, offsetY + markerY*16, 16, 16, WHITE);
  arduboy.drawRect(offsetX + (markerX*16) + 1, offsetY + (markerY*16) + 1, 14, 14, BLACK);
  
  arduboy.display();
}

void winningLoop() {
  if(arduboy.justReleased(A_BUTTON)) resetGame();
  
  arduboy.clear();

  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);

  arduboy.setCursor(10, 10);

  arduboy.print("You won!");
  
  arduboy.display();
}

void losingLoop() {
  if(arduboy.justReleased(A_BUTTON)) resetGame();
  
  arduboy.clear();

  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);

  arduboy.setCursor(10, 10);

  arduboy.print("You lost!");
  
  arduboy.display();
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();

  if (arduboy.frameCount < pauseEnded) {
    return;
  }
  
  if (gameState == STATE_PLAYING) {
    gameLoop();
  } else if (gameState == STATE_WINNING) {
    winningLoop();
  } else if (gameState == STATE_LOSING) {
    losingLoop();
  }

}
