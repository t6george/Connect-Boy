#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#include "pitches.h"

#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#define TFT_CS  10  // Chip select line for TFT display
#define TFT_RST  9  // Reset line for TFT (or see below...)
#define TFT_DC   8  // Data/command line for TFT

#define SD_CS    4  // Chip select line for SD card

#define TFT_CS     10
#define TFT_RST    9  //reset
#define TFT_DC     8

#define selectButton 3
#define leftButton  2
#define rightButton 5
#define downButton 14

#define BUFFPIXEL 20

#define Speaker  7

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

////////Variables
uint16_t colorarray[8]={ST7735_BLACK,ST7735_GREEN,ST7735_BLUE,ST7735_MAGENTA,ST7735_YELLOW,ST7735_WHITE,ST7735_RED};
uint16_t stonecolors[2] = {ST7735_YELLOW,ST7735_RED};
int whereToStart[2];
int row_pixels[6] = {38,56,73,91,108,125}; //cooedinates of each rock and column on display (in pixels)
int col_pixels[7] = {13,30,47,64,80,97,114};
int colorcounter = 0;
int globalturn = 1;
bool intro = true;
bool ingame = true;
bool endgame = false;
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

void drawfillrects(int x, int y, int w, int h, uint16_t color, bool center_x,bool center_y) {
      if (center_x){
        x = tft.width()/2-w/2;
        }
      if (center_y){
        y = tft.height()/2 - h/2;
      }
      tft.fillRect(x,y,w,h,color);
    }

void drawfillcircles(int x,int y, uint8_t radius, uint16_t color,bool center_x,bool center_y) {
    if (center_x){
    x = tft.width()/2-radius;
    }
    if (center_y){
      y = tft.height()/2 - radius;
    }
     tft.fillCircle(y ,x , radius, color);
 }
 

void drawborderrects(int x, int y, int w, int h, uint16_t color, bool center_x,bool center_y) {
  if (center_x){
    x = tft.width()/2-w/2;
    }
  if (center_y){
    y = tft.height()/2 - h/2;
  }
  tft.drawRect(x,y,w,h,color);
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

  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print("File not found");
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print("Image size: ");
        Serial.print(bmpWidth);
        Serial.print('x');
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
        Serial.print("Loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println("BMP format not recognized.");
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
//Connect 4 Functions
  /*
 * toStart1
 * Determies from where to start checking diagonally from top left to bottom right
 * @param row, row number
 * @param col, col number
*/

void(* resetFunc) (void) = 0;

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

boolean winCheck(int row, int col) {
  
  int consecCount = 0;
  //hori check
  for (int i = 0; i<6;i++) { // counts consecutive values horizontally in given row
    if (board[row][i] == board[row][i+1] && (board[row][i] ==1 || board[row][i] == 2)) {
      consecCount++;
      if (consecCount==3) {
        int colorcount = 0;
        while (colorcount<50){
        drawfillcircles(row_pixels[row],col_pixels[i+1], 6,colorarray[colorcount%4],false,false);
        drawfillcircles(row_pixels[row],col_pixels[i], 6,colorarray[(colorcount+1)%4],false,false);
        drawfillcircles(row_pixels[row],col_pixels[i-1], 6,colorarray[(colorcount+2)%4],false,false);
        drawfillcircles(row_pixels[row],col_pixels[i-2], 6,colorarray[(colorcount+3)%4],false,false);
        delay(25);
        colorcount++;
        }
        drawfillcircles(row_pixels[row],col_pixels[i+1], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[row],col_pixels[i], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[row],col_pixels[i-1], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[row],col_pixels[i-2], 6,stonecolors[globalturn%2],false,false);
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
        int colorcount = 0;
        while (colorcount<50){
        drawfillcircles(row_pixels[i+1],col_pixels[col], 6,colorarray[colorcount%4],false,false);
        drawfillcircles(row_pixels[i],col_pixels[col], 6,colorarray[(colorcount+1)%4],false,false);
        drawfillcircles(row_pixels[i-1],col_pixels[col], 6,colorarray[(colorcount+2)%4],false,false);
        drawfillcircles(row_pixels[i-2],col_pixels[col], 6,colorarray[(colorcount+3)%4],false,false);
        delay(25);
        colorcount++;
        }
        drawfillcircles(row_pixels[i+1],col_pixels[col], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i],col_pixels[col], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i-1],col_pixels[col], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i-2],col_pixels[col], 6,stonecolors[globalturn%2],false,false);
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
        int colorcount = 0;
        while (colorcount<50){
          drawfillcircles(row_pixels[i+1],col_pixels[whereToStart[1]+1], 6,colorarray[colorcount%4],false,false);
          drawfillcircles(row_pixels[i],col_pixels[whereToStart[1]], 6,colorarray[(colorcount+1)%4],false,false);
          drawfillcircles(row_pixels[i-1],col_pixels[whereToStart[1]-1], 6,colorarray[(colorcount+2)%4],false,false);
          drawfillcircles(row_pixels[i-2],col_pixels[whereToStart[1]-2], 6,colorarray[(colorcount+3)%4],false,false);
          delay(25);
          colorcount++;
        }
        drawfillcircles(row_pixels[i+1],col_pixels[whereToStart[1]+1], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i],col_pixels[whereToStart[1]], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i-1],col_pixels[whereToStart[1]-1], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i-2],col_pixels[whereToStart[1]-2], 6,stonecolors[globalturn%2],false,false);
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
        int colorcount = 0;
        while (colorcount<50){
          drawfillcircles(row_pixels[i+1],col_pixels[whereToStart[1]-1], 6,colorarray[colorcount%4],false,false);
          drawfillcircles(row_pixels[i],col_pixels[whereToStart[1]], 6,colorarray[(colorcount+1)%4],false,false);
          drawfillcircles(row_pixels[i-1],col_pixels[whereToStart[1]+1], 6,colorarray[(colorcount+2)%4],false,false);
          drawfillcircles(row_pixels[i-2],col_pixels[whereToStart[1]+2], 6,colorarray[(colorcount+3)%4],false,false);
          delay(25);
          colorcount++;
        }
        drawfillcircles(row_pixels[i+1],col_pixels[whereToStart[1]-1], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i],col_pixels[whereToStart[1]], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i-1],col_pixels[whereToStart[1]+1], 6,stonecolors[globalturn%2],false,false);
        drawfillcircles(row_pixels[i-2],col_pixels[whereToStart[1]+2], 6,stonecolors[globalturn%2],false,false);
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
bool space(int select){
  if(board[0][select]!=0){
       return false;
    }
  return true;
  }
  
int userMove(int selection, int playerTurn){
  if(space(selection)){
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

      drawfillcircles(row_pixels[row],col_pixels[selection], 6,stonecolors[playerTurn%2],false,false);
      delay(60);
      drawfillcircles(row_pixels[row],col_pixels[selection], 6,ST7735_WHITE,false,false);
      
    }
    
    Serial.print("Chosen column: ");
    Serial.print(choice);
    Serial.print("  top row is ");
    Serial.println(board[0][choice]);
    drawBoard();
    
    for (int i = 0; i<6; i++){
      for (int j = 0; j<7; j++){
        if (board[i][j] == 1){
           drawfillcircles(row_pixels[i],col_pixels[j], 6,stonecolors[1],false,false);
          }
        else if (board[i][j] == 2){
            drawfillcircles(row_pixels[i],col_pixels[j], 6,stonecolors[0],false,false);
          }
        }
      }
      return row;
  }
  return 0;
}




void setup(void) {
  Serial.begin(9600);
  tft.initR(INITR_BLACKTAB);
  pinMode(selectButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);

  for (int cock = 0; cock<6;cock++){
    for (int vagene = 0; vagene<7; vagene++){
      board[cock][vagene] = 0;
      }
    }
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }
  Serial.println("OK!");
  

}

void introScreen(){
  tft.setFont(&FreeMono9pt7b);
  drawtext(40,127,"PLAY!",colorarray[colorcounter%7]);
  drawtext(36,149,"LEVEL",colorarray[(colorcounter+1)%7]);
  colorcounter+=1;
  }

void gamePlay(int sel){
  int placed_row = userMove(sel,globalturn);
    if (winCheck(placed_row,sel)){
    
    //Serial.print(board[0][0]);
      ingame = false;
      }
    //call ai
    
  }

void loop() {
  bmpDraw("introbg.bmp", 0, 0);
  tft.setFont(&FreeMonoBold12pt7b);
  drawtext(3,27,"CONNECT",ST7735_RED);
  
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
      
      tft.drawTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_WHITE); //adjacent integers are coordinates for each point on triangle
      
      if (selectButtonState == HIGH){
        tone(Speaker,NOTE_A4,300);
        gamePlay(choice);
        globalturn++;
        delay(50);
        }
        
      else if (leftButtonState == HIGH){
        tft.drawTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_BLACK);
        if (choice == 0)choice = 6;
        else choice--;
        delay(50);
        }
      else if (rightButtonState == HIGH){
        tft.drawTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_BLACK);
        if (choice == 6)choice = 0;
        else choice++;
        delay(50);
        }
      else if (downButtonState == HIGH){
        Serial.println("fuck punithan");
        }
        delay(150); 
     }
   
   endgame = true;
   tft.setFont();
   
   if (globalturn%2) {
    bmpDraw("gameover.bmp", 0, 0);
    drawtext(23,120,"THE CONNECT 4",ST7735_GREEN);
    drawtext(23,132,"MATRIX WAS NOT",ST7735_GREEN);
    drawtext(23,144,"WELL-DEFINED!",ST7735_GREEN);
    tft.setFont(&FreeMono9pt7b);
    drawtext(15,110,"GAME OVER",ST7735_GREEN);

    while (endgame){      
      selectButtonState = digitalRead(selectButton);
      drawborderrects(0, 95, 110, 20, colorarray[random(0,7)],true,false);
      if (selectButtonState){
          resetFunc();
      }
      delay(75);
    }
   }else{
    bmpDraw("winner.bmp", 0, 0);
    //tft.setFont(&FreeMono9pt7b);
    while (endgame){
      int16_t txtColor = colorarray[random(0,7)];
      drawtext(100,55,"A",txtColor);
      drawtext(70,75,"GLORIOUS",txtColor);
      drawtext(95,95,"WIN!",txtColor);
      selectButtonState = digitalRead(selectButton);
      if (selectButtonState){
        resetFunc();
        }
      delay(75);
      }
    }
}
