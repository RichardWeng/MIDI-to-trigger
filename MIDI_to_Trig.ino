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
    enum Ports {Triggers, Affichage};

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

  //Utilise pour la division de l'horloge midi
  byte compteurClock = 0;

  //Utilise pour le rafraichissement des sorties
  unsigned long millisTriggers[16];

  //Utilise pour le rafraichissement de l'affichage
  unsigned long millisAffichage;

//SORTIES=====================================================================
  
  //Definit l'etat des sorties trigger/gate
  word etat_sorties = 0;

  //Associe les sorties (n-1) aux evenements midi
  byte note[12] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  byte accent = 0;
  byte transport = 1;
  byte div_clock = 2;
  byte sync_clock = 3;

//DIVERS======================================================================
  //Utilise pour savoir si on a fini l'apprentissage (0) ou non (1)
  boolean learnFlag = 1;
  //Sert a compter le nombre de notes deja apprises
  byte compteurNotesLearn = 0;

/*****************************************************************************
        FONCTIONS
*****************************************************************************/
  //MIDI======================================================================
    void handleNoteOn(byte inChannel, byte inNote, byte inVelocity) {
      Serial.print("Note on");
      Serial.print("  : ");
      Serial.print(inNote);
      Serial.print(" : ");
      Serial.print(inVelocity);
      Serial.print(" : ");
      Serial.println(inChannel);

      //Si l'on doit apprendre le canal ou les notes
      if(learnFlag && (parametres.NotesLearn || parametres.CanalLearn)) {
        //Si l'on doit apprendre le canal
        if(parametres.CanalLearn) {
          //On enregistre le canal dans les parametres
          parametres.Canal = inChannel;
          //TODO : Il ne faut ecouter que ce canal
          //Si l'on n'a pas active l'apprentissage des notes
          if(!parametres.NotesLearn) {
            //On a fini l'apprentissage
            learnFlag = 0;
          }
        }
        
        //Si l'apprentissage des notes est en mode auto
        if(parametres.NotesLearn == Auto) {
          //On enregistre cette note et les notes consecutives dans les parametres
          for(byte compteur = 0; compteur < 12; compteur++) {
            parametres.ParamSortie[compteur] = inNote + compteur;
          }
          //On a fini l'apprentissage
          learnFlag = 0;
        }
        //Sinon, l'apprentissage des notes est en mode learn, on doit les apprendre toutes une par une
        else if(parametres.NotesLearn == Learn) {
          //On enregistre les notes dans les parametres une par une, dans l'ordre de reception
          parametres.ParamSortie[compteurNotesLearn] = inNote;
          //on incremente le compteur
          compteurNotesLearn++;
          //Si le compteur arrive a 12, c'est qu'on a assigne une note a chaque sortie
          if(compteurNotesLearn == 12) {
            //On a fini l'apprentissage
            learnFlag = 0;
          }
        }
      }

      for(byte compteur=0; compteur < 12; compteur++) {
        //Si c'est une des 12 notes assignees a une sortie
        if(parametres.ParamSortie[compteur] == inNote) {
          //On active cette sortie
          bitSet(etat_sorties, note[compteur]);
          //Si cette sortie est en mode trigger
          if(parametres.ModeSortie[compteur] == Trigger) {
            //On demarre le chrono du temps d'impulsion
            millisTriggers[compteur] = millis();
          }
        }
      }
      //On met a jour l'etat des sorties
      seriOut(Triggers, etat_sorties);

    }

    void handleNoteOff(byte inChannel, byte inNote, byte inVelocity) {
        Serial.print("Note off");
        Serial.print(" : ");
        Serial.print(inNote);
        Serial.print(" : ");
        Serial.print(inVelocity);
        Serial.print(" : ");
        Serial.println(inChannel);
    }

    void handleClock() {
      //TODO : verifier la precision de cette horloge !
      //on active la sortie sync24
      bitSet(etat_sorties, sync_clock);
      //on demarre le chrono du temps d'impulsion
      millisTriggers[sync_clock] = millis();
      //si le compteur de clock est revenu a zero
      if(compteurClock == 0){
        //on active la sortie d'horloge divisee
        bitSet(etat_sorties, div_clock);
        //on demarre le chrono du temps d'impulsion
        millisTriggers[div_clock] = millis();
        //on remet le compteur au facteur de division choisit
        compteurClock = parametres.ParamSortie[div_clock];
      }
      //sinon on decremente le commpteur
      compteurClock--;
      seriOut(Triggers, etat_sorties);
    }

    void handleStart() {
      Serial.print("Transport : ");
      //on inverse l'etat de cette sortie
      bitWrite(etat_sorties, transport, !bitRead(etat_sorties, transport));
      //si c'est un message start
      if(midiBench.getType() == midi::Start){
        //on remet a zero le compteur de clock
        compteurClock = 0;
      }
      //si on est en mode trig
      if(parametres.ModeSortie[transport] == Trigger){
        //on demarre le chrono du temps d'impulsion
        millisTriggers[transport] = millis();
      }
      //on met a jour l'etat des sorties
      seriOut(Triggers, etat_sorties);
      if(midiBench.getType() == midi::Start){Serial.println("Start");}
    }

    void handleContinue() {
      handleStart();
      Serial.println("Continue");
    }

    void handleStop() {
      handleStart();
      Serial.println("Stop");
    }

  //DELAIS====================================================================

    void verifier_delais() {
      unsigned long millisActuel = millis();
      //extinction des sorties
      for(int compteur=0; compteur<16; compteur++){
          //si on a depasse le temps et que la sortie est en mode trigger et qu'elle est actuellement active :
          if((millisActuel - millisTriggers[compteur]) > parametres.DureeSortie[compteur] && parametres.ModeSortie[compteur] == Trigger && bitRead(etat_sorties, compteur) == 1){
              bitClear(etat_sorties, compteur); //on passe cette sortie en off
              seriOut(Triggers, etat_sorties); //on envoie cette donnee sur le port serie des sorties
          }
      }
      //rafraichissement de l'affichage
      if(millisActuel - millisAffichage > refreshAffichage){
          //affichage();
      }
    }

  //SORTIE SERIE==============================================================
    void seriOut(Ports port, byte donnees){
      byte dataPin;
      byte clockPin;
      byte latchPin;
      switch (port) {
          case Triggers:
            dataPin = sorties_dataPin;
            clockPin = sorties_clockPin;
            latchPin = sorties_latchPin;
            break;
          case Affichage:
            dataPin = affichage_dataPin;
            clockPin = affichage_clockPin;
            latchPin = affichage_latchPin;
            break;
      }
      digitalWrite(latchPin, LOW);
      if(port == Triggers){
        shiftOut(dataPin, clockPin, MSBFIRST, (donnees >> 8));      
      }
      shiftOut(dataPin, clockPin, MSBFIRST, donnees);
      digitalWrite(latchPin, HIGH);
    }

/*****************************************************************************
        SETUP
*****************************************************************************/
void setup() {

  //PINS======================================================================

    pinMode(sorties_latchPin, OUTPUT);
    pinMode(sorties_clockPin, OUTPUT);
    pinMode(sorties_dataPin, OUTPUT);
    pinMode(affichage_latchPin, OUTPUT);
    pinMode(affichage_clockPin, OUTPUT);
    pinMode(affichage_dataPin, OUTPUT);
    pinMode(affichage_digit1, OUTPUT);
    pinMode(affichage_digit2, OUTPUT);

  //CALLBACKS=================================================================

    //Callback Note on
    midiBench.setHandleNoteOn(handleNoteOn);
    //Callback Note off 
    midiBench.setHandleNoteOff(handleNoteOff);
    //Callback Clock
    midiBench.setHandleClock(handleClock);
    //Callback Start
    midiBench.setHandleStart(handleStart);
    //Callback Stop
    midiBench.setHandleContinue(handleContinue);
    //Callback Continue
    midiBench.setHandleStop(handleStop);

  //PORTS SERIE===============================================================

    //Midi
    midiBench.begin(MIDI_CHANNEL_OMNI);

    //Serie
    while(!Serial);
    Serial.begin(115200);
    Serial.println("Arduino Ready");

  //DEBUG=====================================================================
  parametres.CanalLearn = 1;
  parametres.NotesLearn = Auto;
  
  parametres.ParamSortie[div_clock] = 24;

  for(int i=0; i<16; i++){
    parametres.ModeSortie[i] = Trigger;
  }
  parametres.ModeSortie[transport] = Trigger;
  
  for(int i=0; i<16; i++){
    parametres.DureeSortie[i] = 20;
  }
  parametres.DureeSortie[sync_clock] = 0;

}


/*****************************************************************************
        MAIN
*****************************************************************************/
void loop() {
  midiBench.read();
  verifier_delais();
}
