#include <LedControl.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// declaring pins and values for joystick
const int joystickPinSw = 2;  // digital pin connected to switch output
const int joystickPinX = A0;  // A0 - analog pin connected to X output
const int joystickPinY = A1;  // A1 - analog pin connected to Y output
const int buzzerPin = A4;
const int buzzerFrequency = 1000;
const int upperLimit = 900;
const int lowerLimit = 300;
const int defaultThresholdLower = 450;
const int defaultThresholdUpper = 650;
const byte maxSnakeLength = 30;
const byte timeBetweenEachTextScroll = 150;
const byte left = 0;
const byte right = 1;
const byte up = 3;
const byte down = 4;
const int longPressThreshold = 2000;
byte previousSwReading = 1;
byte swState = LOW;
int xValueJoystick = 0;
int yValueJoystick = 0;
int lastXState = 0;
int lastYState = 0;
const int defaultBuzzerState = -1;
const byte buzzerPlayState = 0;
int buzzerState = defaultBuzzerState;

// declaring pins and values for matrix
const byte dinPin = 12;
const byte clockPin = 11;
const byte loadPin = 10;
const byte matrixSize = 8;
const byte defaultFacing = 7;
LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);  //DIN, CLK, LOAD, No. DRIVER
int foodYPosition = random(1, 7);
int foodXPosition = random(1, 7);
byte facing = defaultFacing;
byte prevFacing = defaultFacing;

// declaring pins and values for lcd
const byte rs = 9;
const byte en = A5;
const byte d4 = 7;
const byte d5 = 8;
const byte d6 = 3;
const byte d7 = 4;
const int LCDcontrastPin = 6;
const int LCDContrastMaxValue = 125;
const int LCDBrightnessPin = 5;
const int LCDbrightnessMaxValue = 255;
const int matrixbrightnessMaxValue = 15;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// declaring time values
const unsigned long debounceTime = 50;
const unsigned long moveHeadInterval = 450;
const unsigned long foodBlinkInterval = 1000;
int buzzerSoundDuration = 300;
unsigned long lastButtonInputChange = 0;
unsigned long lastTextScroll = 0;
unsigned long lastHeadMove = 50;
unsigned long lastFoodBlink = 0;
unsigned long upTime = 0;
unsigned long buzzerNoteStart = 0;

const int gameOverState = -1;
const byte gameState = 0;
const byte menuState = 1;
const byte settingsState = 2;
const byte highscoreState = 3;
const byte aboutState = 4;
const byte howToPlayState = 5;
const byte greetingState = 6;
const byte settingsClickedState = 7;
const byte gamePausedState = 8;
const byte numberOfHighscores = 5;
bool currentBlinkState = true;
byte currentMenuPosition = 0;
byte currentSettingsPosition = 0;
byte currentHighscorePosition = 0;
int appState = greetingState;
int score = 0;

// declaring menu items
const byte menuItemsNumber = 5;
const byte settingsItemsNumber = 8;

const String menuItems[menuItemsNumber] = {
  "1.Start Game",
  "2.Highscore",
  "3.Settings",
  "4.About",
  "5.How to play"
};

const String settingsItems[settingsItemsNumber] = {
  "1.Enter name",
  "2.Level",
  "3.LCD_Ct",
  "4.LCD_Br",
  "5.M_Br",
  "6.Sound",
  "7.Walls",
  "8.Back"
};

int highScores[numberOfHighscores];

const byte matrixGameOver[matrixSize] = {
  0b11000011,
  0b11100111,
  0b01111110,
  0b00111100,
  0b00111100,
  0b01111110,
  0b11100111,
  0b11000011
};

const byte matrixDefault[matrixSize] = {
  0b00000000,
  0b01100110,
  0b01100110,
  0b00000000,
  0b01000010,
  0b01111110,
  0b00111100,
  0b00000000
};

int longTextCurrentPosition = 0;

// game data
String userName;
byte nameLength = 0;
byte gameDifficulty = 0;
byte LCDContrast = 0;
byte LCDBrightness = 0;
byte MTXBrightness = 0;
bool sound = false;
byte walls = 0;

byte snakeYPositions[maxSnakeLength];
byte snakeXPositions[maxSnakeLength];
int currentHeadPosition = maxSnakeLength - 1;
int currentSnakeLength = 1;

// initialize addresses
const byte nameStartAddress = 0;
const byte nameEndAddress = 9;
const byte stringLengthAddress = 10;
const byte gameDifficultyAddress = 11;
const byte LCDContrastAddress = 12;
const byte LCDBrightnessAddress = 13;
const byte MTXBrightnessAddress = 14;
const byte soundAddress = 15;
const byte wallsAddress = 16;
const byte startingHighscoresAddress = 25;
const byte maxNameSpacesOccupied = 10;
const byte scoreSpacesOccupied = 4;
const byte nameLengthSpacesOccupied = 1;
const byte positionsNeededForOneHighScore = 4;
const int menuNoteDuration = 120;
const int eatingNoteDuration = 300;
const int dyingNoteDuration = 1000;
const byte eatingNote = 49;
const int dyingNote = 277;
const int menuMovementNote = 2800;
const int menuButtonPressNote = 1500;
const String scoreTitle = "Score: ";
int buzzerNote = 0;


void buzzerBeepingSound() {
  tone(buzzerPin, buzzerNote);
  if (millis() - buzzerNoteStart > buzzerSoundDuration) {
    noTone(buzzerPin);
    buzzerState = defaultBuzzerState;
  }
}

void setMatrixToDefaultDisplay() {
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize; j++) {
      lc.setLed(0, i, j, bitRead(matrixDefault[i], 7 - j));
    }
  }
}

void initializePinModes() {
  pinMode(joystickPinSw, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
}

void saveNameToEEPROM(byte lengthAddress, byte length, String name, byte nameStartAddress) {
  EEPROM.update(lengthAddress, length);
  for (byte i = nameStartAddress; i < nameStartAddress + length; i++)
    EEPROM.update(i, name[i - nameStartAddress]);
}

String readNameFromEEPROM(byte userNameLengthAddress, byte nameStartAddress) {
  byte userNameLength;
  EEPROM.get(userNameLengthAddress, userNameLength);
  char name[userNameLength];
  for (byte i = nameStartAddress; i < nameStartAddress + userNameLength; i++) {
    name[i - nameStartAddress] = EEPROM.read(i);
  }
  return String(name).substring(0, userNameLength);
}

void saveNewHighscoresToEEPROM() {
  for (int i = 0; i < numberOfHighscores; i++) {
    const int startingAddressForIHighScore = startingHighscoresAddress + i * positionsNeededForOneHighScore;
    EEPROM.put(startingAddressForIHighScore, highScores[i]);
  }
}

void getHighScoresFromEEPROM() {
  for (int i = 0; i < numberOfHighscores; i++) {
    const int startingAddressForIHighScore = startingHighscoresAddress + i * positionsNeededForOneHighScore;
    EEPROM.get(startingAddressForIHighScore, highScores[i]);
    if (!highScores[i]) {
      highScores[i] = 0;
    }
  }
}

void readGameDetailsFromEEPROM() {
  EEPROM.get(gameDifficultyAddress, gameDifficulty);
  EEPROM.get(LCDContrastAddress, LCDContrast);
  EEPROM.get(LCDBrightnessAddress, LCDBrightness);
  EEPROM.get(MTXBrightnessAddress, MTXBrightness);
  EEPROM.get(soundAddress, sound);
  EEPROM.get(wallsAddress, walls);


  analogWrite(LCDcontrastPin, LCDContrast);
  analogWrite(LCDBrightnessPin, LCDBrightness);
  lc.setIntensity(0, MTXBrightness);
}

void changeUserName() {
  if (Serial.available()) {
    String serialInput = Serial.readString();
    serialInput.trim();
    nameLength = (byte)serialInput.length();
    if (nameLength) {
      if (nameLength > maxNameSpacesOccupied) {
        serialInput = serialInput.substring(0, maxNameSpacesOccupied);
        nameLength = maxNameSpacesOccupied;
      }
      userName = serialInput;
      saveNameToEEPROM(stringLengthAddress, nameLength, userName, nameStartAddress);
      appState = settingsState;
      printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, false);
    }
  }
}

void printGreetingMessage() {
  printFirstAndSecondLineForLCD("    Welcome!    ", "*Press to start*");
}

void printItemsForCurrentPosition(String items[], String title, int position, bool replaceNumberForClicked) {
  lcd.clear();
  lcd.print(title);
  lcd.setCursor(0, 1);
  if (replaceNumberForClicked) {
    String itemToDisplay = items[position];
    String replaceValue = String(position + 1) + ".";
    itemToDisplay.replace(replaceValue, ">");
    lcd.print(itemToDisplay);
    return;
  }
  lcd.print(items[position]);
}

void printFirstAndSecondLineForLCD(String firstLine, String secondLine) {
  lcd.clear();
  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
}

void valueLeftDefaultRange(byte movement) {
  switch (appState) {
    case gameState:
      {
        if (facing - movement == 1 || movement - facing == 1) {
          return;
        }
        facing = movement;
        return;
      }

    case menuState:
      {
        implementMenuMovement(movement);
        return;
      }
    case settingsState:
      {
        implementSettingsMovement(movement);
        return;
      }
    case highscoreState:
      {
        implementHighscoresMovement(movement);
        return;
      }
    case aboutState:
      {
        const int maxLength = 23;
        implementAboutAndHowToPlayMovement(movement, maxLength);
        return;
      }
    case howToPlayState:
      {
        const int maxLength = 24;
        implementAboutAndHowToPlayMovement(movement, maxLength);
        return;
      }
    case settingsClickedState:
      {
        implementSettingsClickedMovement(movement);
        return;
      }
  }
}

void implementAboutAndHowToPlayMovement(byte movement, byte maxLength) {
  switch (movement) {
    case left:
      {
        if (longTextCurrentPosition == 0 || millis() - lastTextScroll < timeBetweenEachTextScroll) {
          return;
        }
        lastTextScroll = millis();
        longTextCurrentPosition--;
        lcd.scrollDisplayRight();
        break;
      }
    case right:
      {
        if (longTextCurrentPosition == maxLength || millis() - lastTextScroll < timeBetweenEachTextScroll) {
          return;
        }
        longTextCurrentPosition++;
        lastTextScroll = millis();
        lcd.scrollDisplayLeft();
        break;
      }
  }
}

void implementMenuMovement(byte movement) {
  menuSound(menuMovementNote);
  switch (movement) {
    case up:
      {
        if (currentMenuPosition == 0) {
          currentMenuPosition = menuItemsNumber;
        }
        currentMenuPosition--;
        printItemsForCurrentPosition(menuItems, "     -MENU-     ", currentMenuPosition, false);
        break;
      }
    case down:
      {
        if (currentMenuPosition == menuItemsNumber - 1) {
          currentMenuPosition = -1;
        }
        currentMenuPosition++;
        printItemsForCurrentPosition(menuItems, "     -MENU-     ", currentMenuPosition, false);
      }
  }
}

void implementSettingsMovement(byte movement) {
  menuSound(menuMovementNote);
  switch (movement) {
    case up:
      {
        if (currentSettingsPosition == 0) {
          currentSettingsPosition = settingsItemsNumber;
        }
        currentSettingsPosition--;
        printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, false);
        break;
      }
    case down:
      {
        if (currentSettingsPosition == settingsItemsNumber - 1) {
          currentSettingsPosition = -1;
        }
        currentSettingsPosition++;
        printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, false);
        break;
      }
  }
  completeSettingsWithValues();
}

void implementSettingsClickedMovement(byte movement) {
  menuSound(menuMovementNote);
  switch (movement) {
    case up:
      {
        switch (currentSettingsPosition) {
          case 1:
            {
              if (gameDifficulty == 2) {
                return;
              }
              gameDifficulty++;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              EEPROM.update(gameDifficultyAddress, gameDifficulty);
              break;
            }
          case 2:
            {
              if (LCDContrast == LCDContrastMaxValue) {
                return;
              }
              LCDContrast += 5;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              analogWrite(LCDcontrastPin, LCDContrast);
              EEPROM.update(LCDContrastAddress, LCDContrast);
              break;
            }
          case 3:
            {
              if (LCDBrightness == LCDbrightnessMaxValue) {
                return;
              }
              LCDBrightness += 15;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              analogWrite(LCDBrightnessPin, LCDBrightness);
              EEPROM.update(LCDBrightnessAddress, LCDBrightness);
              break;
            }
          case 4:
            {
              if (MTXBrightness == matrixbrightnessMaxValue) {
                return;
              }
              MTXBrightness++;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              lc.setIntensity(0, MTXBrightness);
              EEPROM.update(MTXBrightnessAddress, MTXBrightness);
              break;
            }
          case 5:
            {
              sound = !sound;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              EEPROM.update(soundAddress, sound);
              break;
            }
          case 6:
            {
              walls = !walls;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              EEPROM.update(wallsAddress, walls);
              break;
            }
        }
        break;
      }
    case down:
      {
        switch (currentSettingsPosition) {
          case 1:
            {
              if (gameDifficulty == 0) {
                return;
              }
              gameDifficulty--;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              EEPROM.update(gameDifficultyAddress, gameDifficulty);
              break;
            }
          case 2:
            {
              if (LCDContrast == 0) {
                return;
              }
              LCDContrast -= 5;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              analogWrite(LCDcontrastPin, LCDContrast);
              EEPROM.update(LCDContrastAddress, LCDContrast);
              break;
            }
          case 3:
            {
              if (LCDBrightness == 0) {
                return;
              }
              LCDBrightness -= 15;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              analogWrite(LCDBrightnessPin, LCDBrightness);
              EEPROM.update(LCDBrightnessAddress, LCDBrightness);
              break;
            }
          case 4:
            {
              if (MTXBrightness == 0) {
                return;
              }
              MTXBrightness--;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              lc.setIntensity(0, MTXBrightness);
              EEPROM.update(MTXBrightnessAddress, MTXBrightness);
              break;
            }
          case 5:
            {
              sound = !sound;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              EEPROM.update(soundAddress, sound);
              break;
            }
          case 6:
            {
              walls = !walls;
              printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, true);
              EEPROM.update(wallsAddress, walls);
              break;
            }
        }
        break;
      }
  }
  completeSettingsWithValues();
}

void implementHighscoresMovement(byte movement) {
  menuSound(menuMovementNote);
  switch (movement) {
    case up:
      {
        if (currentHighscorePosition == 0) {
          return;
        }
        currentHighscorePosition--;
        String title = "Position: " + String(currentHighscorePosition + 1);
        printFirstAndSecondLineForLCD(title, scoreTitle);
        lcd.print(highScores[currentHighscorePosition]);

        break;
      }
    case down:
      {
        if (currentHighscorePosition == numberOfHighscores - 1) {
          return;
        }
        currentHighscorePosition++;
        String title = "Position: " + String(currentHighscorePosition + 1);
        printFirstAndSecondLineForLCD(title, scoreTitle);
        lcd.print(highScores[currentHighscorePosition]);
        break;
      }
  }
}

void completeSettingsWithValues() {
  switch (currentSettingsPosition) {
    case 1:
      {
        lcd.print(": ");
        if (gameDifficulty == 0) {
          lcd.print("Easy");
          break;
        }
        if (gameDifficulty == 1) {
          lcd.print("Medium");
          break;
        }
        lcd.print("Hard");
        break;
      }
    case 2:
      {
        lcd.print(": " + String(LCDContrast));
        break;
      }
    case 3:
      {
        lcd.print(": " + String(LCDBrightness));
        break;
      }
    case 4:
      {
        lcd.print(": " + String(MTXBrightness));
        break;
      }
    case 5:
      {
        lcd.print(": ");
        if (sound == 1)
          lcd.print("On");
        else lcd.print("Off");
        break;
      }
    case 6:
      {
        lcd.print(": ");
        if (walls == 1)
          lcd.print("On");
        else lcd.print("Off");
        break;
      }
  }
}

void implementXMovement(bool continous = false) {
  if (xValueJoystick > upperLimit) {
    if (lastXState != 1 || continous) {
      valueLeftDefaultRange(right);
    }
    lastXState = 1;
    return;
  }
  if (xValueJoystick < lowerLimit) {
    if (lastXState != -1 || continous) {
      valueLeftDefaultRange(left);
    }
    lastXState = -1;
    return;
  }
  if (xValueJoystick > defaultThresholdLower && xValueJoystick < defaultThresholdUpper) {
    lastXState = 0;
  }
}

void implementYMovement(bool continous = false) {
  if (yValueJoystick > upperLimit) {
    if (lastYState != 1 || continous) {
      valueLeftDefaultRange(down);
    }
    lastYState = 1;
    return;
  }
  if (yValueJoystick < lowerLimit) {
    if (lastYState != -1 || continous) {
      valueLeftDefaultRange(up);
    }
    lastYState = -1;
    return;
  }
  if (yValueJoystick > defaultThresholdLower && yValueJoystick < defaultThresholdUpper) {
    lastYState = 0;
  }
}

void implementButtonPress() {
  if (previousSwReading != swState && swState == LOW) {
    lastButtonInputChange = millis();
  }

  if (lastButtonInputChange && previousSwReading != swState && swState == HIGH) {
    if (millis() - lastButtonInputChange < debounceTime) {
      return;
    }
    lastButtonInputChange = 0;
    handleButtonPress(false);
  }

  if (lastButtonInputChange && millis() - lastButtonInputChange > longPressThreshold) {
    lastButtonInputChange = 0;
    handleButtonPress(true);
  }

  previousSwReading = swState;
}

void takeInputBasedOnState() {
  if (appState == aboutState || appState == howToPlayState) {
    implementXMovement(true);
    return;
  }
  if (appState == gameState) {
    implementXMovement(false);
    implementYMovement(false);
    return;
  }

  if (appState != 0) {
    implementYMovement(false);
    return;
  }
}

void menuSound(int note) {
  buzzerState = buzzerPlayState;
  buzzerNote = note;
  buzzerSoundDuration = menuNoteDuration;
  buzzerNoteStart = millis();
}

void readJoystickInput() {
  swState = digitalRead(joystickPinSw);
  xValueJoystick = analogRead(joystickPinX);
  yValueJoystick = analogRead(joystickPinY);
  takeInputBasedOnState();
  implementButtonPress();
}

void handleButtonPress(bool longButtonPress) {
  menuSound(menuButtonPressNote);
  if (appState == menuState) {
    implementMenuButtonPress();
    return;
  }
  if (appState == settingsState || appState == settingsClickedState) {
    implementSettingsButtonPress();
    return;
  }
  if (appState == greetingState) {
    appState = menuState;
    printItemsForCurrentPosition(menuItems, "     -MENU-     ", currentMenuPosition, false);
    return;
  }
  if (appState == aboutState || appState == howToPlayState) {
    appState = menuState;
    printItemsForCurrentPosition(menuItems, "     -MENU-     ", currentMenuPosition, false);
    longTextCurrentPosition = 0;
    return;
  }
  if (appState == highscoreState) {
    appState = menuState;
    printItemsForCurrentPosition(menuItems, "     -MENU-     ", currentMenuPosition, false);
    currentHighscorePosition = 0;
    return;
  }
  if (appState == gameOverState) {
    appState = menuState;
    setMatrixToDefaultDisplay();
    printItemsForCurrentPosition(menuItems, "     -MENU-     ", currentMenuPosition, false);
    return;
  }
  if (appState == gameState) {
    if (longButtonPress) {
      gameOver();
      return;
    }
    appState = gamePausedState;
    printFirstAndSecondLineForLCD("  Game  Paused  ", "Press to resume!");
    return;
  }
  if (appState == gamePausedState) {
    if (longButtonPress) {
      gameOver();
      return;
    }
    appState = gameState;
    printNameAndCurrentScore();
    return;
  }
}


void printNameAndCurrentScore() {
  printFirstAndSecondLineForLCD("Name: " + userName, scoreTitle);
  lcd.print(score);
}

void implementMenuButtonPress() {
  switch (currentMenuPosition) {
    case 0:
      {
        printNameAndCurrentScore();
        appState = gameState;
        for (int row = 0; row < matrixSize; row++) {
          for (int col = 0; col < matrixSize; col++) {
            lc.setLed(0, row, col, false);
          }
        }
        updateMatrix();
        return;
      }
    case 1:
      {
        appState = highscoreState;
        printFirstAndSecondLineForLCD("Position: " + String(currentHighscorePosition + 1), scoreTitle + highScores[currentHighscorePosition]);
        return;
      }
    case 2:
      {
        appState = settingsState;
        printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, false);
        return;
      }
    case 3:
      {
        appState = aboutState;
        printFirstAndSecondLineForLCD("Creator: Safta Adelin Gabriel", "Github: adesafta2002 | Game Name: Snake");
        return;
      }
    case 4:
      {
        appState = howToPlayState;
        printFirstAndSecondLineForLCD("1.Move using joystick & eat food to grow", "2.Avoid walls and your tail!");
        return;
      }
  }
}

void implementSettingsButtonPress() {
  if (appState != settingsClickedState) {
    appState = settingsClickedState;
  } else {
    appState = settingsState;
  }
  switch (currentSettingsPosition) {
    case 0:
      {
        if (appState == settingsClickedState) {
          printFirstAndSecondLineForLCD("  -Enter Name-  ", "*Type in serial*");
        } else {
          printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, false);
        }
        return;
      }
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      {
        const bool replaceWithArrow = appState == settingsClickedState ? true : false;
        printItemsForCurrentPosition(settingsItems, "   -SETTINGS-   ", currentSettingsPosition, replaceWithArrow);
        completeSettingsWithValues();
        return;
      }
    case 7:
      {
        appState = menuState;
        currentSettingsPosition = 0;
        printItemsForCurrentPosition(menuItems, "     -MENU-     ", currentMenuPosition, false);
        return;
      }
  }
}

void updateMatrix() {
  lc.setLed(0, snakeYPositions[currentHeadPosition], snakeXPositions[currentHeadPosition], true);
}

void updateHighScores() {
  if (score <= highScores[numberOfHighscores - 1]) {
    printFirstAndSecondLineForLCD("   Game  Over   ", scoreTitle + String(score));
    score = 0;
    return;
  }
  printFirstAndSecondLineForLCD(" New  Highscore ", scoreTitle + String(score));

  int insertPosition = 255;

  for (int i = 0; i < numberOfHighscores; i++) {
    if (highScores[i] < score) {
      insertPosition = i;
      break;
    }
  }
  for (int i = numberOfHighscores - 1; i > insertPosition; i--) {
    highScores[i] = highScores[i - 1];
  }
  highScores[insertPosition] = score;
  saveNewHighscoresToEEPROM();
  score = 0;
}

void gameOver() {
  appState = gameOverState;
  currentHeadPosition = maxSnakeLength - 1;
  currentSnakeLength = 1;
  snakeYPositions[currentHeadPosition] = 0;
  snakeXPositions[currentHeadPosition] = 0;
  facing = defaultFacing;
  updateHighScores();
  buzzerState = buzzerPlayState;
  buzzerNote = dyingNote;
  buzzerSoundDuration = dyingNoteDuration;
  buzzerNoteStart = millis();
  for (byte i = 0; i < matrixSize; i++) {
    for (byte j = 0; j < matrixSize; j++) {
      lc.setLed(0, i, j, bitRead(matrixGameOver[i], 7 - j));
    }
  }
  return;
}

void updateSnakePositions(bool snakeAte, byte snakeHeadYPosition, byte snakeHeadXPosition) {
  if (snakeAte) {
    currentHeadPosition--;
    snakeXPositions[currentHeadPosition] = snakeHeadXPosition;
    snakeYPositions[currentHeadPosition] = snakeHeadYPosition;
    currentSnakeLength++;
    updateMatrix();
    setFoodPosition();
    buzzerState = buzzerPlayState;
    buzzerNote = eatingNote;
    buzzerSoundDuration = eatingNoteDuration;
    buzzerNoteStart = millis();
    return;
  }
  lc.setLed(0, snakeYPositions[maxSnakeLength - 1], snakeXPositions[maxSnakeLength - 1], false);
  for (int i = maxSnakeLength - 1; i > currentHeadPosition; i--) {
    snakeXPositions[i] = snakeXPositions[i - 1];
    snakeYPositions[i] = snakeYPositions[i - 1];
  }
  snakeXPositions[currentHeadPosition] = snakeHeadXPosition;
  snakeYPositions[currentHeadPosition] = snakeHeadYPosition;
  updateMatrix();
}

bool snakeAteItself(byte snakeHeadYPosition, byte snakeHeadXPosition) {
  for (int i = maxSnakeLength - 1; i > currentHeadPosition; i--) {
    if (snakeXPositions[i] == snakeHeadXPosition && snakeYPositions[i] == snakeHeadYPosition) {
      return true;
    }
  }
  return false;
}

void setFoodPosition() {
  bool exit = false;
  while (!exit) {
    foodYPosition = random(1, 7);
    foodXPosition = random(1, 7);
    exit = true;
    for (int i = maxSnakeLength - 1; i >= currentHeadPosition; i--) {
      if (snakeXPositions[i] == foodXPosition && snakeYPositions[i] == foodYPosition) {
        exit = false;
        break;
      }
    }
  }
}

void changeBlinkState() {
  currentBlinkState = !currentBlinkState;
  lastFoodBlink = millis();
  lc.setLed(0, foodYPosition, foodXPosition, currentBlinkState);
}

void blinkFoodPosition() {
  if (currentBlinkState == true && millis() - lastFoodBlink > foodBlinkInterval / (gameDifficulty + 1)) {
    changeBlinkState();
    return;
  }
  if (currentBlinkState == false && millis() - lastFoodBlink > foodBlinkInterval * (gameDifficulty / 3 + 0.5)) {
    changeBlinkState();
    return;
  }
}

void implementGameLogic() {
  blinkFoodPosition();
  if (facing == defaultFacing) {
    return;
  }

  if (millis() - lastHeadMove > moveHeadInterval / (gameDifficulty + 1)) {
    int snakeHeadYPosition = snakeYPositions[currentHeadPosition];
    int snakeHeadXPosition = snakeXPositions[currentHeadPosition];
    byte snakeAte = 0;
    switch (facing) {
      case up:
        {
          snakeHeadYPosition--;
          if (snakeHeadYPosition < 0) {
            if (walls == 1) {
              gameOver();
              return;
            }
            snakeHeadYPosition = matrixSize - 1;
          }
          break;
        }
      case down:
        {
          snakeHeadYPosition++;
          if (snakeHeadYPosition > 7) {
            if (walls == 1) {
              gameOver();
              return;
            }
            snakeHeadYPosition = 0;
          }
          break;
        }
      case right:
        {
          snakeHeadXPosition++;
          if (snakeHeadXPosition > 7) {
            if (walls == 1) {
              gameOver();
              return;
            }
            snakeHeadXPosition = 0;
          }
          break;
        }
      case left:
        {
          snakeHeadXPosition--;
          if (snakeHeadXPosition < 0) {
            if (walls == 1) {
              gameOver();
              return;
            }
            snakeHeadXPosition = matrixSize - 1;
          }
          break;
        }
    }
    if (snakeHeadYPosition == foodYPosition && snakeHeadXPosition == foodXPosition) {
      score += 1 * (gameDifficulty + 1);
      if (walls) {
        score++;
      }
      lc.setLed(0, foodYPosition, foodXPosition, false);
      snakeAte = 1;
    }
    updateSnakePositions(snakeAte, snakeHeadYPosition, snakeHeadXPosition);
    updateMatrix();
    if (snakeAteItself(snakeHeadYPosition, snakeHeadXPosition)) {
      gameOver();
      return;
    }
    printFirstAndSecondLineForLCD("Name: " + userName, scoreTitle);
    lcd.print(score);
    lastHeadMove = millis();
  }
}

void setup() {
  Serial.begin(9600);
  initializePinModes();
  lcd.begin(16, 2);
  lc.shutdown(0, false);
  lc.clearDisplay(0);
  printGreetingMessage();
  userName = readNameFromEEPROM(stringLengthAddress, nameStartAddress);
  readGameDetailsFromEEPROM();
  randomSeed(analogRead(A3));
  setMatrixToDefaultDisplay();
  snakeYPositions[currentHeadPosition] = 0;
  snakeXPositions[currentHeadPosition] = 0;
  getHighScoresFromEEPROM();
}

void loop() {
  readJoystickInput();
  if (buzzerState != defaultBuzzerState && sound) {
    buzzerBeepingSound();
  }
  if (appState == settingsClickedState && currentSettingsPosition == 0) {
    changeUserName();
  }
  if (appState == gameState) {
    implementGameLogic();
  }
}