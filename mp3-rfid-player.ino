/**
   \file MP3Shield_Library_Demo.ino

   \brief Example sketch of using the MP3Shield Arduino driver, demonstrating all methods and functions.
   \remarks comments are implemented with Doxygen Markdown format

   \author Bill Porter
   \author Michael P. Flaga

   This sketch listens for commands from a serial terminal (like the Serial
   Monitor in the Arduino IDE). If it sees 1-9 it will try to play an MP3 file
   named track00x.mp3 where x is a number from 1 to 9. For eaxmple, pressing
   2 will play 'track002.mp3'. A lowe case 's' will stop playing the mp3.
   'f' will play an MP3 by calling it by it's filename as opposed to a track
   number.

   Sketch assumes you have MP3 files with filenames like "track001.mp3",
   "track002.mp3", etc on an SD card loaded into the shield.
*/

#include <SPI.h>

//Add the SdFat Libraries
#include <SdFat.h>
#include <FreeStack.h>

//and the MP3 Shield Library
#include <SFEMP3Shield.h>

// Below is not needed if interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_Timer1
#include <TimerOne.h>
#elif defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer
#include <SimpleTimer.h>
#endif

#include <SoftwareSerial.h>
SoftwareSerial RFID(3, 4); // RX and TX

/**
   \brief Object instancing the SdFat library.

   principal object for handling all SdCard functions.
*/
SdFat sd;

/**
   \brief Object instancing the SFEMP3Shield library.

   principal object for handling all the attributes, members and functions for the library.
*/
SFEMP3Shield MP3player;


const int nKnownTags = 6;
char* authorizedTags[nKnownTags]; // array to hold the list of authorized tags

// fills the list of authorzied tags
void initAuthorizedTags() {
  // add your own tag IDs here
  authorizedTags[0] = "4700658674D0";
  authorizedTags[1] = "470065C46680";
  authorizedTags[2] = "4A0096EEDDEF";
  authorizedTags[3] = "010A3B6E9CC2";
  authorizedTags[4] = "010A3ACE56A9";
  authorizedTags[5] = "010A3B621F4D";

}

//------------------------------------------------------------------------------
/**
   \brief Setup the Arduino Chip's feature for our use.

   After Arduino's kernel has booted initialize basic features for this
   application, such as Serial port and MP3player objects with .begin.
   Along with displaying the Help Menu.

   \note returned Error codes are typically passed up from MP3player.
   Whicn in turns creates and initializes the SdCard objects.

   \see
   \ref Error_Codes
*/
void setup() {

  uint8_t result; //result code from some function as to be tested at later time.

  Serial.begin(115200);

  Serial.print(F("F_CPU = "));
  Serial.println(F_CPU);
  Serial.print(F("Free RAM = ")); // available in Version 1.0 F() bases the string to into Flash, to use less SRAM.
  Serial.print(FreeStack(), DEC);  // FreeStack() is provided by SdFat
  Serial.println(F(" Should be a base line of 1028, on ATmega328 when using INTx"));
  //pinMode(53, OUTPUT);
  Serial.print(F("sdsel code: "));
  Serial.println(SD_SEL);

  //Initialize the SdCard.
  if (!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  // depending upon your SdCard environment, SPI_HAVE_SPEED may work better.
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");

  //Initialize the MP3 Player Shield
  result = MP3player.begin();
  //check result, see readme for error codes.
  if (result != 0) {
    Serial.print(F("Error code: "));
    Serial.print(result);
    Serial.println(F(" when trying to start MP3 player"));
    if ( result == 6 ) {
      Serial.println(F("Warning: patch file not found, skipping.")); // can be removed for space, if needed.
      Serial.println(F("Use the \"d\" command to verify SdCard can be read")); // can be removed for space, if needed.
    }
  }

#if defined(__BIOFEEDBACK_MEGA__) // or other reasons, of your choosing.
  // Typically not used by most shields, hence commented out.
  Serial.println(F("Applying ADMixer patch."));
  if (MP3player.ADMixerLoad("admxster.053") == 0) {
    Serial.println(F("Setting ADMixer Volume."));
    MP3player.ADMixerVol(-3);
  }
#endif
  MP3player.setVolume(10, 10);
  RFID.begin(9600);    // start serial to RFID reader
  initAuthorizedTags();
}

//------------------------------------------------------------------------------
/**
   \brief Main Loop the Arduino Chip

   This is called at the end of Arduino kernel's main loop before recycling.
   And is where the user's serial input of bytes are read and analyzed by
   parsed_menu.

   Additionally, if the means of refilling is not interrupt based then the
   MP3player object is serviced with the availaible function.

   \note Actual examples of the libraries public functions are implemented in
   the parse_menu() function.
*/

int data1 = 0;
int tagLength = 0;
const int maxTagLength = 15;
int newtag[maxTagLength];
char tagId[maxTagLength];
int counter = -1;
void loop() {

  // Below is only needed if not interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) \
    && ( (USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer) \
    ||   (USE_MP3_REFILL_MEANS == USE_MP3_Polled)      )

  MP3player.available();
#endif

  //   if( MP3player.getState() == playback) {
  //      MP3player.pauseMusic();
  //      Serial.println(F("Pausing"));
  //    } else if( MP3player.getState() == paused_playback) {
  //      MP3player.resumeMusic();
  //      Serial.println(F("Resuming"));
  //    } else {
  //      Serial.println(F("Not Playing!"));
  //    }



  if (RFID.available() > 0)
  {
    Serial.print("/");
    counter = -1;
    // read tag numbers
    delay(300); // needed to allow time for the data to come in from the serial buffer.
    int n = 0;
    while ((data1 = RFID.read()) >= 0) {
      Serial.print((char)data1);
      if (data1 == 2) {
        counter = 0;


      } else if (data1 == 3 && counter < maxTagLength) {
        tagLength = counter;


        counter = -1;
        Serial.print("TAG: ");
        int j;
        for (int j = 0; j < tagLength; j++) {
          Serial.print(tagId[j]);
        }
        //Serial.println(tagId);
        Serial.println("|");
        int trackNum = tagToTrack();
        if(trackNum >= 0) {
        playSong(trackNum);
        } else {
          Serial.println("du kommst hier nicht rein");
        }
        clearRFID();
        delay(2000);

        
        break;
      } else if (counter >= 0 && counter < maxTagLength) {
        tagId[counter] = data1;
        //newtag[counter] = data1;

        ++counter;

      } 

      n++;
    }
    //RFID.flush(); // stops multiple reads
    delay(100);
  }
}

int tagToTrack() {
  int i;

  for (i = 0; i < nKnownTags; ++i) {
    if (strncmp(authorizedTags[i], tagId, tagLength) == 0) {
      return i;
    }
  }
  return -1;
}

void clearRFID() {
  while (RFID.read() >= 0) {
    ; // do nothing
  }
}
char trackName[256];
int lastTrack = -1;
void playSong(int trackNum) {

  uint8_t result;

  sprintf(trackName,"track%03d.mp3",trackNum);
  
  Serial.println(trackName);
  if ( MP3player.getState() != playback || trackNum != lastTrack) {
    Serial.println("playing song");
    MP3player.stopTrack();
    result = MP3player.playMP3(trackName);
    
      if (result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
        lastTrack = trackNum;
      }
    
  }
  Serial.print(";");
}




