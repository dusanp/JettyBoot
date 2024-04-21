#include "MCUFRIEND_kbv.h"
MCUFRIEND_kbv tft;
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>
SoftwareSerial mySerial(13, 12); // RX, TX
DFRobotDFPlayerMini myMP3;
#include "bitmaps.h"
#include <EEPROM.h>

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
//#define GREEN   0x6748
#define GREEN   0x07E0
//#define GREEN_2 0x8E8E
#define GREEN_2 0x6748
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0x8410
#define ORANGE  0xE880

#define MAX_FPS 30
#define SCROLL_SPEED 1
#define SCREEN_TOP 120
#define SCREEN_BOTTOM 380
#define PILLAR_WIDTH 22
#define PILLAR_MIN_HEIGHT 30
#define GAP_SIZE 62
#define MENU_SCENE 0
#define GAME_SCENE 1
#define SCROLL_SPEED 10
#define GRAVITY 0.00063
#define JUMP_SPEED -0.17
#define LINE_WIDTH 4
#define RES_X 320
#define RES_Y 480
#define GAME_RES_Y (SCREEN_BOTTOM-SCREEN_TOP)
#define X_GAP 135
#define PILLAR_COUNT 3
#define PILLAR_CAP_HEIGHT 10
#define BOOT_X 140
#define SCORE_HEIGHT 50
#define BOOT_HEIGHT 15
#define BOOT_WIDTH 15
#define MAX_BOOT_SPEED 1 
#define JUMP_DEBOUNCE_DELAY 50
#define IDLE_MUSIC_TRACK 1
#define PLAYING_MUSIC_TRACK 2
#define VICTORY_MUSIC_TRACK 4
#define LOSS_MUSIC_TRACK 3

// 'jettyBootSmall', 15x15px
const unsigned char epd_bitmap_jettyBootSmall [] PROGMEM = {
  0xff, 0x80, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xfe, 0xf0, 
  0xff, 0xf8, 0xff, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0x00, 0x00, 0xff, 0xfe, 0xee, 0xee
};

void setup()
{
    Serial.begin(9600);
    uint16_t ID = tft.readID();

    tft.setCursor(0, 0);
    tft.begin(ID);
    tft.setRotation(2);
    tft.setFont(&FreeMonoBold12pt7b);
    tft.setTextColor(GREEN); 
    tft.fillScreen(BLACK);
    delay(3000); //DFPlayer needs some time to boot up?
    mySerial.begin(9600);
    if (myMP3.begin(mySerial, true)){
        Serial.println(F("inited"));
    }else{
        Serial.println(F("failed to init"));
    }
    
    pinMode(10, INPUT);
    myMP3.volume(15);
}

int getPillarHeight(){
  return random(PILLAR_MIN_HEIGHT,GAME_RES_Y-PILLAR_MIN_HEIGHT-GAP_SIZE);
}

short scene = MENU_SCENE;

void loop(void)
{
  tft.fillScreen(BLACK);
  switch (scene) {
    case MENU_SCENE:
      drawMenu();
      break;
    case GAME_SCENE:
      playGame();
    default:
      // statements
      break;
  }
}

void playGame(){
    unsigned long gameStart=millis();
    unsigned long lastUpdate=millis();
    int yBootPos = (GAME_RES_Y/2)-(BOOT_HEIGHT/2);
    double bootSpeed = 0;
    int xPositions[] = {RES_X+X_GAP, RES_X+(X_GAP*2), RES_X+(X_GAP*3)};
    int yHeights[] = {getPillarHeight(), getPillarHeight(), getPillarHeight()};
    byte score = 0;
    unsigned long bootDebounceTime=millis();
    bool isJumpHeld = false;

    gameSetup();

    while(true){
      int delta = millis()-lastUpdate;
      lastUpdate=millis();

      tft.fillRect(BOOT_X, getAbsY(yBootPos), 15, BOOT_HEIGHT, BLACK); //TODO make more efficient cleanup
      //BOOT movement
      if(digitalRead(10) && !isJumpHeld){
        bootSpeed = JUMP_SPEED;
        isJumpHeld = true;
        bootDebounceTime = lastUpdate + JUMP_DEBOUNCE_DELAY;
      }else{
        bootSpeed += delta*GRAVITY;
        if(!digitalRead(10) && lastUpdate>=bootDebounceTime){
          isJumpHeld = false;
        }
      }
      if(bootSpeed>MAX_BOOT_SPEED){
        bootSpeed=MAX_BOOT_SPEED;
      }
      if(millis()<gameStart+500){
        bootSpeed=0;
      }
      yBootPos+=delta*bootSpeed;

      // DRAW BOOT
      tft.drawBitmap(BOOT_X, getAbsY(yBootPos), epd_bitmap_jettyBootSmall, BOOT_WIDTH, BOOT_HEIGHT, WHITE);
      
      // Pillar movement
      int moveBy = delta/SCROLL_SPEED;
      for (byte i = 0; i < PILLAR_COUNT; i = i + 1) {
        int height1 = yHeights[i];
        int yPos2 = height1+GAP_SIZE;
        int height2 = GAME_RES_Y-yPos2;

        int oldXPos = xPositions[i];
        int xPos = xPositions[i]-= moveBy;

        if(oldXPos > BOOT_X && xPos <= BOOT_X){
          score += 1;
          tft.fillRect(240, SCORE_HEIGHT+1, 100, 40, BLACK);
          tft.setCursor(240,SCORE_HEIGHT+30);
          tft.print(score);
        }

        tft.fillRect(xPos,getAbsY(0), moveBy, height1, GREEN);
        tft.fillRect(xPos,getAbsY(height1-PILLAR_CAP_HEIGHT), moveBy, PILLAR_CAP_HEIGHT, GREEN_2);
        tft.fillRect(xPos,getAbsY(yPos2), moveBy, height2, GREEN);
        tft.fillRect(xPos,getAbsY(yPos2), moveBy, PILLAR_CAP_HEIGHT, GREEN_2);
  
        tft.fillRect(xPos+PILLAR_WIDTH, getAbsY(0), moveBy, height1, BLACK);
        tft.fillRect(xPos+PILLAR_WIDTH, getAbsY(yPos2), moveBy, height2, BLACK);

        //Note: this logic has an edge case if PILLAR_WIDTH is smaller than BOOT_WIDTH
        if((BOOT_X > xPos && BOOT_X < xPos+PILLAR_WIDTH) || (BOOT_X+BOOT_WIDTH > xPos && BOOT_X+BOOT_WIDTH < xPos+PILLAR_WIDTH)){
          if(height1>yBootPos || yPos2<yBootPos+BOOT_HEIGHT){
            bootSpeed=0;
            myMP3.play(LOSS_MUSIC_TRACK);
            delay(5000);
            if(score>EEPROM.read(0)){
              newHighScore(score);
            }
            scene = MENU_SCENE;
            goto out;
          }
        }
      }



      //check if boot is in bounds
      if (yBootPos+BOOT_HEIGHT>GAME_RES_Y || yBootPos<=0){
        bootSpeed=0;
        myMP3.play(LOSS_MUSIC_TRACK);
        delay(5000);
        if(score>EEPROM.read(0)){
          newHighScore(score);
        }
        scene = MENU_SCENE;
        goto out;
      }

      

      for (byte i = 0; i < PILLAR_COUNT; i = i + 1) {
        if(xPositions[i]<-PILLAR_WIDTH){
          int previousIndex = i-1;
          if(previousIndex<0){previousIndex=PILLAR_COUNT-1;}
          xPositions[i] = xPositions[previousIndex]+X_GAP;
          yHeights[i] = getPillarHeight();
        }
      }


      Serial.println(millis()-lastUpdate);
      while((millis()-lastUpdate) < (1000/MAX_FPS)){
        delay(1);
      }    
    }
    out:
    Serial.println("loss");
}

void gameSetup(){
    myMP3.loop(PLAYING_MUSIC_TRACK);
    tft.setCursor(25,SCORE_HEIGHT);
    tft.print("HIGHSCORE  |  SCORE");
    tft.setCursor(190,SCORE_HEIGHT);
    tft.setCursor(80,SCORE_HEIGHT+30);
    tft.print(EEPROM.read(0));
    tft.fillRect(0,SCREEN_TOP-LINE_WIDTH, RES_X, LINE_WIDTH, GREEN);
    tft.fillRect(0,SCREEN_BOTTOM, RES_X, LINE_WIDTH, GREEN);
}

void gameEnd(byte score){
  
}

void newHighScore(byte score){ 
    EEPROM.update(0, score);
    myMP3.play(VICTORY_MUSIC_TRACK);
    tft.fillScreen(BLACK);
    tft.setCursor(60,160);
    tft.print("NEW HIGH SCORE!");
    tft.setCursor(150,220);
    tft.print(score);
        
    while(true){
      delay(1);
      if(digitalRead(10)){
        scene = MENU_SCENE;
        break;
      }
  }
}

int getAbsX(int x){
  return x;
}

int getAbsY(int y){
  return y+SCREEN_TOP;
}

void drawMenu(){
  myMP3.play(IDLE_MUSIC_TRACK);
  tft.drawBitmap(30,80,epd_bitmap_jettyMenuText, 276,61, GREEN);
  tft.drawBitmap(115,160,epd_bitmap_jettyMenuBoot, 104,136, WHITE);  
  tft.setCursor(70,387);
  tft.setFont(&FreeMonoBold9pt7b);
  tft.print("INSERT 5 CREDITS");
  tft.setCursor(0,0);
  tft.setFont(&FreeMonoBold12pt7b);

  while(true){
//    tft.drawBitmap(75,263,epd_bitmap_jettyMenuPuff, 114,107, WHITE);
//    delay(1500);
//    tft.drawBitmap(75,263,epd_bitmap_jettyMenuPuff, 114,107, BLACK);
//    delay(1500);
    delay(1);
    if(digitalRead(10)){
      scene = GAME_SCENE;
      break;
    }
  }

}
