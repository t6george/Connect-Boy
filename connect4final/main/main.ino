#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#include "pitches.h"

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#define TFT_SCLK 13   
#define TFT_MOSI 11 
#define TFT_CS  10  // Chip select line for TFT display
#define TFT_RST  9  // Reset line for TFT (or see below...)
#define TFT_DC   8  // Data/command line for TFT
#define SD_CS    4  // Chip select line for SD card

#define Speaker  A0

#define selectButton 3
#define leftButton  2
#define rightButton 5
#define downButton 7
#define upButton 6

#define BUFFPIXEL 1

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

////////Variables
int depth = 1;
uint16_t colorarray[8]={ST7735_BLACK,ST7735_GREEN,ST7735_BLUE,ST7735_MAGENTA,ST7735_WHITE,ST7735_YELLOW,ST7735_RED};
int whereToStart[2];
int row_pixels[6] = {38,56,73,91,108,125}; //coordinates of each rock and column on display (in pixels)
int col_pixels[7] = {13,30,47,64,80,97,114};
int globalturn = 1;
bool intro = true;
bool ingame = true;
bool endgame = false;
int colorcounter = 0;
int selectButtonState = 0; // variable for reading the selectButton status
int leftButtonState = 0; // variable for reading the leftButton status
int rightButtonState = 0; // variable for reading the leftButton status
int downButtonState = 0; // variable for reading the downButton status
int board[6][7]; //the actual board
int choice = 3; //the column the user is hovering over

/////Graphics Functions
void drawtext(int x,int y,String text, uint16_t color) {
      tft.setCursor(x,y);
      tft.setTextColor(color);
      tft.setTextWrap(true);
      tft.print(text);
    }

void bmpDraw(char *filename, uint8_t x, uint8_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  /*Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');*/

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        
        Serial.print(bmpWidth);
        
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.Color565(r,g,b));
          } // end pixel
        } // end scanline
      } // end goodBmp
    }
  }

  bmpFile.close();
}

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
/*
void drawBoard() {
  Serial.println("");
  for (int i=0; i<6; i++) {
    for (int j=0; j<7;j++) {
      Serial.print(board[i][j]);
      Serial.print(" ");
    }
    Serial.println("");
  }
  Serial.println("");
}
*/


//Connect 4 Functions
 /*
 * toStart1
 * Determies from where to start checking diagonally from top left to bottom right
 * @param row, row number
 * @param col, col number
*/

void(* resetFunc) (void) = 0;

void drawBoard() {
  Serial.println("");
  for (int i=0;i<13;i++) {
    Serial.print("_");
  }
  Serial.println("");
  for (int i=0; i<6; i++) {
    for (int j=0; j<7;j++) {
      Serial.print(board[i][j]);
      Serial.print(" ");
    }
    Serial.println("");
  }
  Serial.println("");
}

void toStart1(int row, int col) { //top left to bottom right
  whereToStart[0] = 0;
  whereToStart[1] = 0;
  int a = col-row;
  if (a>=0) {
    whereToStart[0] = 0;
    whereToStart[1] = a;
  } else {
    whereToStart[0] = -1*a;
    whereToStart[1] = 0;
  }
}
/*
 * toStart2
 * Determies from where to start checking diagonally from top right to bottom left
 * @param row, row number
 * @param col, col number
*/
void toStart2(int row, int col) { //top right to bottom right
  whereToStart[0] = 0;
  whereToStart[1] = 0;
  int a = col+ row;
  if (a>6) {
    whereToStart[0] = a-6;
    whereToStart[1] = 6;
  } else {
    whereToStart[0] = 0;
    whereToStart[1] = a;
  }
}

boolean winCheckNoPrint(int row, int col) {
  int consecCount = 0;
  //hori check
  for (int i = 0; i<6;i++) { // counts consecutive values horizontally in given row
    if (board[row][i] == board[row][i+1] && (board[row][i] ==1 || board[row][i] == 2)) {
      consecCount++;
      if (consecCount==3) {
        return true;
      }
    } else {
      consecCount = 0;
    }
  }
  //verti check
  consecCount = 0;
  for (int i =0; i<5;i++) { // counts consecutive values vertically in the given col
    if (board[i][col] == board[i+1][col] && (board[i][col] ==1 || board[i][col] == 2)) {
      consecCount++;
      if (consecCount==3) {
        return true;
      }
    } else {
      consecCount = 0;
    }
  }
  //diag 1 check
  toStart1(row,col);
  consecCount = 0;
  for (int i=whereToStart[0]; i<5;i++) { //rows are the limiting factor 
    // this if checks consecutive-ness, if the value is a player tile, and that the column number doesnt go out of range
    if (board[i][whereToStart[1]] == board[i+1][whereToStart[1]+1] && (board[i][whereToStart[1]]==1 || board[i][whereToStart[1]]==2) && whereToStart[1]<6) {
      consecCount++;
      if (consecCount==3) {
        return true;
      }
    } else {
      consecCount = 0;
    }
    whereToStart[1]++;
  }
  //diag 2 check
  toStart2(row,col);
  consecCount = 0;
  for (int i=whereToStart[0]; i<5;i++) { //rows are the limiting factor 
    // this if checks consecutive-ness, if the value is a player tile, and that the column number doesnt go out of range
    if (board[i][whereToStart[1]] == board[i+1][whereToStart[1]-1] && (board[i][whereToStart[1]]==1 || board[i][whereToStart[1]]==2) && whereToStart[1] >0) {
      consecCount++;
      if (consecCount==3) {
        return true;
      }
    } else {
      consecCount = 0;
    }
    whereToStart[1]--;
  }
  return false;
}
/////////////////////////////////////////////////////////////////////////////////
boolean winCheck(int row, int col) {
  int consecCount = 0;
  int colorcount = 0;
  //hori check
  for (int i = 0; i<6;i++) { // counts consecutive values horizontally in given row
    if (board[row][i] == board[row][i+1] && (board[row][i] ==1 || board[row][i] == 2)) {
      consecCount++;
      if (consecCount==3) {
        while (colorcount<50){
        
        tft.fillCircle(col_pixels[i+1],row_pixels[row], 6,colorarray[colorcount%4]);
        tft.fillCircle(col_pixels[i],row_pixels[row], 6,colorarray[(colorcount+1)%4]);
        tft.fillCircle(col_pixels[i-1],row_pixels[row], 6,colorarray[(colorcount+2)%4]);
        tft.fillCircle(col_pixels[i-2],row_pixels[row], 6,colorarray[(colorcount+3)%4]);
        delay(25);
        colorcount++;
        }
        tft.fillCircle(col_pixels[i+1],row_pixels[row], 6,colorarray[5+globalturn%2]);
        tft.fillCircle(col_pixels[i],row_pixels[row], 6,colorarray[5+globalturn%2]);
        tft.fillCircle(col_pixels[i-1],row_pixels[row], 6,colorarray[5+globalturn%2]);
        tft.fillCircle(col_pixels[i-2],row_pixels[row], 6,colorarray[5+globalturn%2]);
        return true;
      }
    } else {
      consecCount = 0;
    }
  }
  //verti check
  consecCount = 0;
  for (int i =0; i<5;i++) { // counts consecutive values vertically in the given col
    if (board[i][col] == board[i+1][col] && (board[i][col] ==1 || board[i][col] == 2)) {
      consecCount++;
      if (consecCount==3) {
        while (colorcount<50){
        tft.fillCircle(col_pixels[col],row_pixels[i+1], 6,colorarray[colorcount%4]);
        tft.fillCircle(col_pixels[col],row_pixels[i], 6,colorarray[(colorcount+1)%4]);
        tft.fillCircle(col_pixels[col],row_pixels[i-1], 6,colorarray[(colorcount+2)%4]);
        tft.fillCircle(col_pixels[col],row_pixels[i-2], 6,colorarray[(colorcount+3)%4]);
        delay(25);
        colorcount++;
        }
        tft.fillCircle(col_pixels[col],row_pixels[i+1], 6,colorarray[5+globalturn%2]);
        tft.fillCircle(col_pixels[col],row_pixels[i], 6,colorarray[5+globalturn%2]);
        tft.fillCircle(col_pixels[col],row_pixels[i-1], 6,colorarray[5+globalturn%2]);
        tft.fillCircle(col_pixels[col],row_pixels[i-2], 6,colorarray[5+globalturn%2]);
        return true;
      }
    } else {
      consecCount = 0;
    }
  }
  //diag 1 check
  toStart1(row,col);
  consecCount = 0;
  for (int i=whereToStart[0]; i<5;i++) { //rows are the limiting factor 
    // this if checks consecutive-ness, if the value is a player tile, and that the column number doesnt go out of range
    if (board[i][whereToStart[1]] == board[i+1][whereToStart[1]+1] && (board[i][whereToStart[1]]==1 || board[i][whereToStart[1]]==2) && whereToStart[1]<6) {
      consecCount++;
      if (consecCount==3) {
        while (colorcount<50){
          tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i+1], 6,colorarray[colorcount%4]);
          tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[(colorcount+1)%4]);
          tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i-1], 6,colorarray[(colorcount+2)%4]);
          tft.fillCircle(col_pixels[whereToStart[1]-2],row_pixels[i-2], 6,colorarray[(colorcount+3)%4]);
          delay(25);
          colorcount++;
        }
          tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i+1], 6,colorarray[5+globalturn%2]);
          tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[5+globalturn%2]);
          tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i-1], 6,colorarray[5+globalturn%2]);
          tft.fillCircle(col_pixels[whereToStart[1]-2],row_pixels[i-2], 6,colorarray[5+globalturn%2]);
        return true;
      }
    } else {
      consecCount = 0;
    }
    whereToStart[1]++;
  }
  //diag 2 check
  toStart2(row,col);
  consecCount = 0;
  for (int i=whereToStart[0]; i<5;i++) { //rows are the limiting factor 
    // this if checks consecutive-ness, if the value is a player tile, and that the column number doesnt go out of range
    if (board[i][whereToStart[1]] == board[i+1][whereToStart[1]-1] && (board[i][whereToStart[1]]==1 || board[i][whereToStart[1]]==2) && whereToStart[1] >0) {
      consecCount++;
      if (consecCount==3) {
        while (colorcount<50){
          tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i+1], 6,colorarray[colorcount%4]);
          tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[(colorcount+1)%4]);
          tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i-1], 6,colorarray[(colorcount+2)%4]);
          tft.fillCircle(col_pixels[whereToStart[1]+2],row_pixels[i-2], 6,colorarray[(colorcount+3)%4]);
          delay(25);
          colorcount++;
        }
          tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i+1], 6,colorarray[5+globalturn%2]);
          tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[5+globalturn%2]);
          tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i-1], 6,colorarray[5+globalturn%2]);
          tft.fillCircle(col_pixels[whereToStart[1]+2],row_pixels[i-2], 6,colorarray[5+globalturn%2]);
        return true;
      }
    } else {
      consecCount = 0;
    }
    whereToStart[1]--;
  }
  return false;
}

/*
 * addTile
 * Adds a tile to the given col by the current player
 * @param board[6][7], the board's current state
 * @param choice, col number
 * @param playerTurn, current player's turn
 * @return int value of the row number that it was placed
*/
int space(int select){ //returns first empty col in row. -1 for full
  for(int i = 5; i >= 0; i--){
    if(board[i][select] == 0){
      return i;
    }
  }
  return -1;
}
  
int userMove(int selection, int playerTurn){
  if(space(selection) != -1){
    int row = 0;
    for (row; row<6; row++) { // from the top to the bottom
      if (row == 5){
          board[5][selection] = 2-playerTurn%2;
          break;
      }
      else if (board[row+1][selection] != 0){
          board[row][selection] = 2-playerTurn%2;
          break;
          }

      tft.fillCircle(col_pixels[selection],row_pixels[row], 6,colorarray[5+playerTurn%2]);
      delay(60);
      tft.fillCircle(col_pixels[selection],row_pixels[row], 6,ST7735_WHITE);
      
    }
    tft.fillCircle(col_pixels[selection],row_pixels[row], 6,colorarray[5+playerTurn%2]);
    
    return row;
  }
  return -1;
}

int AI() {
  int maximum = -42;
  int score = 0;
  int bestCol = 0;
  int row = -1;
  for (int i=0;i<7;i++) {
    row = space(i);
    if(row != -1){
      board[row][i] = 2;
      if(winCheckNoPrint(row,i)){
        return i;
      }
      score = negamax(depth, globalturn+1);
      board[row][i] = 0;
      if (score >maximum) {
        maximum = score;
        bestCol = i;
      }
    }
  }
  board[space(bestCol)][bestCol] = 2;
  drawBoard();
  return bestCol;
}

int negamax(int d, int turns) {
  int row = 0;
  if (d == 0){return 0;}
  if (turns == 42)return 0;
  
  for (int i = 0;i < 7;i++) {
    row = space(i);
    if (row != -1){
      board[row][i] = 2-(turns)%2;
      if (winCheckNoPrint(row, i)) {
        //drawBoard();
        board[row][i] = 0;
        return -(43-turns)/2;
      }
      board[row][i] = 0;
    }
  }

  int bestScore = -42;
  int score =-42;

  for (int i =0;i<7;i++) {
    row = space(i);
    if(row != -1){
      board[row][i] = 2-(turns)%2;
      score = -negamax(d-1, turns+1);
      board[row][i] = 0;
      if (score>bestScore) {
        bestScore = score;
      }
    }
  }
  return bestScore; 
}

void setup(void) {
  Serial.begin(9600);
  tft.initR(INITR_BLACKTAB);
  pinMode(selectButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  SD.begin(SD_CS);

}

void introScreen(){
  tft.setFont(&FreeMono9pt7b);
  drawtext(40,127,"PLAY!",colorarray[colorcounter%7]);
  drawtext(36,149,"LEVEL",colorarray[(colorcounter+1)%7]);
  colorcounter++;
  }

void gamePlay(int sel){
  int placed_row = userMove(sel,globalturn);
  if(placed_row != -1){
    globalturn++;
    if (winCheck(placed_row,sel)){
      ingame = false;
      return;
    }
    //call ai
    sel = AI();
    globalturn++;
    if (winCheck(space(sel),sel)){
      ingame = false;
      return;
    }
  }
  return;
}

void loop() {
  bmpDraw("introbg.bmp", 0, 0);
  tft.setFont(&FreeMonoBold12pt7b);
  //drawtext(3,27,"CONNECT",ST7735_RED);
  
  while (intro){
    selectButtonState = digitalRead(selectButton);
    introScreen();
    
    if (selectButtonState){
      intro = false;
      }
    delay(75);
    }
    bmpDraw("board.bmp", 0, 0);
   while(ingame){
      selectButtonState = digitalRead(selectButton);
      leftButtonState = digitalRead(leftButton);
      rightButtonState = digitalRead(rightButton);
      downButtonState = digitalRead(downButton);
      
      tft.fillTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_WHITE); //adjacent integers are coordinates for each point on triangle
      
      if (selectButtonState == HIGH){
        tone(Speaker,NOTE_A4,300);
        gamePlay(choice);
        delay(50);
        }
        
      else if (leftButtonState == HIGH){
        tft.fillTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_BLACK);
        if (choice == 0)choice = 6;
        else choice--;
        delay(50);
        }
      else if (rightButtonState == HIGH){
        tft.fillTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_BLACK);
        if (choice == 6)choice = 0;
        else choice++;
        delay(50);
        }
      else if (downButtonState == HIGH){
        Serial.println("");
        }
        delay(150); 
     }
   
   endgame = true;
   //tft.setFont();
   
   if (globalturn%2) {
    bmpDraw("gameover.bmp", 0, 0);
    /*
    drawtext(23,120,"THE CONNECT 4",ST7735_GREEN);
    drawtext(23,132,"MATRIX WAS NOT",ST7735_GREEN);
    drawtext(23,144,"WELL-DEFINED!",ST7735_GREEN);
    tft.setFont(&FreeMono9pt7b);
    drawtext(15,110,"GAME OVER",ST7735_GREEN);
*/
    /*while (endgame){      
      selectButtonState = digitalRead(selectButton);
      tft.drawRect(9, 95, 110, 20, colorarray[random(0,7)]);
      if (selectButtonState){
          resetFunc();
      }
      delay(75);
    }
    */
   }else{
    bmpDraw("winner.bmp", 0, 0);
    //tft.setFont(&FreeMono9pt7b);
    while (endgame){
      /*int16_t txtColor = colorarray[random(0,7)];
      drawtext(100,55,"A",txtColor);
      drawtext(70,75,"GLORIOUS",txtColor);
      drawtext(95,95,"WIN!",txtColor);
      */
      selectButtonState = digitalRead(selectButton);
      if (selectButtonState){
        resetFunc();
        }
      delay(75);
      }
    }
}
