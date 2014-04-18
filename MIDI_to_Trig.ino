/*
MIDI to Trig
Laurent Calvignac
Janvier 2014
 
 * Ce programme est un programme libre ;
 * vous pouvez le redistribuer et/ou le modifier
 * dans les termes de la licence GNU GPL, comme publiée par
 * la Free Software Foundation; soit dans la version 3 de la license, ou
 * (suivant votre préférence) dans une version ultérieure.
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU GPL as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 
*/
 
/*
 
  sorties 1 a 12 : triggers/gates
  sortie 13 : accent (velocite > )
  sortie 14 : play/stop
  sortie 15 : clock divisee
  sortie 16 : clock 1:1
 
*/
 
#include <string.h>
#include <MIDI.h>
#include <SoftwareSerial.h>
    SoftwareSerial midiSerial(2,3);
    MIDI_CREATE_INSTANCE(SoftwareSerial, midiSerial, midiBench);





/*****************************************************************************
        VARIABLES
*****************************************************************************/

//PINS========================================================================

//Pins connectes aux pins ST_CP des 74HC595
const int sorties_latchPin = 3;
const int affichage_latchPin = A1;

//Pins connectes aux pins SH_CP des 74HC595
const int sorties_clockPin = 4;
const int affichage_clockPin = A2;

//Pins connectes aux pins DS des 74HC595
const int sorties_dataPin = 5;
const int affichage_dataPin = A0;

//Pins utilises pour le multiplexage des afficheurs
const int affichage_digit1 = A5;
const int affichage_digit2 = A4;

//ENUMS=======================================================================

enum Modes {Trigger, Gate};
enum LearnModes {Learn, Auto, Off};

//PARAMETRES==================================================================

typedef struct{

  //apprentissage des notes
  LearnModes NotesLearn;

  //apprentissage du canal midi
  boolean CanalLearn;

  //canal midi selectionne
  int Canal;

  //1->12:triggers, 13:Accent, 14:Play/Stop, 15:Clk div, 16: Clk
  //mode gate ou trigger pour chaque sortie
  Modes ModeSortie[16];

  //duree du trigger pour chaque sortie
  int DureeSortie[16];

  //Triggers (1->12) : note autorisee
  //Accent : velocite seuil
  //Play/Stop : NC 
  //Div : facteur de division 
  //Clk : NC
  int ParamSortie[16];

}structParametres;

structParametres parametres;

//VARIABLES TEMPORELLES=======================================================

//taux de rafraichissement de l'affichage (en ms)
byte refreshAffichage = 10;

//Utilise pour le rafraichissement des sorties
unsigned long millisTriggers[16];

//Utilise pour le rafraichissement de l'affichage
unsigned long millisAffichage;

/*****************************************************************************
        FONCTIONS
*****************************************************************************/
void handleNoteOn(byte inChannel, byte inNote, byte inVelocity)
{
    Serial.print("Note on");
    Serial.print("  : ");
    Serial.print(inNote);
    Serial.print(" : ");
    Serial.print(inVelocity);
    Serial.print(" : ");
    Serial.println(inChannel);
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
    Serial.print("Note off");
    Serial.print(" : ");
    Serial.print(inNote);
    Serial.print(" : ");
    Serial.print(inVelocity);
    Serial.print(" : ");
    Serial.println(inChannel);
}


/*****************************************************************************
        SETUP
*****************************************************************************/
void setup() {

  pinMode(sorties_latchPin, OUTPUT);
  pinMode(sorties_clockPin, OUTPUT);
  pinMode(sorties_dataPin, OUTPUT);
  pinMode(affichage_latchPin, OUTPUT);
  pinMode(affichage_clockPin, OUTPUT);
  pinMode(affichage_dataPin, OUTPUT);
  pinMode(affichage_digit1, OUTPUT);
  pinMode(affichage_digit2, OUTPUT);

    midiBench.setHandleNoteOn(handleNoteOn);
    midiBench.setHandleNoteOff(handleNoteOff);
    midiBench.begin(MIDI_CHANNEL_OMNI);

    while(!Serial);
    Serial.begin(115200);
    Serial.println("Arduino Ready");
}


/*****************************************************************************
        MAIN
*****************************************************************************/
void loop() {
  midiBench.read();
}