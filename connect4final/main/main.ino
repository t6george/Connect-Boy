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

#define Speaker  A0 // Analog pin for the speaker

#define selectButton 3
#define leftButton  2
#define rightButton 5
#define downButton 7
#define upButton 6

#define BUFFPIXEL 1

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/// Variables ///

//Music
const int inGameSong[98] = {440, 440, 440, 440, 440, 440, 440, 440, 440, 440, 440, 440, 440, 0, 0, 0, 494, 494, 494, 523, 523, 523, 0, 0, 0, 523, 523, 523, 0, 0, 0,
                            494, 494, 0, 440, 440, 0, 0, 0, 0, 0, 0, 0, 0, 392, 440, 440, 440, 440, 440, 440, 440, 440, 0, 392, 392, 392, 392, 0, 0, 0, 0, 0, 0, 392, 440, 440,
                            440, 440, 440, 440, 440, 440, 0, 392, 392, 392, 392, 0, 0, 0, 0, 0, 0, 523, 523, 523, 523, 523, 0, 0, 494, 494, 494, 494, 494, 0, 0
                           };

const int PROGMEM introSong[169] = {131, 0, 0, 131, 0, 0, 220, 0, 0, 0, 87, 0, 0, 110, 0, 0, 87, 0, 0, 110, 0, 0, 0, 175, 0, 0, 220, 0, 0, 175, 0, 0, 147, 0, 0, 0, 87, 0, 0, 98, 0, 0, 87, 0, 0, 98, 0, 0, 0,
                                    147, 0, 0, 147, 0, 0, 233, 0, 0, 0, 98, 0, 0, 117, 0, 0, 98, 0, 0, 117, 0, 0, 0, 196, 0, 0, 220, 0, 0, 196, 0, 0, 165, 0, 0, 0, 98, 0, 0, 110, 0, 0, 98, 0, 0, 110, 0, 0, 0,
                                    165, 0, 0, 165, 0, 0, 131, 0, 0, 0, 0, 220, 0, 0, 0, 0, 175, 0, 0, 0, 0, 233, 0, 0, 131, 0, 0, 147, 0, 0, 0, 0, 233, 0, 0, 0, 0, 196, 0, 0, 0, 0, 175, 0, 0, 196, 0, 0, 220,
                                    0, 0, 0, 0, 0, 0, 165, 0, 0, 175, 0, 0, 196, 0, 0, 175, 0, 0, 0, 0, 0, 0
                                   }; //Had to put the larger song into progmem in order to free up RAM
int introMusicIndex = 0;
int inGameMusicIndex = 0;
                                  
//Board
int board[6][7]; //the actual board
int choice = 3; //the column the user is hovering over
int row = -1;

//AI
int depth = 3;
int scores[7] = { -42};
int aiChoice = 0;
int bestCol = 0;
int maximum = -42;
int random_choice = 0;

//Graphics
const uint16_t colorarray[7] = {ST7735_BLACK, ST7735_GREEN, ST7735_BLUE, ST7735_MAGENTA, ST7735_WHITE, ST7735_YELLOW, ST7735_RED};
const int row_pixels[6] = {38, 56, 73, 91, 108, 125}; //coordinates of each rock and column on display (in pixels)
const int col_pixels[7] = {13, 30, 47, 64, 80, 97, 114};
int colorcounter = 0;
int width = 0;

//Inputs
int selectButtonState = 0; // variable for reading the selectButton status
int leftButtonState = 0; // variable for reading the leftButton status
int rightButtonState = 0; // variable for reading the leftButton status
int downButtonState = 0; // variable for reading the downButton status
int upButtonState = 0; // variable for reading the upButton status
int button_delay = 0;

//Gameplay
int whereToStart[2];
int globalturn = 1;
int timer = 0;
int badMoveCount = 0;
int time_limit = 200;
int options_state = 0;
int settings_state = 0;
bool isTimeLimit = true;
bool intro = true;
bool ingame = false;
bool settings = false;
bool endgame = true;


void(* resetFunc) (void) = 0; //Reset Function - essentially presses the reset button

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

///// Graphics Functions ///// ~ Came with our LCD screen and these are the functions we need to build images and draw shapes
void drawtext(int x, int y, String text, uint16_t color) {
  tft.setCursor(x, y);
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
  uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();


  // Open requested file on SD card
  bmpFile = SD.open(filename);

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    read32(bmpFile);
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(bmpWidth);
        Serial.println(bmpHeight);
        rowSize = (bmpWidth * 3 + 3) & ~3;
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);

        for (row = 0; row < h; row++) { // For each scanline...
          if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col = 0; col < w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.Color565(r, g, b));
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



///// Connect 4 Functions /////

/*
  toStart1
  Determies from where to start checking diagonally from top left to bottom right
  @param row, row number
  @param col, col number
*/
void toStart1(int row, int col) { //top left to bottom right
  whereToStart[0] = 0;
  whereToStart[1] = 0;
  int a = col - row;
  if (a >= 0) {
    whereToStart[0] = 0;
    whereToStart[1] = a;
  } else {
    whereToStart[0] = -1 * a;
    whereToStart[1] = 0;
  }
}
/*
   toStart2
   Determies from where to start checking diagonally from top right to bottom left
   @param row, row number
   @param col, col number
*/
void toStart2(int row, int col) { //top right to bottom right
  whereToStart[0] = 0;
  whereToStart[1] = 0;
  int a = col + row;
  if (a > 6) {
    whereToStart[0] = a - 6;
    whereToStart[1] = 6;
  } else {
    whereToStart[0] = 0;
    whereToStart[1] = a;
  }
}
/*
 * winCheck
 * Determines if a given move results in a win
 * @param row, row number
 * @param col, col number
 * @param with_print, decides whether to print the colors of a winning move or not (used in AI)
 * @return true if win found, false if not
 */
bool winCheck(int row, int col, bool with_print) {
  int consecCount = 0; // counts consecutive positions to see if there's four in a row
  int colorcount = 0; // used for flashy graphics
  
  //hori check
  for (int i = 0; i < 6; i++) { // counts consecutive values horizontally in given row
    if (board[row][i] == board[row][i + 1] && (board[row][i] == 1 || board[row][i] == 2)) {
      consecCount++;
      if (consecCount == 3) { //3 consecutive means 4 in a row
        if (with_print) {
          while (colorcount < 50) {

            tft.fillCircle(col_pixels[i + 1], row_pixels[row], 6, colorarray[colorcount % 4]); //flashy winning graphics
            tft.fillCircle(col_pixels[i], row_pixels[row], 6, colorarray[(colorcount + 1) % 4]);
            tft.fillCircle(col_pixels[i - 1], row_pixels[row], 6, colorarray[(colorcount + 2) % 4]);
            tft.fillCircle(col_pixels[i - 2], row_pixels[row], 6, colorarray[(colorcount + 3) % 4]);
            delay(25);
            colorcount++;
          }
          tft.fillCircle(col_pixels[i + 1], row_pixels[row], 6, colorarray[5 + (globalturn + 1) % 2]); //reset tile colors
          tft.fillCircle(col_pixels[i], row_pixels[row], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[i - 1], row_pixels[row], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[i - 2], row_pixels[row], 6, colorarray[5 + (globalturn + 1) % 2]);
        }
        return true;
      }
    } else {
      consecCount = 0;
    }
  }
  
  //verti check ~ similar logic as hori check
  consecCount = 0;
  for (int i = 0; i < 5; i++) { // counts consecutive values vertically in the given col
    if (board[i][col] == board[i + 1][col] && (board[i][col] == 1 || board[i][col] == 2)) {
      consecCount++;
      if (consecCount == 3) {
        if (with_print) {
          while (colorcount < 50) {
            tft.fillCircle(col_pixels[col], row_pixels[i + 1], 6, colorarray[colorcount % 4]);//flashy winning graphics
            tft.fillCircle(col_pixels[col], row_pixels[i], 6, colorarray[(colorcount + 1) % 4]);
            tft.fillCircle(col_pixels[col], row_pixels[i - 1], 6, colorarray[(colorcount + 2) % 4]);
            tft.fillCircle(col_pixels[col], row_pixels[i - 2], 6, colorarray[(colorcount + 3) % 4]);
            delay(25);
            colorcount++;
          }
          tft.fillCircle(col_pixels[col], row_pixels[i + 1], 6, colorarray[5 + (globalturn + 1) % 2]);//reset tile colors
          tft.fillCircle(col_pixels[col], row_pixels[i], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[col], row_pixels[i - 1], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[col], row_pixels[i - 2], 6, colorarray[5 + (globalturn + 1) % 2]);
        }
        return true;
      }
    } else {
      consecCount = 0;
    }
  }

  //Disclaimer: WhereToStart function mathematically calculates the starting row and col we need to start checking in
  //diag 1 check ~ top left to bottom right
  toStart1(row, col);
  consecCount = 0;
  for (int i = whereToStart[0]; i < 5; i++) { //rows are the limiting factor
    // this if checks consecutive-ness, if the value is a player tile, and that the column number doesnt go out of range and that value is equal to the next, increment counter
    if (board[i][whereToStart[1]] == board[i + 1][whereToStart[1] + 1] && (board[i][whereToStart[1]] == 1
        || board[i][whereToStart[1]] == 2) && whereToStart[1] < 6) {
      consecCount++;
      if (consecCount == 3) {
        if (with_print) {
          while (colorcount < 50) {
            tft.fillCircle(col_pixels[whereToStart[1] + 1], row_pixels[i + 1], 6, colorarray[colorcount % 4]);//flashy winning graphics
            tft.fillCircle(col_pixels[whereToStart[1]], row_pixels[i], 6, colorarray[(colorcount + 1) % 4]);
            tft.fillCircle(col_pixels[whereToStart[1] - 1], row_pixels[i - 1], 6, colorarray[(colorcount + 2) % 4]);
            tft.fillCircle(col_pixels[whereToStart[1] - 2], row_pixels[i - 2], 6, colorarray[(colorcount + 3) % 4]);
            delay(25);
            colorcount++;
          }
          tft.fillCircle(col_pixels[whereToStart[1] + 1], row_pixels[i + 1], 6, colorarray[5 + (globalturn + 1) % 2]);//reset tile colors
          tft.fillCircle(col_pixels[whereToStart[1]], row_pixels[i], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[whereToStart[1] - 1], row_pixels[i - 1], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[whereToStart[1] - 2], row_pixels[i - 2], 6, colorarray[5 + (globalturn + 1) % 2]);
        }
        return true;
      }
    } else {
      consecCount = 0;
    }
    whereToStart[1]++;
  }
  //diag 2 check ~ top right to bottom left
  toStart2(row, col);
  consecCount = 0;
  for (int i = whereToStart[0]; i < 5; i++) { //rows are the limiting factor
    // this if checks consecutive-ness, if the value is a player tile, and that the column number doesnt go out of rangeand that value is equal to the next, increment counter
    if (board[i][whereToStart[1]] == board[i + 1][whereToStart[1] - 1] && (board[i][whereToStart[1]] == 1
        || board[i][whereToStart[1]] == 2) && whereToStart[1] > 0) {
      consecCount++;
      if (consecCount == 3) {
        if (with_print) {
          while (colorcount < 50) {
            tft.fillCircle(col_pixels[whereToStart[1] - 1], row_pixels[i + 1], 6, colorarray[colorcount % 4]);//flashy winning graphics
            tft.fillCircle(col_pixels[whereToStart[1]], row_pixels[i], 6, colorarray[(colorcount + 1) % 4]);
            tft.fillCircle(col_pixels[whereToStart[1] + 1], row_pixels[i - 1], 6, colorarray[(colorcount + 2) % 4]);
            tft.fillCircle(col_pixels[whereToStart[1] + 2], row_pixels[i - 2], 6, colorarray[(colorcount + 3) % 4]);
            delay(25);
            colorcount++;
          }
          tft.fillCircle(col_pixels[whereToStart[1] - 1], row_pixels[i + 1], 6, colorarray[5 + (globalturn + 1) % 2]);//reset tile colors
          tft.fillCircle(col_pixels[whereToStart[1]], row_pixels[i], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[whereToStart[1] + 1], row_pixels[i - 1], 6, colorarray[5 + (globalturn + 1) % 2]);
          tft.fillCircle(col_pixels[whereToStart[1] + 2], row_pixels[i - 2], 6, colorarray[5 + (globalturn + 1) % 2]);
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
 * space
 * Determines the first empty row in a col and -1 if full
 * @param select, the column selected
 * @return first empty row in selected col and -1 if full
*/
int space(int select) { 
  for (int i = 5; i >= 0; i--) {
    if (board[i][select] == 0) {
      return i;
    }
  }
  return -1;
}

/*
 * AI
 * Uses negamax logic to recursively compute the best moves at a given depth
 * @return the best column choice
 */
int AI() {
  // resetting key variables every time AI is called
  maximum = -42; //max score value
  bestCol = 0; 
  row = -1;
  badMoveCount = 0; //used in making educated random guess

  for (int i = 0; i < 7; i++)scores[i] = -42; // reset score values

  for (int i = 0; i < 7; i++) { // for each col...
    row = space(i); //check for space
    if (row != -1) {
      board[row][i] = 2; //place coin
      if (winCheck(row, i, false)) { //win found?
        board[row][i] = 0; //remove coin
        return i; //return the current col as the best col
      }
      scores[i] = -negamax(depth, globalturn + 1); //calculate score of a given column
      board[row][i] = 0; //remove coin
    }
  }
  for (int i = 0; i < 7; i++) { //find maximum score
    if (scores[i] > maximum) {
      maximum = scores[i];
      bestCol = i;
    }
  }
  for (int i = 0; i < 7; i++) { //'remove' all non-maximum values
    if (scores[i] != maximum) {
      scores[i] = -42;
      badMoveCount++; //keep track of bad moves
    }
  }
  if (badMoveCount == 6)return bestCol; //special case if only one best col (6 bad = 1 good)
  
  int boundary = 0; //used in making educated guess
  do {
    randomSeed(analogRead(A1));
    random_choice = random(2 - boundary % 3, 4 + boundary % 3); //forces random value to first check rows 3-5(A) then 2-6(B) then 1-7(C), in a repeating fashion (ABCABCABC...)
    boundary++;
  } while (scores[random_choice] == -42); //doesn't consider 'removed' values
  return random_choice;
}

/*
 * negamax
 * Calculates the score of a given board state for the AI at a given depth
 * @param d, the depth (how far it looks ahead)
 * @param turns, keeps track of the internal turns to correctly place AI coins of player coins
 * @return the score of a given board state
 */
int negamax(int d, int turns) {
  int row = 0;
  int bestScore = -42; //best score of the whole board state
  int score = -42; //current score of current branch

  if (d == 0)return 0; //hit the depth limit
  if (turns == 42)return 0; //hit an end to the game

  for (int i = 0; i < 7; i++) { //for each col...
    row = space(i); //check for space
    if (row != -1) {
      board[row][i] = 2 - (turns) % 2; //place coin
      if (winCheck(row, i, false)) { //check win
        board[row][i] = 0; // remove coin
        return (43 - turns) / 2; // return score (the further the score the smaller in magnitude it is where the score represents the number of coins the player would have not placed yet)
      }
      board[row][i] = 0;//remove coin
    }
  }

  for (int i = 0; i < 7; i++) { // for each col...
    row = space(i);
    if (row != -1) { //check for space
      board[row][i] = 2 - (turns) % 2; //place coin
      score = -negamax(d - 1, turns + 1); //recursively call negamax incrementing depth and turns
      board[row][i] = 0; //remove coin
      if (score > bestScore) { //calculate best score within the current board state
        bestScore = score;
      }
    }
  }
  return bestScore;
}

/*
 * introScreen
 * Displays the intro screen
 */
void introScreen() {
  if (options_state % 2) {
    drawtext(50, 118, "PLAY!", colorarray[5]);
    drawtext(40, 141, "SETTINGS", colorarray[colorcounter % 7]); //hovering over settings
    tft.drawRect(27, 134, 74, 22, colorarray[(colorcounter + 2) % 7]);

  } else {
    drawtext(50, 118, "PLAY!", colorarray[(colorcounter + 1) % 7]); //hovering over play
    drawtext(40, 141, "SETTINGS", colorarray[5]);
    tft.drawRect(27, 111, 74, 22, colorarray[(colorcounter + 2) % 7]);
  }
  if (button_delay > 0) button_delay--; //button delay delays button presses without delaying the music
  colorcounter++;
}

/*
 * settingsScreen
 * Displays the settings screen
 */
void settingsScreen() {
  if (settings_state == 0) {
    tft.fillTriangle(5, 66, 5, 80, 19, 73, colorarray[1]); //hovering over difficulty
    tft.fillTriangle(5, 90, 5, 104, 19, 97, colorarray[0]);
    tft.fillTriangle(5, 117, 5, 130, 19, 123, colorarray[0]);
  
  }
  else if(settings_state == 1){
    tft.fillTriangle(5, 66, 5, 80, 19, 73, colorarray[0]);
    tft.fillTriangle(5, 90, 5, 104, 19, 97, colorarray[1]); //hovering over timer
    tft.fillTriangle(5, 117, 5, 130, 19, 123, colorarray[0]);
    
  } else {
    tft.fillTriangle(5, 66, 5, 80, 19, 73, colorarray[0]);
    tft.fillTriangle(5, 90, 5, 104, 19, 97, colorarray[0]);
    tft.fillTriangle(5, 117, 5, 130, 19, 123, colorarray[1]); //hovering over GAME ON!
  }
  if (button_delay > 0) button_delay--;//button delay delays button presses without delaying the music
}

/*
 * userMove
 * Places a coin in the selected column
 * @param selection, the column selected
 * @param player, the player placing the coin
 * @return the row of the placed column or if there's no space returns -1
 */
int userMove(int selection, int player) {
  if (space(selection) != -1) {
    int row = 0;
    for (row; row < 6; row++) { // from the top to the bottom
      if (row == 5) {
        board[5][selection] = 2 - player;
        break;
      }
      else if (board[row + 1][selection] != 0) {
        board[row][selection] = 2 - player;
        break;
      }

      tft.fillCircle(col_pixels[selection], row_pixels[row], 6, colorarray[5 + player]);
      delay(60);
      tft.fillCircle(col_pixels[selection], row_pixels[row], 6, ST7735_WHITE);

    }
    tft.fillCircle(col_pixels[selection], row_pixels[row], 6, colorarray[5 + player]);
    return row;
  }
  return -1;
}

/*
 * gamePlay
 * Plays both player's chips, checks for wins and ties
 * @param sel, selected column
 */
void gamePlay(int sel) {
  int placed_row = userMove(sel, 1); //place user coin
  if (placed_row == -1) return;
  globalturn++;
  if (winCheck(placed_row, sel, true)) { //check for a win
    ingame = false;
    return;
  }

  aiChoice = AI();
  userMove(aiChoice, 0); //place AI coin
  globalturn++;

  if (winCheck(space(aiChoice) + 1, aiChoice, false)) { //check for a win
    winCheck(space(aiChoice) + 1, aiChoice, true);
    ingame = false;
    return;
  }

  if (globalturn >= 42) { //check for a tie
    ingame = false;
  }

  timer = 0; //reset in game timer
}


void loop() {
  bmpDraw("start.bmp", 0, 0);//branding
  delay(200);
  tone(Speaker, 262, 75); //classic ding noise
  delay(75);
  tone(Speaker, 2093, 225);
  delay(500);

  bmpDraw("introbg.bmp", 0, 0); //draws intro screen

  /// Intro Screen ///
  while (intro) {
    selectButtonState = digitalRead(selectButton);
    downButtonState = digitalRead(downButton);
    upButtonState = digitalRead(upButton);

    //music goes here
    tone(Speaker, pgm_read_byte_near(introSong + introMusicIndex), 75); //plays intro song

    introMusicIndex++;
    if (introMusicIndex == 169) { //loops music
      introMusicIndex = 0;
    }

    introScreen(); //animates intro screen

    if (selectButtonState && !button_delay) { //pressing select
      intro = false;
      if (options_state % 2) {
        settings = true;
      } else {
        ingame = true;
      }
    }
    else if (downButtonState == HIGH && !button_delay) { //going down an option
      tft.drawRect(27, 111, 74, 22, colorarray[5]);
      options_state++;
      button_delay = 4; //game waits 4 delays of 30 milliseconds before letting you press another button
    }
    else if (upButtonState == HIGH && !button_delay) { //going up an option
      tft.drawRect(27, 134, 74, 22, colorarray[5]);
      options_state--;
      button_delay = 4;//game waits 4 delays of 30 milliseconds before letting you press another button
    }
    delay(30);
  }

  /// Settings Screen /// ~ essentially filled with more navigation and options
  if (settings) {
    bmpDraw("settings.bmp", 0, 0);
    drawtext(105, 70, (String)(depth-1), colorarray[1]);
    drawtext(102, 94, (String)(time_limit/20), colorarray[1]);
    
    while (settings) {
      selectButtonState = digitalRead(selectButton);
      downButtonState = digitalRead(downButton);
      upButtonState = digitalRead(upButton);
      
      settingsScreen();
      
      tone(Speaker, pgm_read_byte_near(introSong + introMusicIndex), 75); //plays music

      introMusicIndex++;
      if (introMusicIndex == 169) { //loop music
        introMusicIndex = 0;
      }

      if (selectButtonState && !button_delay) {
        if (settings_state == 2) { //GAME ON!
          settings = false;
          ingame = true;
        } else if (settings_state == 1) { //altering time limit
          time_limit+=100;
          if (time_limit>400) time_limit = 0;
          tft.fillRect(100, 90, 15, 15, colorarray[0]);
          if (!time_limit)drawtext(102, 94, "--", colorarray[1]);
          else drawtext(102, 94, (String)(time_limit/20), colorarray[1]);
          
        } else { //altering difficulty
          depth++;
          if (depth>4)depth = 2;
          tft.fillRect(100, 66, 15, 15, colorarray[0]);
          drawtext(105, 70, (String)(depth-1), colorarray[1]);
          if(depth == 2)bmpDraw("ballg.bmp", 85, 114); 
        }
        button_delay = 4; //same logic as above
      }

      else if (downButtonState == HIGH && !button_delay) {
        if (settings_state == 2){
            settings_state = 0;
        }else{
          settings_state++;
        }
        button_delay = 4;//same logic as above
      }
      else if (upButtonState == HIGH && !button_delay) {
        if (settings_state == 0){
          settings_state = 2;
          }else{ 
        settings_state--;
        }
      button_delay = 4;//same logic as above
      }
      delay(50);
    }
  }
  
  /// In-Game Screen ///
  if (ingame) {
    bmpDraw("board.bmp", 0, 0);
    tft.fillRect(40, 140, 80, 13, colorarray[6]);
    isTimeLimit = time_limit;

    while (ingame) {
      selectButtonState = digitalRead(selectButton);
      leftButtonState = digitalRead(leftButton);
      rightButtonState = digitalRead(rightButton);

      if (button_delay > 0) button_delay--;
        if(isTimeLimit){
          if (timer % 20 == 0) {
            width = 80 - 80 * timer / time_limit;
            tft.fillRect(40 + width, 140, 80 - width, 13, colorarray[5]);
            tft.fillRect(40, 140, width, 13, colorarray[6]);
          }
      if (timer == time_limit) {
        randomSeed(analogRead(A2));
        gamePlay(random(0, 6));
        timer = 0;
      }
      
      tft.fillTriangle(col_pixels[choice] - 6, 10, col_pixels[choice], 22, col_pixels[choice] + 6, 10, ST7735_WHITE); //adjacent integers are coordinates for each point on triangle ~ choice triangle

      if (selectButtonState == HIGH && !button_delay) {
        gamePlay(choice); //does each player's move, checks for wins and ties
        button_delay = 3;
      }

      else if (leftButtonState == HIGH && !button_delay) { //move left
        tft.fillTriangle(col_pixels[choice] - 6, 10, col_pixels[choice], 22, col_pixels[choice] + 6, 10, ST7735_BLACK);
        if (choice == 0)choice = 6;
        else choice--;
        button_delay = 3;
      }
      else if (rightButtonState == HIGH && !button_delay) { //move right
        tft.fillTriangle(col_pixels[choice] - 6, 10, col_pixels[choice], 22, col_pixels[choice] + 6, 10, ST7735_BLACK);
        if (choice == 6)choice = 0;
        else choice++;
        button_delay = 3;
      }

      //music goes here
      tone(Speaker, inGameSong[inGameMusicIndex], 75); //plays objectively cooler in game music

      inGameMusicIndex++;
      if (inGameMusicIndex == 98) { //loops music
        inGameMusicIndex = 0;
      }
      timer++;
      }
      delay(50);
    }

  }

  if (globalturn % 2) { //determines which end screen to draw
    bmpDraw("gameover.bmp", 0, 0);
  } else {
    bmpDraw("winner.bmp", 0, 0);
  }
  while (endgame) { // play again option
    selectButtonState = digitalRead(selectButton);
    if (selectButtonState) {
      resetFunc();
    }
    delay(75);
  }
}


