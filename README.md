<img src="https://user-images.githubusercontent.com/31244240/43902863-1e9abfde-9bb9-11e8-86b9-c3cef55818bb.png" width="440" height="536" /> <img src="https://user-images.githubusercontent.com/31244240/43902910-38f83e88-9bb9-11e8-8998-ab6d4a8c946d.png" width="440" height="536" /> 
<img src="https://user-images.githubusercontent.com/31244240/43902892-3068355c-9bb9-11e8-867e-37af075c791b.png" width="440" height="536" /> <img src="https://user-images.githubusercontent.com/31244240/43902876-27c79618-9bb9-11e8-9871-de80ce3d416f.png" width="440" height="536" /> 

# Connect-Boy
The Connect-Boy is a handheld Connect 4, single-player game that uses the Negamax Algorithm to make moves. The algorithm, (depending on the difficulty level selected by the player), recursively attempts every hypothetical move for n turns in the future. It then generates a score for each move (magnitude depends on the number of turns to a win, sign depends on whose turn results in a win).

Under the hood, the Connect-Boy consists of an ATmega328 microcontroller, and other components (a 16 MHz clock, decoupling capacitors, a 5V voltage regulator) that allow the circuit to function as an Arduino Uno. The move which would result in the highest score is executed.

This project was created in C, and the game graphics made with the [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library). 
