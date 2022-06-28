#include <Esplora.h>
#include <SPI.h>
#include <SD.h>
#include <TFT.h>            // Arduino LCD library


#define SD_CS    8         // Chip select line for SD card in Esplora

#define qSize 15
#define playerPosition 10

enum gameState {start,game,fail};
enum dinoState {land,jumping,midair,falling};

struct dino
{
  dinoState prevState = falling;
  dinoState state = land;
  int prevScore;
  int score = 0;
  int topScore = 0;
  unsigned int start;
  PImage character;
  unsigned int airStart;
  void jump()
  {
    if(state == land)
    {
      prevState = land;
      state = jumping;
      airStart = millis();
    }
  }
  void fall()
  {
    if(state == midair)
      {
        state = falling;
        prevState = midair;
      }
    else
      {
        prevState = falling;
        state = land;
      }
  }
};

struct node
{
  int pos;
  node * next;
  node(int val)
  {
    pos = val;
    next = nullptr;
  }
};

struct q
{
  PImage obstacle;
  node * start = nullptr;
  node * last = nullptr;
  void push(int val)
  {
    if(last == nullptr)
    {
      last = new node(val);
      start = last;
      return;
    }
    last->next = new node(val);
    last = last->next;
  }
  void pop()
  {
    node * tmp = start;
    start = start->next;
    delete tmp;
  }
  void move()
  {
    node * tmp = start;
    while (tmp->next)
    {
      tmp->pos -= 5;
      tmp = tmp->next;
    }
  }
  void clear()
  {
    while(start != nullptr)
    {
      node * tmp = start;
      start = start->next;
      delete tmp;
    }
    start = nullptr;
    last = nullptr;
  }
};



dino Player;
gameState currState = start;
q cactuses;
int cactusesGap; //gap between cactuses, 30 by start of the game, decreases with time

void gravity()
  {
    switch(Player.state)
    {
      case (jumping):
        if(millis()-Player.airStart > 500)
          {
            Player.prevState = jumping;
            Player.state = midair;
          }
        break;
      case (midair):
        if(millis()-Player.airStart > 1000)
          Player.fall();
      case (falling):
         if(millis()-Player.airStart > 1500)
          Player.fall();
      default: return;
    }
  }


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  EsploraTFT.begin(); 
  SD.begin(SD_CS);

  EsploraTFT.background(255,255,255);
  EsploraTFT.stroke(10,10,10);
  EsploraTFT.fill(255,255,255);

  if (!SD.begin(SD_CS)) 
  {
     EsploraTFT.text("No SD card inserted!",0,0);
      return;
  }
  if (!SD.exists("dino.bmp"))
  {
    EsploraTFT.text("No character!",0,0);
      return;
  }

  Player.character = EsploraTFT.loadImage("dino.bmp");
  if(!Player.character.isValid())
  {
    EsploraTFT.text("Invalid character picture!",0,0);
      return;
  }

  if (!SD.exists("obstacle.bmp"))
  {
    EsploraTFT.text("No obstacle!",0,0);
      return;
  }

  cactuses.obstacle = EsploraTFT.loadImage("obstacle.bmp");
  if(!Player.character.isValid())
  {
    EsploraTFT.text("Invalid obstacle picture!",0,0);
      return;
  }
  
  cactusesGap = 60;
}



void loop() {
  // put your main code here, to run repeatedly:
  int button = Esplora.readButton(SWITCH_UP);
  if(currState == start)
  {
    EsploraTFT.text("**Press button to start!**",0,60);
    if(!button)
    {
      currState = game;
      Player.start = millis();
      printScene(); 
      cactusesGap = 60;
      for(int i = 0; i < qSize; ++i)
        cactuses.push(150+i*cactusesGap);
    }
  }
  if(currState == game)
  {
    gravity();
    updateScene();
    Player.score = (millis() - Player.start) / 1000;
    if(Player.score != Player.prevScore)
    {
      updateScore();
      Player.prevScore = Player.score; 
      if (cactusesGap > 20)
        cactusesGap -= (Player.score/10);
    }
    if( !button )
      Player.jump();
    if ( touchesCactus() )
    {
      cactuses.clear();
      showFailScene();
      currState = fail;
    }
  }
  if(currState == fail)
  {
    if(Player.score > Player.topScore)
      Player.topScore = Player.score;
    if(!button)
    {
      currState = game;
      Player.start = millis();
      printScene(); 
      cactusesGap = 60;
      for(int i = 0; i < qSize; ++i)
        cactuses.push(150+i*cactusesGap);
    }
  }
}

void printScene()
{
  EsploraTFT.background(255,255,255);
  EsploraTFT.text("___________________________",0,80);
  EsploraTFT.text("Score:", 50, 10);
  EsploraTFT.text("Best:",105,10);
  char topScore[4];
  itoa(Player.topScore,topScore,10); // converts int data type to string data type
  EsploraTFT.text(topScore,135,10);
}

bool touchesCactus ()
{

  if(cactuses.start->pos == playerPosition && Player.state != midair)
    return true;
  
  if(cactuses.start->pos < 5)
  {
    cactuses.pop();
    cactuses.push(cactuses.last->pos + cactusesGap);
  }  
  return false;
}

void updateScore()
{
  EsploraTFT.stroke(255,255,255);
  EsploraTFT.fill(255,255,255);
  char score[4];
  itoa(Player.score,score,10); // converts int data type to string data type
  EsploraTFT.rect(85,10,20,20);
  EsploraTFT.stroke(10,10,10);
  EsploraTFT.text(score, 85,10);
}

void showFailScene()
{
  EsploraTFT.background(255,255,255);
  EsploraTFT.text("Score:", 50, 80);
  char score[4];
  itoa(Player.score,score,10); // converts int data type to string data type
  EsploraTFT.text(score, 85,80);
  EsploraTFT.text("Press button to start!",15,60);
}

void drawCactuses()
{
  node * tmp = cactuses.start;
  while(tmp->pos < 160)
  {  
    EsploraTFT.image(cactuses.obstacle,tmp->pos,68);
    tmp = tmp->next;
  }
  cactuses.move();
}

void updateScene()
{
  EsploraTFT.stroke(255,255,255);
  EsploraTFT.fill(255,255,255);
  EsploraTFT.rect(160,43,150,43);

  if(Player.state != Player.prevState)
  {
    EsploraTFT.rect(10,50,20,36);
    switch (Player.state)
    {
      case(land):
        EsploraTFT.image(Player.character,10,68);
        break;
      case(midair):
        EsploraTFT.image(Player.character,10,50);
        break;
      default:
        EsploraTFT.image(Player.character,10,59);  
    }
  }
   drawCactuses();
   EsploraTFT.stroke(10,10,10);
}
