#include <SPI.h>
#include <Wire.h>
#include <MIDI.h>
#include <EEPROM.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiOut); 

#define MIDI_CHANNEL   1
#define MAX_BTN     6 // 버튼수 ( scene 5개 + tuner 1 )

////////////////////////////////////////////////////////////////////////

// TUNER
#define BTN_0_PORT    2   // 버튼 1
#define BTN_1_PORT    3
#define BTN_2_PORT    4
#define BTN_3_PORT    5
#define BTN_4_PORT    6
#define TUNNER_PORT     7   // 튜너?

// led
#define BTN_0_LED_PORT    8     // 버튼1 LED
#define BTN_1_LED_PORT    9
#define BTN_2_LED_PORT    10
#define BTN_3_LED_PORT    11
#define BTN_4_LED_PORT    12
#define TUNNER_LED_PORT     13

// Scene CC / Value
#define SCENE_CC      35    // Value가 0이면 Scene1 , 1이면 Scene2
// Value
#define SCENE_0   0 // Scene 1
#define SCENE_1   1 // Scene 2
#define SCENE_2   2 // Scene 3
#define SCENE_3   3 // Scene 4
#define SCENE_4   4 // Scene 5

// MIDI CC / Value
#define TUNER_CC      33
#define TUNER_ON      127
#define TUNER_OFF     0

  
////////////////////////////////////////////////////////////////////////

#define KEY_STATE_NONE          0
#define KEY_STATE_PRESS         1
#define KEY_STATE_LONGPRESS     2
#define KEY_STATE_RELEASE       3

// channel + bank
int btnState[MAX_BTN] = { KEY_STATE_NONE, KEY_STATE_NONE, KEY_STATE_NONE, KEY_STATE_NONE, KEY_STATE_NONE, KEY_STATE_NONE };
unsigned long pressTime[MAX_BTN] = { 0, 0, 0, 0, 0, 0 };

unsigned long infoTimer = 0;
bool on_tunner = false;
int lastselect = -1;

void SetFM3Init();

////////////////////////////////////////////////////////////////////////

// CC는 노브나 그런 continues한 것들
void SetCC( int ControlNumber, int ControlValue, int Channel )
{
  midiOut.sendControlChange(ControlNumber, ControlValue, Channel); 
}

void SetPC( int ProgramNumber, int Channel )
{
  midiOut.sendProgramChange(ProgramNumber, Channel);
}


void setup() 
{
  // Must set to Midi !
  Serial.begin(31250);
  //Serial.begin(9600);

  // 버튼 / LED 초기화
  pinMode(BTN_0_PORT, INPUT_PULLUP);
  pinMode(BTN_1_PORT, INPUT_PULLUP);
  pinMode(BTN_2_PORT, INPUT_PULLUP);
  pinMode(BTN_3_PORT, INPUT_PULLUP);
  pinMode(BTN_4_PORT, INPUT_PULLUP);
  pinMode(TUNNER_PORT, INPUT_PULLUP);

  pinMode(BTN_0_LED_PORT, OUTPUT);
  pinMode(BTN_1_LED_PORT, OUTPUT);
  pinMode(BTN_2_LED_PORT, OUTPUT);
  pinMode(BTN_3_LED_PORT, OUTPUT);
  pinMode(BTN_4_LED_PORT, OUTPUT);
  pinMode(TUNNER_LED_PORT, OUTPUT);
  
  SetFM3Init();
  
  Serial.print(F("Setup Done\n"));
}

unsigned long debounceDelay = 200; // ms
//unsigned long debounceDelay = 50; // ms
unsigned long longpresstime = 400;

bool checkPress(int btn)
{
    int state = digitalRead(btn);
    int p = btn - BTN_0_PORT;
    unsigned long ct = millis();
	bool ret = false;
    
	if( state == LOW )
	{
		if( btnState[p] == KEY_STATE_NONE)
		{
			btnState[p] = KEY_STATE_PRESS;
			pressTime[p] = ct;
			//Serial.print(F("check low\n"));        
			ret = true;
		}
	}
	else if( state == HIGH )
	{
		if( btnState[p] != KEY_STATE_NONE && pressTime[p] + debounceDelay < ct)
		{
			btnState[p] = KEY_STATE_NONE;
			pressTime[p] = ct;        
			//Serial.print(F("check high\n"));
		}
	}
	
    return ret;
}

bool checkPress_TUNNER(int btn)
{
    int state = digitalRead(btn);
    int p = btn - BTN_0_PORT;
    unsigned long ct = millis();
    bool ret = false;
	
    if( state == LOW )
    {
      if( btnState[p] == KEY_STATE_NONE)
      {
        btnState[p] = KEY_STATE_PRESS;
        pressTime[p] = ct;
        //Serial.print(F("check low\n"));        
        ret = true;
      }
    }
    else if( state == HIGH )
    {
      if( btnState[p] == KEY_STATE_PRESS && pressTime[p] + debounceDelay < ct)
      {
        btnState[p] = KEY_STATE_RELEASE;
        pressTime[p] = ct;
		//Serial.print(F("check high\n"));
      }
	  else if( btnState[p] == KEY_STATE_RELEASE && pressTime[p] + longpresstime < ct)
	  {
		  btnState[p] = KEY_STATE_NONE;
	  }
    }
	
    return ret;
}

void LedOn(int pos)
{
  for(int i=0; i<MAX_BTN; i++)
  {
    if( pos == lastselect) continue;
    digitalWrite(BTN_0_LED_PORT+i, LOW);  
  }
  
  digitalWrite(BTN_0_LED_PORT+pos, HIGH);
}

void SetTunner(bool enable)
{
	SetCC( TUNER_CC, enable == true ? TUNER_ON:TUNER_OFF, MIDI_CHANNEL );
	digitalWrite(TUNNER_LED_PORT, enable ? HIGH : LOW);
}

void ChangeScene(int pos)
{
	LedOn(pos);
	SetCC( SCENE_CC, SCENE_0 + pos, MIDI_CHANNEL );
}

void PressTunner()
{
	on_tunner = !on_tunner;
	SetTunner(on_tunner);
}

void PressBTN(int pos)
{
	if( on_tunner == true )
		PressTunner();

	if( lastselect == pos ) return;
	ChangeScene(pos);
	lastselect = pos;	
}


void SetFM3Init()
{
	for(int i=0; i<MAX_BTN; i++)
	{
		digitalWrite(BTN_0_LED_PORT + i, HIGH);
		delay(200);
	}
	
	delay(300);
	for(int i=0; i<MAX_BTN; i++)
		digitalWrite(BTN_0_LED_PORT + i, LOW);
}


////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
    if(checkPress_TUNNER(TUNNER_PORT) == true)
		PressTunner();
	
    if(checkPress(BTN_0_PORT) == true)
		PressBTN(SCENE_0);
    else if(checkPress(BTN_1_PORT) == true)
		PressBTN(SCENE_1);
    else if(checkPress(BTN_2_PORT) == true)
		PressBTN(SCENE_2);
    else if(checkPress(BTN_3_PORT) == true)
		PressBTN(SCENE_3);
    else if(checkPress(BTN_4_PORT) == true)
		PressBTN(SCENE_4);

}
