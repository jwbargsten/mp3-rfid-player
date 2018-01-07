#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h>
#include <SFEMP3Shield.h>
#include <stdio.h>
#include <string.h>                      
#include <stdlib.h>     

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

  for(int i = 0; i < TAGS_MAX; i++)      
    authorizedTags[i] = NULL;      

  if (!file.open("TAGS.TXT", O_READ)) return 0;

  while ((n = file.fgets(line, sizeof(line))) > 0) {
    tstart = strchr(line, SEP_CHAR);     
    lend = strchr(line, '\n');           
    if(tstart && lend) {                 
      tstart++;                          
      if(*(lend - 1) == '\r') lend--;       
      int idx = atoi(line);              
      fprintf(stderr,"idx: %d ", idx);   
      if(idx >= TAGS_MAX || idx < 0)     
        continue;
      tlen = lend - tstart;
      tmp = (char *)malloc( (tlen+1) * sizeof(char));
      
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

  Serial.begin(9600);

  if (!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");
  if(!initAuthorizedTags()) sd.errorHalt("tag init failed");
  result = MP3player.begin();

  MP3player.setVolume(10, 10);
  
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
    delay(100);
    //MP3player.stopTrack();
    counter = -1;
    
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
        delay(100);
        break;
      } else if (counter >= 0 && counter < maxTagLength) {
        tagId[counter] = data1;
        ++counter;
      } 
    }
  }
}

int tagToTrack() {
  for (int i = 0; i < TAGS_MAX; ++i) {
    if(authorizedTags == NULL)
      continue;
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
