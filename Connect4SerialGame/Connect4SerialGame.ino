const int selectButton = 3; // the number of the selectButton pin
const int leftButton = 4; // the number of the leftButton pin
const int rightButton = 2; // the number of the rightButton pin


// variables will change:
int selectButtonState = 0; // variable for reading the selectButton status
int leftButtonState = 0; // variable for reading the leftButton status
int rightButtonState = 0; // variable for reading the leftButton status
int board[6][7] = {0}; //the actual board
int choices[7] = {0}; //the array of their current col choice
int turnCheck = 0; //counter that keeps track of the number of turns
int lastMoveRow = 0; // used to keep track of the row of the last placed move
int winner = 0; // the number of the winner
int whereToStart[2]; // an array used in diagonal winning calculations

/* 
 *  Arduino Setup
 *  Essentially sets up buttons, initial choice and draws the blank board
*/
void setup() {
  Serial.begin(9600);
  // initialize the pushbuttons pins as an input:
  pinMode(selectButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  choices[3] = 1;
  drawBoard(board);
}

/* 
 *  Arduino loop
 *  Runs the game, temporarily just delays when someone wins
*/
void loop() {
  // read the state of the pushbutton value:
  selectButtonState = digitalRead(selectButton);
  leftButtonState = digitalRead(leftButton);
  rightButtonState = digitalRead(rightButton);

  if (turnCheck == 42) {
    Serial.println("It's a tie!");
  }

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (selectButtonState == HIGH) {
    Serial.println("Select was Pressed");
    if (spaceForTile(board, findChoice(choices)) == false) { // check for space in the column
      Serial.println("No space in that column!");
      drawBoard(board);
    } else {
//      if (whosTurn(turnCheck) ==2) {
//        lastMoveRow = addTile(board, AI(board,whosTurn(turnCheck)), whosTurn(turnCheck)); //actually add the move and keep track of the row the tile ended up in
//      } else {
        lastMoveRow = addTile(board, findChoice(choices), whosTurn(turnCheck)); //actually add the move and keep track of the row the tile ended up in
//      }
      drawBoard(board);
      if (winCheck(board, lastMoveRow, findChoice(choices))) { //check if someone wins
        winner = turnCheck%2+1;
        Serial.print("Congrats Player ");
        Serial.print(winner);
        Serial.print(", You win!");
        delay(10000000);
      }
      turnCheck++;
    }
  }
  else if (leftButtonState == HIGH) { //move choice left
    Serial.println("Left was Pressed");
    if (findChoice(choices) == 0) { //check if its in range
      Serial.println("Out of Range!");
      drawBoard(board);
    } else {
      goLeft(choices);
      drawBoard(board);
    }
  }
  else if (rightButtonState == HIGH) { //move choice right
    Serial.println("Right was Pressed");
    if (findChoice(choices) == 6) { //check if its in range
      Serial.println("Out of Range!");
      drawBoard(board);
    } else {
      goRight(choices);
      drawBoard(board);
    }
  }
  delay(150); // this is just a reasonable delay for the buttons for now
}

/*
 * AI
 * Recursively chooses the column choice of the AI
 * @param board, the current board state
 * @param player, the current player's turn
 * @return int value of the col number
*/
int AI() { //
  int maximum = -42;
  int value = 0;
  int bestCol = 0;
  for (int i=0;i<7;i++) {
    value = negamax(3, turnCheck);
    if (value >maximum) {
      maximum = value;
      bestCol = i;
    }
  }
  return bestCol;
}
int negamax(int depth, int turns) {
  int innerTurn = turns;
  int row = 0;
  if (depth == 0) {
    return 0;
  }
  if (innerTurn == 42) {
    return 0;
  }

  for (int i =0;i<7;i++) {
    if (spaceForTile(board,i)) {
      row = addTile(board, i, whosTurn(innerTurn));
      if (winCheck(board, row, i)) {
        board[row][i] = 0;
        return (22-innerTurn/2);
      }
      board[row][i] = 0;
    }
  }

  int bestScore = -42;
  int score =-42;

  for (int i =0;i<7;i++) {
    if (spaceForTile(board,i)) {
      row = addTile(board, i, whosTurn(innerTurn));
      int score = -1*negamax(depth-1, innerTurn+1);
      board[row][i] = 0;
      if (score>bestScore) {
        bestScore = score;
      }
    }
  }
  return bestScore; 
}
/*
 * whosTurn
 * Calculates who's turn it is
 * @param turnCheck, the total count of turns so far
 * @return int value representing player 1 or 2
*/
int whosTurn(int turnCheck) {
  if (turnCheck%2==0){
    return 1;
  }
  return 2;
}

/*
 * goLeft
 * Moves choice left
 * @param choices[7], the array of col choices
*/
void goLeft(int choices[7]) {
  int where = findChoice(choices);
  choices[where] = 0;
  choices[where-1] = 1;
}

/*
 * goRight
 * Moves choice right
 * @param choices[7], the array of col choices
*/
void goRight(int choices[7]) {
  int where = findChoice(choices);
  choices[where] = 0;
  choices[where+1] = 1;
}

/*
 * findChoice
 * Calculates which column their choice is
 * @param choices[7], the array of col choices
 * @return int value of the column number
*/
int findChoice(int choices[7]) {
  int whereItIs=0;
  for (int i=0;i<7;i++) {
    if (choices[i] ==1) {
      whereItIs = i;
    }
  }
  return whereItIs;
}

/*
 * spaceForTile
 * Calculates if there's space for that tile in the row
 * @param board[6][7], the board's current state
 * @param choice, col number
 * @return boolean value is true is there is space, false is not
*/
boolean spaceForTile(int board[6][7], int choice) {
  int count = 0;
  for (int i=0;i<6;i++) { // counts up used spaces
    if (board[i][choice] >0) {
      count++;
    }
  }
  if (count <6) {
    return true;
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
int addTile(int board[6][7], int choice, int playerTurn){
  int whereToPutRow = 0;
  for (int i=5; i>=0; i--) { // from the bottom to the top
    if (board[i][choice] <1) {
      whereToPutRow = i;
      break;
    }
  }
  if (playerTurn == 1) {
    board[whereToPutRow][choice] = 1;
  } else {
    board[whereToPutRow][choice] = 2;
  }
  return whereToPutRow;
}

/*
 * winCheck
 * Determines if a win has been formed based on the last move
 * @param board[6][7], the board's current state
 * @param row, row number
 * @param col, col number
 * @return boolean value true if win, false otherwise
*/
boolean winCheck(int board[6][7], int row, int col) {
  int consecCount = 0;
  //hori check
  for (int i =0; i<6;i++) { // counts consecutive values horizontally in given row
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

/*
 * toStart1
 * Determies from where to start checking diagonally from top left to bottom right
 * @param row, row number
 * @param col, col number
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

/*
 * drawBoard
 * Draws the board and where the current choice is
 * @param board[6][7], the board's current state
*/
void drawBoard(int board[6][7]) {
  for (int i=0; i<7;i++) {
    Serial.print(choices[i]);
    Serial.print(" ");
  }
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
