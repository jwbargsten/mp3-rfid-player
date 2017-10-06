#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h>
#include <SFEMP3Shield.h>

SdFat sd;
SFEMP3Shield MP3player;

const int nKnownTags = 6;
char* authorizedTags[nKnownTags];

void initAuthorizedTags() {
  authorizedTags[0] = "4700658674D0";
  authorizedTags[1] = "470065C46680";
  authorizedTags[2] = "4A0096EEDDEF";
  authorizedTags[3] = "010A3B6E9CC2";
  authorizedTags[4] = "010A3ACE56A9";
  authorizedTags[5] = "010A3B621F4D";
}

void setup() {
  uint8_t result;

  Serial.begin(9600);

  if (!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");

  result = MP3player.begin();

  MP3player.setVolume(10, 10);
  initAuthorizedTags();
}

int data1 = 0;
int tagLength = 0;
const int maxTagLength = 15;
char tagId[maxTagLength];
int counter = -1;

void loop() {

  // Below is only needed if not interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) \
    && ( (USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer) \
    ||   (USE_MP3_REFILL_MEANS == USE_MP3_Polled)      )

  MP3player.available();
#endif

  if(Serial.available() > 0) {
    MP3player.stopTrack();
    counter = -1;
    delay(100);
    while ((data1 = Serial.read()) >= 0) {
      if (data1 == 2) {
        counter = 0;
      } else if (data1 == 3 && counter < maxTagLength) {
        tagLength = counter;
        counter = -1;
        int trackNum = tagToTrack();
        if(trackNum >= 0) {
          playSong(trackNum);
        } else {
          /*Serial.println("du kommst hier nicht rein");*/
        }
        clearSerial();
        delay(1000);
        break;
      } else if (counter >= 0 && counter < maxTagLength) {
        tagId[counter] = data1;
        ++counter;
      } 
    }
    delay(100);
  }
}

int tagToTrack() {
  for (int i = 0; i < nKnownTags; ++i) {
    if (strncmp(authorizedTags[i], tagId, tagLength) == 0) {
      return i;
    }
  }
  return -1;
}

void clearSerial() {
  while (Serial.read() >= 0) {
    ; // do nothing
  }
}

char trackName[256];
int lastTrack = -1;

void playSong(int trackNum) {
  uint8_t result;

  sprintf(trackName,"track%03d.mp3",trackNum);
  
  if ( MP3player.getState() != playback || trackNum != lastTrack) {
    MP3player.stopTrack();
    result = MP3player.playMP3(trackName);
    
      if (result == 0) {
        lastTrack = trackNum;
      }
  }
}
