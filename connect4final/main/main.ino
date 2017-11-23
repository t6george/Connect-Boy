#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>

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

const int PROGMEM introSong[169] = {131,0,0,131,0,0,220,0,0,0,87,0,0,110,0,0,87,0,0,110,0,0,0,175,0,0,220,0,0,175,0,0,147,0,0,0,87,0,0,98,0,0,87,0,0,98,0,0,0,
                    147,0,0,147,0,0,233,0,0,0,98,0,0,117,0,0,98,0,0,117,0,0,0,196,0,0,220,0,0,196,0,0,165,0,0,0,98,0,0,110,0,0,98,0,0,110,0,0,0,
                    165,0,0,165,0,0,131,0,0,0,0,220,0,0,0,0,175,0,0,0,0,233,0,0,131,0,0,147,0,0,0,0,233,0,0,0,0,196,0,0,0,0,175,0,0,196,0,0,220,
                    0,0,0,0,0,0,165,0,0,175,0,0,196,0,0,175,0,0,0,0,0,0};
            

////////Variables
int depth = 4;
const uint16_t colorarray[7]={ST7735_BLACK,ST7735_GREEN,ST7735_BLUE,ST7735_MAGENTA,ST7735_WHITE,ST7735_YELLOW,ST7735_RED};
int whereToStart[2];
const int row_pixels[6] = {38,56,73,91,108,125}; //coordinates of each rock and column on display (in pixels)
const int col_pixels[7] = {13,30,47,64,80,97,114};
int globalturn = 1;
bool intro = true;
bool ingame = false;
bool settings = false;
bool endgame = true;
int colorcounter = 0;
int selectButtonState = 0; // variable for reading the selectButton status
int leftButtonState = 0; // variable for reading the leftButton status
int rightButtonState = 0; // variable for reading the leftButton status
int downButtonState = 0; // variable for reading the downButton status
int upButtonState = 0; // variable for reading the upButton status
int board[6][7]; //the actual board
int choice = 3; //the column the user is hovering over
int introMusicIndex = 0;
int inGameMusicIndex = 0;
int aiChoice = 0;
int timer = 0;
int button_delay = 0;
int scores[7] = {-42};
int maximum = -42;
int bestCol = 0;
int row = -1;
int random_choice = 0;
int tie_count = 0;
int time_limit = 200;
int width = 0;
int options_state = 0;
int settings_state = 0;


const int inGameSong[98] = {440,440,440,440,440,440,440,440,440,440,440,440,440,0,0,0,494,494,494,523,523,523,0,0,0,523,523,523,0,0,0,
                      494,494,0,440,440,0,0,0,0,0,0,0,0,392,440,440,440,440,440,440,440,440,0,392,392,392,392,0,0,0,0,0,0,392,440,440,
                      440,440,440,440,440,440,0,392,392,392,392,0,0,0,0,0,0,523,523,523,523,523,0,0,494,494,494,494,494,0,0};

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


  // Open requested file on SD card
  bmpFile = SD.open(filename);

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    read32(bmpFile);
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
        rowSize = (bmpWidth * 3 + 3) & ~3;
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

//Connect 4 Functions
 /*
 * toStart1
 * Determies from where to start checking diagonally from top left to bottom right
 * @param row, row number
 * @param col, col number
*/

void(* resetFunc) (void) = 0;
/*
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
*/
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
/////////////////////////////////////////////////////////////////////////////////
bool winCheck(int row, int col, bool with_print) {
  int consecCount = 0;
  int colorcount = 0;
  //hori check
  for (int i = 0; i<6;i++) { // counts consecutive values horizontally in given row
    if (board[row][i] == board[row][i+1] && (board[row][i] ==1 || board[row][i] == 2)) {
      consecCount++;
      if (consecCount==3) {
        if (with_print){
          while (colorcount<50){
        
          tft.fillCircle(col_pixels[i+1],row_pixels[row], 6,colorarray[colorcount%4]);
          tft.fillCircle(col_pixels[i],row_pixels[row], 6,colorarray[(colorcount+1)%4]);
          tft.fillCircle(col_pixels[i-1],row_pixels[row], 6,colorarray[(colorcount+2)%4]);
          tft.fillCircle(col_pixels[i-2],row_pixels[row], 6,colorarray[(colorcount+3)%4]);
          delay(25);
          colorcount++;
          }
          tft.fillCircle(col_pixels[i+1],row_pixels[row], 6,colorarray[5+(globalturn+1)%2]);
          tft.fillCircle(col_pixels[i],row_pixels[row], 6,colorarray[5+(globalturn+1)%2]);
          tft.fillCircle(col_pixels[i-1],row_pixels[row], 6,colorarray[5+(globalturn+1)%2]);
          tft.fillCircle(col_pixels[i-2],row_pixels[row], 6,colorarray[5+(globalturn+1)%2]);
          }
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
        if(with_print){
          while (colorcount<50){
          tft.fillCircle(col_pixels[col],row_pixels[i+1], 6,colorarray[colorcount%4]);
          tft.fillCircle(col_pixels[col],row_pixels[i], 6,colorarray[(colorcount+1)%4]);
          tft.fillCircle(col_pixels[col],row_pixels[i-1], 6,colorarray[(colorcount+2)%4]);
          tft.fillCircle(col_pixels[col],row_pixels[i-2], 6,colorarray[(colorcount+3)%4]);
          delay(25);
          colorcount++;
          }
          tft.fillCircle(col_pixels[col],row_pixels[i+1], 6,colorarray[5+(globalturn+1)%2]);
          tft.fillCircle(col_pixels[col],row_pixels[i], 6,colorarray[5+(globalturn+1)%2]);
          tft.fillCircle(col_pixels[col],row_pixels[i-1], 6,colorarray[5+(globalturn+1)%2]);
          tft.fillCircle(col_pixels[col],row_pixels[i-2], 6,colorarray[5+(globalturn+1)%2]);
        }
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
    if (board[i][whereToStart[1]] == board[i+1][whereToStart[1]+1] && (board[i][whereToStart[1]]==1 
    || board[i][whereToStart[1]]==2) && whereToStart[1]<6) {
      consecCount++;
      if (consecCount==3) {
        if (with_print){
          while (colorcount<50){
            tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i+1], 6,colorarray[colorcount%4]);
            tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[(colorcount+1)%4]);
            tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i-1], 6,colorarray[(colorcount+2)%4]);
            tft.fillCircle(col_pixels[whereToStart[1]-2],row_pixels[i-2], 6,colorarray[(colorcount+3)%4]);
            delay(25);
            colorcount++;
          }
            tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i+1], 6,colorarray[5+(globalturn+1)%2]);
            tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[5+(globalturn+1)%2]);
            tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i-1], 6,colorarray[5+(globalturn+1)%2]);
            tft.fillCircle(col_pixels[whereToStart[1]-2],row_pixels[i-2], 6,colorarray[5+(globalturn+1)%2]);
          }
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
    if (board[i][whereToStart[1]] == board[i+1][whereToStart[1]-1] && (board[i][whereToStart[1]]==1 
    || board[i][whereToStart[1]]==2) && whereToStart[1] >0) {
      consecCount++;
      if (consecCount==3) {
        if (with_print){
          while (colorcount<50){
            tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i+1], 6,colorarray[colorcount%4]);
            tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[(colorcount+1)%4]);
            tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i-1], 6,colorarray[(colorcount+2)%4]);
            tft.fillCircle(col_pixels[whereToStart[1]+2],row_pixels[i-2], 6,colorarray[(colorcount+3)%4]);
            delay(25);
            colorcount++;
          }
            tft.fillCircle(col_pixels[whereToStart[1]-1],row_pixels[i+1], 6,colorarray[5+(globalturn+1)%2]);
            tft.fillCircle(col_pixels[whereToStart[1]],row_pixels[i], 6,colorarray[5+(globalturn+1)%2]);
            tft.fillCircle(col_pixels[whereToStart[1]+1],row_pixels[i-1], 6,colorarray[5+(globalturn+1)%2]);
            tft.fillCircle(col_pixels[whereToStart[1]+2],row_pixels[i-2], 6,colorarray[5+(globalturn+1)%2]);
        }
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
  
int userMove(int selection, int player){
  if(space(selection) != -1){
    int row = 0;
    for (row; row<6; row++) { // from the top to the bottom
      if (row == 5){
          board[5][selection] = 2-player;
          break;
      }
      else if (board[row+1][selection] != 0){
          board[row][selection] = 2-player;
          break;
          }

      tft.fillCircle(col_pixels[selection],row_pixels[row], 6,colorarray[5+player]);
      delay(60);
      tft.fillCircle(col_pixels[selection],row_pixels[row], 6,ST7735_WHITE);
      
    }
    tft.fillCircle(col_pixels[selection],row_pixels[row], 6,colorarray[5+player]); 
    return row;
  }
  return -1;
}

int AI() {
  maximum = -42;
  bestCol = 0;
  row = -1;
  tie_count = 0;
  
  for (int i=0;i<7;i++)scores[i] = -42;
  
  for (int i=0;i<7;i++) {
    row = space(i);
    if(row != -1){
      board[row][i] = 2;
      if(winCheck(row,i,false)){
        board[row][i] = 0;
        return i;
      }
      scores[i] = -negamax(depth, globalturn+1);
      board[row][i] = 0;
    }
  }
  for (int i=0;i<7;i++) {//find maximum score
     if (scores[i] > maximum) {
        maximum = scores[i];
        bestCol = i;
      }
    }
  for (int i=0;i<7;i++) {
    if(scores[i]!=maximum){
      scores[i] = -42;
      tie_count++;
    }
  }
  if (tie_count == 6)return bestCol;
  //Serial.println("so what you're saying is that its a random GARBAGE value");
  int boundary = 0;
  do{
  randomSeed(analogRead(A1));
  random_choice = random(2-boundary%3,4+boundary%3);
  boundary++;
  } while(scores[random_choice] ==-42);
  //Serial.print("Random choice: ");
  //Serial.println(random_choice);
  return random_choice;
}

int negamax(int d, int turns) {
  int row = 0;
  int bestScore = -42;
  int score =-42;
  
  if (d == 0)return 0;
  if (turns == 42)return 0;
  
  for (int i = 0;i < 7;i++) {
    row = space(i);
    if (row != -1){
      board[row][i] = 2-(turns)%2;
      if (winCheck(row, i,false)) {
        board[row][i] = 0;
        return (43-turns)/2;
      }
      board[row][i] = 0;
    }
  }

  for (int i = 0;i<7;i++) {
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

void gameClock(bool select_pressed, bool left_pressed, bool right_pressed){
  timer++;
  }

void setup(void) {
  Serial.begin(9600);
  tft.initR(INITR_BLACKTAB);
  pinMode(selectButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  pinMode(downButton, INPUT);
  pinMode(upButton, INPUT);
  SD.begin(SD_CS);
}

void introScreen(){
  if(options_state%2){
  drawtext(50,118,"PLAY!",colorarray[5]);
  drawtext(40,141,"SETTINGS",colorarray[colorcounter%7]);
  tft.drawRect(27, 134, 74, 22, colorarray[(colorcounter+2)%7]);

  }else{
  drawtext(50,118,"PLAY!",colorarray[(colorcounter+1)%7]);
  drawtext(40,141,"SETTINGS",colorarray[5]);
  tft.drawRect(27, 111, 74, 22, colorarray[(colorcounter+2)%7]);
  }
  if (button_delay>0) button_delay--;
  colorcounter++;
  }

void gamePlay(int sel){
  int placed_row = userMove(sel,1);
  if(placed_row == -1) return;
  globalturn++;
  if (winCheck(placed_row,sel,true)){
    ingame = false;
    return;
  }
  
  aiChoice = AI();
  userMove(aiChoice, 0);
  globalturn++;
  
  if (winCheck(space(aiChoice)+1,aiChoice,false)){
    winCheck(space(aiChoice)+1,aiChoice,true);
    ingame = false;
    return;
  }
  
  if(globalturn == 42){
    ingame = false;
    }
    
  timer = 0;
}

void settingsScreen(){
  if(!settings_state%3){
     tft.fillTriangle(20, 10, 20, 10, 20, 10, colorarray[2]);

  }else{
     tft.fillTriangle(20, 110, 20, 10, 20, 10, colorarray[2]);
  }
  if (button_delay>0) button_delay--;
  }


void loop() {
  bmpDraw("start.bmp", 0, 0);
  delay(200);
  tone(Speaker,262,75);
  delay(75);
  tone(Speaker,2093,225);
  delay(500);
  
  bmpDraw("introbg.bmp", 0, 0);
  
  while (intro){
    selectButtonState = digitalRead(selectButton);
    downButtonState = digitalRead(downButton);
    upButtonState = digitalRead(upButton);

    //music goes here
    tone(Speaker,pgm_read_byte_near(introSong+introMusicIndex),75);
    
    introMusicIndex++;
    if (introMusicIndex==169){
      introMusicIndex=0;
    }
        
    introScreen();

    
    if (selectButtonState && !button_delay){
      intro = false;
      if(options_state%2){
        settings = true;
      }else{
        ingame = true;
      }
      button_delay = 4;
    }
    
    else if (downButtonState == HIGH && !button_delay){
       tft.drawRect(27, 111, 74, 22, colorarray[5]);
       options_state++;
       button_delay = 4;
       }
    else if (upButtonState == HIGH && !button_delay){
       tft.drawRect(27, 134, 74, 22, colorarray[5]);
       options_state--;
       button_delay = 4;
       }
    delay(30);
    }

    
   if(settings){
     button_delay = 0;
     bmpDraw("settings.bmp", 0, 0);
   
    while(settings){
      tone(Speaker,pgm_read_byte_near(introSong+introMusicIndex),75);
      introMusicIndex++;
      if (introMusicIndex==169){
          introMusicIndex=0;
      }  
      settingsScreen();
      if (selectButtonState && !button_delay){
        settings = false;
        if(settings_state%3==2){
          ingame = true;
        }else if(settings_state%3==1){
          Serial.println("Timer");

        }else{
          Serial.println("Difficulty");
          }
        button_delay = 4;
      }
      delay(50);
    }
}
    
   
   
if(ingame){
     button_delay = 0;
     bmpDraw("board.bmp", 0, 0);
     tft.fillRect(40, 140, 80, 13, colorarray[6]);
   
   while(ingame){
      selectButtonState = digitalRead(selectButton);
      leftButtonState = digitalRead(leftButton);
      rightButtonState = digitalRead(rightButton);
      
      tft.fillTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_WHITE); //adjacent integers are coordinates for each point on triangle
      
      if (selectButtonState == HIGH && !button_delay){
        gamePlay(choice);
        button_delay = 3;
        }
        
      else if (leftButtonState == HIGH && !button_delay){
        tft.fillTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_BLACK);
        if (choice == 0)choice = 6;
        else choice--;
        button_delay = 3;
        }
      else if (rightButtonState == HIGH && !button_delay){
        tft.fillTriangle(col_pixels[choice]-6, 10, col_pixels[choice], 22, col_pixels[choice]+6, 10, ST7735_BLACK);
        if (choice == 6)choice = 0;
        else choice++;
        button_delay = 3;
        }

        //music goes here
        tone(Speaker,inGameSong[inGameMusicIndex],75);
        
        inGameMusicIndex++;
        if (inGameMusicIndex==98){
          inGameMusicIndex=0;
        }
        
        if (button_delay>0) button_delay--;
         
        if (timer%20 == 0) {
          width = 80-80*timer/time_limit;
          tft.fillRect(40+width, 140, 80-width, 13, colorarray[5]);
          tft.fillRect(40, 140, width, 13, colorarray[6]);
          }
        if (timer==time_limit){
          randomSeed(analogRead(A2));
          gamePlay(random(0,6));
          timer = 0;
        }
        timer++;
        delay(50); 
     }

   }
      
   if (globalturn%2) {
    bmpDraw("gameover.bmp", 0, 0);
   }else{
    bmpDraw("winner.bmp", 0, 0);
   }
    while (endgame){
      selectButtonState = digitalRead(selectButton);
      if (selectButtonState){
        resetFunc();
        }
      delay(75);
      }
    }


