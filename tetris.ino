#include <MatrixMathInt.h> // https://github.com/ceritium/MatrixMathInt
#include "LedControl.h"

/*
 * TODO:  
 * limites
 * realtime
 * 
 * 
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
#define PIEZED (3)
#define CENTER (3)

LedControl lc=LedControl(12,11,10,SCREENS);

/* we always wait a bit between updates of the display */
unsigned long delaytime=500;
const long interval = 1000; 
unsigned long previousMillis = 0;        // will store last time LED was updated


int world[ROWS][COLS] = {
//{0,0,0,0,0,0,0,0},
//{0,0,0,0,0,0,0,0},
//{0,0,0,0,0,0,0,0},
//{0,0,0,0,0,0,0,0},
//{0,0,0,0,0,0,0,0},
//{0,0,0,0,0,0,0,0},
//{0,0,0,0,0,0,0,0},
//{0,0,0,0,0,0,0,0},
//{1,1,1,1,0,1,1,1},
  };

int piezes[4][3][3] = {
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
    {1,0,0},
    {1,1,0},
    {0,1,0},
  },
};
  
int pieze[3][3] = {};
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
    lc.setIntensity(address,10);
    /* and clear the display */
    lc.clearDisplay(address);
  }
  Serial.println("starting....");
  generatePieze();
}

void loop() { 
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;


    if (collision(world, pieze, ROWS, COLS, PIEZED, PIEZED, piezeY, piezeX)){
      mergeMatrix(world, pieze, ROWS, COLS, PIEZED, PIEZED, piezeY, piezeX);
      piezeY = 0;
      piezeX = CENTER;
      generatePieze();
    } else {
      piezeY++;
    }
  
    checkLine();
  }
  int view[ROWS][COLS] = {};
  Matrix.Copy((int*)world, ROWS, COLS,(int*)view);

  mergeMatrix(view, pieze, ROWS, COLS, PIEZED, PIEZED, piezeY, piezeX);
  render(view);
  actions();
}

void generatePieze(){
  int randNumber = random(4);
  Matrix.Copy((int*)piezes[randNumber], 3, 3,(int*)pieze);
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
  Serial.println(screen);
  Serial.println(row);
  for(int i=0;i<10;i++) {
    lc.setColumn(screen, row, B11111111);
    delay(10);
    lc.setColumn(screen, row, B00000000);
    delay(10);
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
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte);
    if (incomingByte == 97){ // a
        piezeX--;
     }
     if (incomingByte == 100){ //d
        piezeX++;
     }
     if (incomingByte == 115){
        piezeY++;
     }
     if (incomingByte == 119){
        rotatePieze();
      }
  }
}

void rotatePieze(){
  int newPieze[3][3] = {};
  Matrix.Transpose((int*)pieze,PIEZED, PIEZED, (int*)newPieze);
  for(int row=0;row<PIEZED;row++) {
    for(int col=0;col<PIEZED;col++) {
      pieze[row][PIEZED-col-1] = newPieze[row][col];
    }
  }
  if(pieze[0][0] + pieze[1][0] + pieze[2][0] == 0){
    for(int row=0;row<PIEZED;row++) {
      for(int col=1;col<PIEZED;col++) {
        pieze[row][col-1] = pieze[row][col];
        
      }
      pieze[row][2] = 0;
    }
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
  int mergeX = 0;
  int mergeY = 0;
  for(int row=0;row<bM;row++) {
    for(int col=0;col<bN;col++) {
      if(col>= pX && mergeX < PIEZED && row >= pY && mergeY < PIEZED ){
        int val = small[mergeY][mergeX];        
        int small = val;
        if (val == 0) {
          val = big[row][col];       
        }
        big[row][col] = val; 
        if(col>= piezeX && mergeX < PIEZED){
          mergeX++;
        }
      }
    }

    mergeX = 0;
    mergeX = 0;
    if( row >= pY && mergeY < PIEZED){
      mergeY++;
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

