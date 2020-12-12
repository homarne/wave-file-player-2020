// Based on Teensy Simple WAV file player example
// Reads list of files from SD card
// And plays them in a random shuffle
// Resuffling after every unique file has been played
// To do: Read .wav files only
// To do: Some way to select either short test files for debugging, or ignore test files
// To do: Next and Prev buttons
// To do: Some sort of beat/temo extraction with beat indicator via serial port
//
// Three types of output may be used, by configuring the code below.
//
//   1: Digital I2S - Normally used with the audio shield:
//         http://www.pjrc.com/store/teensy3_audio.html
//
//   2: Digital S/PDIF - Connect pin 22 to a S/PDIF transmitter
//         https://www.oshpark.com/shared_projects/KcDBKHta
//
//   3: Analog DAC - Connect the DAC pin to an amplified speaker
//         http://www.pjrc.com/teensy/gui/?info=AudioOutputAnalog
//
// To configure the output type, first uncomment one of the three
// output objects.  If not using the audio shield, comment out
// the sgtl5000_1 lines in setup(), so it does not wait forever
// trying to configure the SGTL5000 codec chip.
//
// The SD card may connect to different pins, depending on the
// hardware you are using.  Uncomment or configure the SD card
// pins to match your hardware.
//
// Data files to put on your SD card can be downloaded here:
//   http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
//
// This example code is in the public domain.

#include "audio_files.h"

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

File root;

AudioPlaySdWav           playWav1;
// Use one of these 3 output types: Digital I2S, Digital S/PDIF, or Analog DAC
AudioOutputI2S           audioOutput;
//AudioOutputSPDIF       audioOutput;
//AudioOutputAnalog      audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
//#define SDCARD_CS_PIN    4
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

//file format is 8.3 so 12 characters plus null termination character
#define MAX_FILES 100
char file_list[MAX_FILES][13]; // names of audio files found on SD card
int file_ref[MAX_FILES]; // file references - used for shuffle
int file_count; // used in main loop to maintian count of unique files that have been played
int file_num; // used in main loop to as index to currently playing file name
int files_available = SONG_FILES;  // actual number of songs aviable
                                  // initialized here from number of files specified in static list in audio_files.h
                                 // later updated with count of audio files read from disk

void setup() {
  Serial.begin(9600);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(8);

  // Comment these out if not using the audio adaptor board.
  // This may wait forever if the SDA & SCL pins lack
  // pullup resistors
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

//  root = SD.open("/");
//  printDirectory(root, 0);
//  Serial.println("directory read done!!!!!!");

  //this will apply the current value of the voume potentiometer as the random seed
  unsigned long seed = analogRead(15);
  randomSeed(seed);

  //intialize file pointer array
  for (int i = 0; i < MAX_FILES; i++) {
    file_ref[i] = i;
  }
  
  //Serial.println("INTERNAL files....");
  //printFileList(files, files_available);
  //delay(1000);
  
  root = SD.open("/");
  files_available = readFileList(root, 0, file_list, MAX_FILES);
  
//  printFileList(file_list, files_available);
//  delay(1000);
}

void loop() {
  //  playFile("SDTEST1.WAV");  // filenames are always uppercase 8.3 format
  //  delay(500);
  //  playFile("SDTEST2.WAV");
  //  delay(500);
  //  playFile("SDTEST3.WAV");
  //  delay(500);
  //  playFile("SDTEST4.WAV");
  //  delay(1500);

  file_num = 0; // index to currently playing file
  file_count = 0; // count of unique files that have been played
  intArrayShuffle(file_ref, files_available);
  Serial.println("start list");
  while (file_count < files_available) {
    file_num = file_ref[file_count];
    playFile(file_list[file_num]);
    //playFile(files[file_num]);
    file_count += 1;
    delay(1000);
  }
}

void playFile(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.println(filename);

  // Start playing the file.  
  // This sketch continues to run while the file plays.
  playWav1.play(filename);

  // A brief delay for the library read WAV info
  delay(5);

  // Simply wait for the file to finish playing.
  while (playWav1.isPlaying()) {
    //     uncomment these lines if you audio shield
    //     has the optional volume pot connected
    float vol = analogRead(15);
    vol = vol / 1024;
    sgtl5000_1.volume(vol);
  }
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

// walk through array and randomly swap current location with rndom location
void intArrayShuffle(int target[], int target_size) {
  int random_location = 0;
  int random_value = 0;

  for (int i = 0; i < target_size; i++) {
    random_location = random(0, target_size - 1);
    random_value = target[random_location];
    target[random_location] = target[i];
    target[i] = random_value;
  }
}

//print all entires in array of 8.3 (12 characters + null termination) file names
void printFileList(char file_list[][13], int list_size) {
  Serial.println("Printing file list...");
  for (int i = 0; i < list_size; i++) {
    Serial.print("File num: ");
    Serial.print(i);
    Serial.print(" name: ");
    Serial.println(file_list[i]);
  }
}

int readFileList(File dir, int numTabs, char file_list[][13], int max_files) {
  Serial.println("Identifying audio files to list...");
  int file_count = 0;
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      Serial.println("**nomorefiles**");
      break;
    }
    //Serial.println(entry.name());
    if (entry.isDirectory()) {
    //  Serial.println("Skipping directory");
    } else {
    // files have sizes, directories do not
    //  Serial.print("Audio file identified: ");
    //  Serial.println(entry.name());
      strncpy(file_list[file_count], entry.name(), 13);
      //file_list[file_count] = entry.name();
      file_count += 1;
    }
    entry.close();
    //delay(500);
    
    if (file_count >= max_files) {
      break;
    }
  }
  Serial.print("Audio files found:  ");
  Serial.println(file_count);
  return file_count;
}

//https://stackoverflow.com/questions/21147838/arduino-sd-file-extension
bool isFnMusic(char* filename) {
  int8_t len = strlen(filename);
  bool result;
  if (  strstr(strlwr(filename + (len - 4)), ".mp3")
        || strstr(strlwr(filename + (len - 4)), ".aac")
        || strstr(strlwr(filename + (len - 4)), ".wma")
        || strstr(strlwr(filename + (len - 4)), ".wav")
        || strstr(strlwr(filename + (len - 4)), ".fla")
        || strstr(strlwr(filename + (len - 4)), ".mid")
        // and anything else you want
     ) {
    result = true;
  } else {
    result = false;
  }
  return result;
}
