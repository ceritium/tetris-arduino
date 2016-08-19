#include <MatrixMathInt.h> // https://github.com/ceritium/MatrixMathInt
#include "LedControl.h"

/*
 * TODO:  
 * - refactor render using cols or rows of LedControl
 * - show points on gameover.
 */

/*
 Now we need a LedControl to work with.
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 But the maximum default of 8 MAX72XX wil also work.
 */

#define SCREENS (2)
#define SCREEN_SIZE (8)
#define COLS (8)
#define ROWS (16)
#define CENTER (3)

LedControl lc=LedControl(12,11,10,SCREENS);

/* we always wait a bit between updates of the display */
unsigned long delaytime=500;
const long interval = 1000; 
unsigned long previousMillis = 0;        // will store last time LED was updated
bool gameOver = false;

int world[ROWS][COLS] = {};

const int pinL = 6;
const int pinU = 5;
const int pinR = 4;
const int pinD = 7;


#define PIEZED (4)
#define PIEZES (7)
int piezes[PIEZES][PIEZED][PIEZED] = {
  {
    {1,0,0},
    {1,0,0},
    {1,0,0},
    {1,0,0},
  },
  {
    {1,0,0},
    {1,1,0},
    {1,0,0},
  },
  {
    {1,1,0},
    {1,1,0},
    {0,0,0},
  },
  {
    {1,0,0},
    {1,0,0},
    {1,1,0},
  },
  {
    {0,1,0},
    {0,1,0},
    {1,1,0},
  },
  {
    {1,0,0},
    {1,1,0},
    {0,1,0},
  },
  {
    {0,1,0},
    {1,1,0},
    {1,0,0},
  },
};
  
int pieze[PIEZED][PIEZED] = {};
int piezeX = CENTER;
int piezeY = 0;
int incomingByte = 0;   // for incoming serial data

 
void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  //we have already set the number of devices when we created the LedControl
  int devices=lc.getDeviceCount();
  //we have to init all devices in a loop
  for(int address=0;address<devices;address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address,false);
    /* Set the brightness to a medium values */
    lc.setIntensity(address,1);
    /* and clear the display */
    lc.clearDisplay(address);
  }
  Serial.println("starting....");
  generatePieze();

  pinMode(pinR, INPUT);
  pinMode(pinL, INPUT);
  pinMode(pinU, INPUT);
  pinMode(pinD, INPUT);
}

void loop() { 
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (collision(world, pieze, ROWS, COLS, PIEZED, PIEZED, piezeY, piezeX)){
      mergeMatrix(world, pieze, ROWS, COLS, PIEZED, PIEZED, piezeY, piezeX);
      if(piezeY == 0){
        gameOver = true;
      }
      piezeY = 0;
      piezeX = CENTER;
      generatePieze();
    } else {
      piezeY++;
    }
  
    checkLine();
  }
  
  if (gameOver == true){
    beginGameOver();  
  } else {
  int view[ROWS][COLS] = {};
  Matrix.Copy((int*)world, ROWS, COLS,(int*)view);

  mergeMatrix(view, pieze, ROWS, COLS, PIEZED, PIEZED, piezeY, piezeX);
  render(view);
  actions();
  }
}

void beginGameOver(){
  Serial.println("gameover" );
  for(int screen=0;screen<SCREENS;screen++){
    for(int row=0;row<SCREEN_SIZE;row++) {
      lc.setColumn(SCREENS-screen-1, row, B11111111);
      delay(100);
    }
  }
  int newWorld[ROWS][COLS] = {};
  Matrix.Copy((int*)newWorld, ROWS, COLS,(int*)world);

  piezeX = CENTER;
  piezeY = -1;
  generatePieze();
  gameOver = false;
  render(world);
}

void generatePieze(){
  int randNumber = random(PIEZES);
  Matrix.Copy((int*)piezes[randNumber], PIEZED, PIEZED,(int*)pieze);
};

void checkLine(){
  for(int row=0;row<ROWS;row++) {
    int count = 0;
    for(int col=0;col<COLS;col++) {
      count = count + world[row][col];
      if (count >= COLS){
        deleteRow(row);        
      }
     }
  } 
}

void deleteRow(int toDelete){
  int screen = (toDelete / SCREEN_SIZE);
  int row = SCREEN_SIZE - (toDelete - (screen * SCREEN_SIZE)) - 1;
  for(int i=0;i<10;i++) {
    lc.setColumn(screen, row, B11111111);
    delay(25);
    lc.setColumn(screen, row, B00000000);
    delay(25);
  }

  for(int row=ROWS;row>0;row--) {
    if(row <= toDelete){
      if(row == 0){
        //world[row] = {};
      } else {
        for(int col=0;col<COLS;col++) {
          world[row][col] = world[row-1][col];        
        }
      }
    }
  }
}

void actions(){
     if (digitalRead(pinR) == HIGH){
      if (validPosition(pieze, piezeY, piezeX+1)){
        piezeX++;
      }
     }

     if (digitalRead(pinL) == HIGH){
      if (validPosition(pieze, piezeY, piezeX-1)){
        piezeX--;
      }
     }

    if (digitalRead(pinD) == HIGH){
      if (validPosition(pieze, piezeY+1, piezeX)){
        piezeY++;
      }
     }


     if (digitalRead(pinU) == HIGH){
         rotatePieze();
     }
  
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
    if (incomingByte == 97){ // a
      if (validPosition(pieze, piezeY, piezeX-1)){
        piezeX--;
      }
     }
     if (incomingByte == 100){ // d
      if (validPosition(pieze, piezeY, piezeX+1)){
        piezeX++;
      }
     }
     if (incomingByte == 115){ // s
      if (validPosition(pieze, piezeY+1, piezeX)){
        piezeY++;
      }
     }
     
     
     if (incomingByte == 119){ // w
        rotatePieze();
      }
  }
}

void rotatePieze(){
  int newPieze[PIEZED][PIEZED] = {};
  int tPieze[PIEZED][PIEZED] = {};
  Matrix.Transpose((int*)pieze,PIEZED, PIEZED, (int*)newPieze);
  for(int row=0;row<PIEZED;row++) {
    for(int col=0;col<PIEZED;col++) {
      tPieze[row][PIEZED-col-1] = newPieze[row][col];
    }
  }
  
  for(int i=0; i<PIEZED; i++){
    if(tPieze[0][0] + tPieze[1][0] + tPieze[2][0] == 0){
      for(int row=0;row<PIEZED;row++) {
        for(int col=1;col<PIEZED;col++) {
          tPieze[row][col-1] = tPieze[row][col];    
        }
        tPieze[row][PIEZED-1] = 0;
      }
    }
  } 

  if (validPosition(tPieze, piezeY, piezeX)){
    Matrix.Copy((int*)tPieze, PIEZED, PIEZED,(int*)pieze);
  }
}

bool validPosition(int mPieze[PIEZED][PIEZED], int destY, int destX){
  bool valid = true;
  int x = 0;
  int y = 0;

  for(int row=destY;row<(PIEZED+destY);row++){
    for(int col=destX;col<(PIEZED+destX);col++){
      if (mPieze[y][x] == 1){
        if(world[row][col] == 1){
          return false;
        }
        if(col>COLS-1){
          return false;
        }
        if(col < 0){
          return false;
        }
        if(row > ROWS-1){
          return false;
        }
      }
      x++;
    }
    x = 0;
    y++;
  }
}

bool collision(int big[ROWS][COLS], int small[PIEZED][PIEZED], int bM, int bN, int sM, int sN, int pY, int pX){
  int mergeX = 0;
  int mergeY = 0;
  for(int row=0;row<bM;row++) {
    for(int col=0;col<bN;col++) {
      if( col>= pX && mergeX < PIEZED && row >= pY && mergeY < PIEZED ){
        if (small[mergeY][mergeX] == 1 && big[row+1][col] == 1) {
          return true;
        }

        if (row == ROWS-1 && small[mergeY][mergeX] == 1){
          return true;
        }
        if( col>= piezeX && mergeX < PIEZED){
          mergeX++;
        }
      }
    }

    mergeX = 0;
    if( row >= pY && mergeY < PIEZED){
      mergeY++;
    }
  }
  return false;
}

void mergeMatrix(int big[ROWS][COLS], int small[PIEZED][PIEZED], int bM, int bN, int sM, int sN, int pY, int pX){
  for(int row=0;row<bM;row++) {
    for(int col=0;col<bN;col++) {
      if (col >= pX && col < pX + PIEZED  && row >= pY && row < pY + PIEZED){
        int val = small[row - pY][col - pX];
        if (val == 0) {
          val = big[row][col];
        }
        big[row][col] = val;
      }
    }
  }
}

void render(int matrix[ROWS][COLS]){
  int row = 0;
        
  for(int screen=0;screen<SCREENS;screen++) {

    int matrox[SCREEN_SIZE][SCREEN_SIZE] = {};    
    for(int srow=0;srow<SCREEN_SIZE;srow++) {
      for(int col=0;col<COLS;col++) {
        matrox[srow][col] = matrix[row][col];
      }
      row++;
    }
  
    int matrex[SCREEN_SIZE][SCREEN_SIZE] = {};    
    Matrix.Transpose((int*)matrox,SCREEN_SIZE, SCREEN_SIZE, (int*)matrex);
    for(int srow=0;srow<SCREEN_SIZE;srow++) {
      for(int col=0;col<COLS;col++) {
        lc.setLed(screen,srow,SCREEN_SIZE-col-1,matrex[srow][col]);
      }
    }
  }
}

