#include <MIDI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "InputDebounce.h"
#include <EEPROM.h>

struct Preset {
  char lineOne[20];
  char lineTwo[20];
  int whammy;
};

const int savedBankAddress = 0;
const int savedPatchAddress = 2;
const char *files[8] = { "/BANK_0.P", "/BANK_1.P", "/BANK_2.P", "/BANK_3.P", "/BANK_4.P", "/BANK_5.P", "/BANK_6.P", "/BANK_7.P" };
const int buttonPins[8] = { 3, 4, 0, 0, 0, 0, 0, 0 };
InputDebounce inputButtons[8];
short int currentBank = 0;
short int currentPresetIndex = 0;
Preset currentPreset;

MIDI_CREATE_DEFAULT_INSTANCE();

LiquidCrystal_I2C lcd (0x27, 20, 4);

void setup() {
  InitLCD();

  for (int i = 0; i < 2; i++) { // TODO: update when we have more buttons
    pinMode(buttonPins[i], INPUT);
    inputButtons[i].registerCallbacks(NULL, LoadPreset, NULL, NULL);
    inputButtons[i].setup(buttonPins[i], DEFAULT_INPUT_DEBOUNCE_DELAY, InputDebounce::PIM_EXT_PULL_DOWN_RES);
  }

  currentBank = EEPROM.read(savedBankAddress);
  currentPresetIndex = EEPROM.read(savedPatchAddress);

  SD.begin(BUILTIN_SDCARD);
  LoadFile(files[currentBank], currentPresetIndex);

  MIDI.begin(MIDI_CHANNEL_OFF);

  PrintPresetToLCD(currentPreset, currentPresetIndex);
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 2; i++) { // TODO: update when we have more buttons
    inputButtons[i].process(now); 
  }
  
  delay(1); // [ms]
}

void LoadPreset(uint8_t pinIn) {
  currentPresetIndex = FindIndex(pinIn);

  if (currentPresetIndex == -1) {
    PrintError();
    return;
  }
  
  LoadFile(files[currentBank], currentPresetIndex);
  MIDI.sendProgramChange((currentBank * 8) + currentPresetIndex, 1);
  PrintPresetToLCD(currentPreset, currentPresetIndex);
  EEPROM.update(savedPatchAddress, currentPresetIndex);
}

int FindIndex(int element) {
  for (int i = 0; i < 8; i++) {
    if (buttonPins[i] == element) return i;
  }
  return -1;
}

void LoadFile(const char *filename, const int patchNo) {
  File file = SD.open(filename, FILE_READ);
  
  StaticJsonDocument<1024> doc;

  DeserializationError error = deserializeJson(doc, file);
  if (error){
      lcd.print(error.f_str());
  } else {
    currentPreset.whammy = doc["root"][patchNo]["whammy"] | 44;
    strlcpy(currentPreset.lineOne,
          doc["root"][patchNo]["lineOne"] | "Patch not created!",
          sizeof(currentPreset.lineOne));
    strlcpy(currentPreset.lineTwo,
          doc["root"][patchNo]["lineTwo"] | "",
          sizeof(currentPreset.lineTwo));
  }
  file.close();
}

void PrintError() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Error loading");
  lcd.setCursor(0, 1);
  lcd.print("preset");
}

void PrintPresetToLCD(Preset &preset, int presetIndex){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("0");
  lcd.print(currentBank + 1);
  lcd.print(" - 0");
  lcd.print(presetIndex + 1);
  lcd.setCursor(0, 1);
  lcd.print(preset.lineOne);
  lcd.setCursor(0, 2);
  lcd.print(preset.lineTwo);
  ChangeBank(0);
}

void ChangeBank(int direction){
  currentBank = (currentBank + direction) % 8;
  EEPROM.update(savedBankAddress, currentBank);
  lcd.setCursor(0, 3);
  lcd.print("Next Bank: ");
  lcd.print(currentBank + 1);
}

void InitLCD() {
  lcd.init();
  lcd.backlight();
  lcd.print("Booting...");
}
