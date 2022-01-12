#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h>
#include <SFEMP3Shield.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/wdt.h>

SdFat sd;
SFEMP3Shield MP3player;

const char SEP_CHAR = ':';
const int TAGS_MAX = 100;
char* authorizedTags[TAGS_MAX];
const size_t LINE_DIM = 30;


int initAuthorizedTags()
{
  SdFile file;
  char line[LINE_DIM];
  int n;
  int tlen = 0;
  char *tstart, *lend, *tmp;

  for (int i = 0; i < TAGS_MAX; i++)
    authorizedTags[i] = NULL;

  if (!file.open("TAGS.TXT", O_READ)) return 0;

  while ((n = file.fgets(line, sizeof(line))) > 0) {
    tstart = strchr(line, SEP_CHAR);
    lend = strchr(line, '\n');
    if (tstart && lend) {
      tstart++;
      if (*(lend - 1) == '\r') lend--;
      int idx = atoi(line);
      fprintf(stderr, "idx: %d ", idx);
      if (idx >= TAGS_MAX || idx < 0)
        continue;
      tlen = lend - tstart;
      tmp = (char *)malloc( (tlen + 1) * sizeof(char));

      strncpy(tmp, tstart, tlen);
      tmp[tlen] = '\0';
      authorizedTags[idx] = tmp;

      // authorizedTags[idx] = strndup(tstart, tlen * sizeof(char));
    }
  }
  file.close();
  return 1;
}


void setup() {
  uint8_t result;
  wdt_enable(WDTO_8S);
  Serial.begin(9600);

  if (!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");
  if (!initAuthorizedTags()) sd.errorHalt("tag init failed");
  result = MP3player.begin();

  MP3player.setVolume(10, 10);
}

int data1 = 0;
int tagLength = 0;
const int maxTagLength = 15;
char tagId[maxTagLength + 1];
int counter = -1;

void loop() {

  // Below is only needed if not interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) \
    && ( (USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer) \
    ||   (USE_MP3_REFILL_MEANS == USE_MP3_Polled)      )

  MP3player.available();
#endif

  if (Serial.available() > 0) {
    delay(100);
    //MP3player.stopTrack();

    // 3C5175AE
    // . 20 00BD2F14 A6;
    // .=start byte,  20=head, 00BD2F14=card nr, A6=chksum
    // counter needs to skip start byte and the head = 3
    counter = -3;
    while ((data1 = Serial.read()) >= 0) {
  
      if (data1 == 2) {
        // start byte
        counter = -2;
      } else if (data1 == 3 && counter > 2 && counter < maxTagLength) {
        tagLength = min(counter, 8);
        counter = -3;
        tagId[tagLength] = '\0';
        int trackNum = tagToTrack();
        if (trackNum >= 0) {
          playSong(trackNum);
        } else {
          /*Serial.println("du kommst hier nicht rein");*/
        }
        clearSerial();
        delay(100);
        break;
      } else if (counter >= 0 && counter < maxTagLength) {
        tagId[counter] = data1;
        ++counter;
      } else if (counter > -3 && counter < 0) {
        // the head
        counter++;
      }
    }
  }
  wdt_reset();
}

int tagToTrack() {
  int result;

  for (int i = 0; i < TAGS_MAX; ++i) {
    if (authorizedTags == NULL)
      continue;
    if (strncmp(authorizedTags[i], tagId, tagLength) == 0) {
      return i;
    }
  }
  //Serial.println("du kommst hier nicht rein1");

  //Serial.println("du kommst hier nicht rein2");
  SdFile file;

  if (!file.open("UNKNOWN.TXT", O_CREAT | O_APPEND | O_WRITE)) return -1;
  // if (!file.open("UNKNOWN.TXT", O_RDWR | O_CREAT | O_AT_END)) return -1;
  noInterrupts();
  //Serial.println("du kommst hier nicht rein3");
  result = file.write(tagId, tagLength);
  file.write('\n');
  //Serial.println("du kommst hier nicht rein4");
  file.sync();
  file.close();
  interrupts();
  playBeep();

  delay(3000);

  // Serial.println("du kommst hier nicht rein5");
  return -1;
}

void clearSerial() {
  while (Serial.read() >= 0) {
    ; // do nothing
  }
}

int lastTrack = -1;

void playSong(int trackNum) {
  uint8_t result;

  char trackName[16];
  sprintf(trackName, "track%03d.mp3", trackNum);

  if ( MP3player.getState() != playback || trackNum != lastTrack) {
    MP3player.stopTrack();
    result = MP3player.playMP3(trackName);


    if (result == 0) {
      lastTrack = trackNum;
    }
  }
}

void playBeep() {
  uint8_t result;
  if ( MP3player.getState() != playback) {
    result = MP3player.playMP3("BEEP.MP3");
  }

}
