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
  const int affichage_digit[2] = {A5, A4};

  //Boutons 8=echap 9=gauche 10=droite 11=entree
  const int entreesBoutons[4] = {8, 9, 10, 11};

//ENUMS=======================================================================

  enum Modes {Trigger, Gate};
  enum LearnModes {Off, Learn, Auto};
  enum Ports {Triggers, Affichage};
  enum Boutons {Entree, Echap, Gauche, Droite};

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

  //Taux de rafraichissement de l'affichage (en ms)
  byte refreshAffichage = 10;

  //Temps pour le debounging des boutons (en ms)
  byte tempsDebounce = 20;

  //Utilise pour la division de l'horloge midi
  byte compteurClock = 0;

  //Utilise pour le rafraichissement des sorties
  unsigned long millisTriggers[16];

  //Utilise pour le rafraichissement de l'affichage
  unsigned long millisAffichage;

  //Utilise pour le debouncing des boutons
  unsigned long millisBoutons[4];

  //MENUS=====================================================================
    typedef struct{
      char nom[3];
      int id_parent;
      int valeur;
    }menuItem;

    menuItem menu[20];

    byte menuCourant = 0;

//AFFICHAGE===================================================================
  typedef struct{
    byte segments;
    char symbole;
  }digits;

  //Le tableau contenant les caracteres pour l'afficheur 7 segments
  digits tableauDigits[40];
  
  //donnees a afficher sur les digits
  char donneesAffichage[2];

//SORTIES=====================================================================
  
  //Definit l'etat des sorties trigger/gate
  word etat_sorties = 0;

  //Associe les sorties (n-1) aux evenements midi
  byte note[12] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  byte accent = 0;
  byte transport = 1;
  byte div_clock = 2;
  byte sync_clock = 3;

//BOUTONS=====================================================================
  //Le bouton que l'on vient de presser
  Boutons dernierBoutonPresse;
  byte
    etat_bouton[4], //
    dernier_etat_bouton[4], //
    nouvel_appui[4];  //

//DIVERS======================================================================
  //Utilise pour savoir si on a fini l'apprentissage (0) ou non (1)
  boolean learnFlag = 1;
  //Sert a compter le nombre de notes deja apprises
  byte compteurNotesLearn = 0;
  //

  byte i = 0;

/*****************************************************************************
        FONCTIONS
*****************************************************************************/
  //MIDI======================================================================
    void handleNoteOn(byte inChannel, byte inNote, byte inVelocity) {

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
            parametres.ParamSortie[note[compteur]] = inNote + compteur;
          }
          //On a fini l'apprentissage
          learnFlag = 0;
        }
        //Sinon, l'apprentissage des notes est en mode learn, on doit les apprendre toutes une par une
        else if(parametres.NotesLearn == Learn) {
          //Serial.println("Mode Learn");
          //On enregistre les notes dans les parametres une par une, dans l'ordre de reception
          parametres.ParamSortie[note[compteurNotesLearn]] = inNote;
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
        if(parametres.ParamSortie[note[compteur]] == inNote) {
          //On active cette sortie
          bitSet(etat_sorties, note[compteur]);
          //Si cette sortie est en mode trigger
          if(parametres.ModeSortie[note[compteur]] == Trigger) {
            //On demarre le chrono du temps d'impulsion
            millisTriggers[note[compteur]] = millis();
          }
        }
      }

      if(inVelocity >= parametres.ParamSortie[accent]) {
        bitSet(etat_sorties, accent);
        millisTriggers[accent] = millis();
      }

      //On met a jour l'etat des sorties
      seriOut(Triggers, etat_sorties);

    }

    void handleNoteOff(byte inChannel, byte inNote, byte inVelocity) {

      for(byte compteur=0; compteur < 12; compteur++) {
        //Si c'est une des 12 notes assignees a une sortie et que cette sortie est en mode gate
        if(parametres.ParamSortie[note[compteur]] == inNote && parametres.ModeSortie[note[compteur]] == Gate) {
          //On desactive cette sortie
          bitClear(etat_sorties, note[compteur]);
          //Si cette sortie est en mode trigger
        }
      }
      //On met a jour l'etat des sorties
      seriOut(Triggers, etat_sorties);

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
      killNotes();
    }

    /*void handleControlChange(byte channel, byte number, byte value) {
      if(number == 114 && value == 127) {handleStop();}
    }*/

    void killNotes() {
      for(byte compteur=0; compteur < 12; compteur++) {
        //On eteint toutes les notes
        bitClear(etat_sorties, note[compteur]);
      }
      //On met a jour l'etat des sorties
      seriOut(Triggers, etat_sorties);
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
          affichage();
      }
    }

  //SORTIE SERIE==============================================================
    void seriOut(Ports port, byte donnees) {
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

  //AFFICHAGE=================================================================

    void affichage() {
      byte pin_digitActif;
      byte pin_digitInactif;
      byte donnees;
    
      //On active le digit qui n'etait pas actif lors de la lecture precedente
      boolean lecture = digitalRead(affichage_digit[0]);
      pin_digitActif = affichage_digit[lecture];
      pin_digitInactif = affichage_digit[!lecture];
      donnees = donneesAffichage[lecture];
   
      digitalWrite(pin_digitInactif, LOW);  //desactivation du digit precedent
      seriOut(Affichage, donnees);  //transmission des donnees sur le port AFFICHAGE
      digitalWrite(pin_digitActif, HIGH); //activation du digit courant
      millisAffichage = millis();
    }

  //CREATION DIGITS===========================================================
 
  void creation_digits(const char* caracteres) {
    //TODO
    //ici, rechercher correspondance entre les caracteres en entree et le
    //tableau des digits pour en deduire le numero du digit a afficher.
    //Penser a afficher du vide devant lorsqu'il n'y a qu'un seul digit.
    
    byte compteur = 0;

    for(int i=0; i < 2; i++){
      compteur = 0;
      while(caracteres[i] != tableauDigits[compteur].symbole) {
        compteur++;
      }
      donneesAffichage[i] = tableauDigits[compteur].segments;
    }    
  }

  //ENTREE BOUTON=============================================================

    void verifierBoutons() {
      for(int compteur=0; compteur<4; compteur++){
        
        //on enregistre le temps actuel
        unsigned long millisActuel = millis();
        
        //lire l'entree du bouton-poussoir
        byte lecture = digitalRead(entreesBoutons[compteur]);
       
        //comparer l'etat du bouton a son etat precedent
        //si le statut du bouton a change
        if (lecture != dernier_etat_bouton[compteur]) {
          //on enregistre le temps actuel
          millisBoutons[compteur] = millisActuel;
        }
        
        //Si on a passe e temps de debouncing
        if((millisActuel - millisBoutons[compteur]) > tempsDebounce){
          
          //si l'etat du bouton a change
          if(lecture != etat_bouton[compteur]) {
            etat_bouton[compteur] = lecture;
       
            //si le bouton est presse
            if(etat_bouton[compteur] == true) {
              nouvel_appui[compteur] = true;
              //DEBUG=========================================================
                switch (compteur) {
                    case 0:
                      Serial.println("Exit");
                      break;
                    case 1:
                      Serial.println("Gauche");
                      menuCourant--;
                      break;
                    case 2:
                      Serial.println("Droite");
                      menuCourant++;
                      break;
                    case 3:
                      Serial.println("Entree");
                      break;
                    default:
                      Serial.println("Erreur boutons");
                }
                strcpy(donneesAffichage, menu[menuCourant].nom);
                creation_digits(donneesAffichage);
            }    
          }      
        }
        dernier_etat_bouton[compteur] = lecture;
         
      }
    }

/*****************************************************************************
        SETUP
*****************************************************************************/
  void setup() {

    //MENU====================================================================
        //Clock division
          strncpy(menu[0].nom, "di", 3);
          menu[0].id_parent = -1;
          menu[0].valeur = 24;  //1->96
        //Root note
          strncpy(menu[1].nom, "no", 3);
          menu[1].id_parent = -1;
          menu[1].valeur = 0;
        //Canal
          strncpy(menu[2].nom, "ch", 3);
          menu[2].id_parent = -1;
          menu[2].valeur = 0; //-1 = learn / 1->16
        //Start / Stop;
          strncpy(menu[3].nom, "St", 3);
          menu[3].id_parent = -1;
          menu[3].valeur = 0; //0 = trig / 1 = gate
       
       
      /*enum menus {
      HOME,
      CLOCK,
      CLOCK_DIV,
      NOTE,
      NOTE_SELEC,
      CANAL,
      CANAL_SELEC,
      START,
      START_SELEC,
        SORTIES,
        SORTIES_MODE,
        SORTIES_MODE_SELEC,
        SORTIES_MODE_SELEC_MODE,
        SORTIES_DUREE,
        SORTIES_DUREE_SELEC,
        SORTIES_DUREE_SELEC_DUREE,
        ACCENT,
        ACCENT_SELEC,
        LEARN
      };*/

    //PINS====================================================================

      pinMode(sorties_latchPin, OUTPUT);
      pinMode(sorties_clockPin, OUTPUT);
      pinMode(sorties_dataPin, OUTPUT);
      
      pinMode(affichage_latchPin, OUTPUT);
      pinMode(affichage_clockPin, OUTPUT);
      pinMode(affichage_dataPin, OUTPUT);
      pinMode(affichage_digit[0], OUTPUT);
      pinMode(affichage_digit[1], OUTPUT);

      pinMode(entreesBoutons[0], INPUT);
      pinMode(entreesBoutons[1], INPUT);
      pinMode(entreesBoutons[2], INPUT);
      pinMode(entreesBoutons[3], INPUT);

    //AFFICHAGE===============================================================
    //                                                             GFABDC.E
    tableauDigits[0].symbole  = '0'; tableauDigits[0].segments  = B01111101;
    tableauDigits[1].symbole  = '1'; tableauDigits[1].segments  = B00010100;
    tableauDigits[2].symbole  = '2'; tableauDigits[2].segments  = B10111001;
    tableauDigits[3].symbole  = '3'; tableauDigits[3].segments  = B10111100;
    tableauDigits[4].symbole  = '4'; tableauDigits[4].segments  = B11010100;
    tableauDigits[5].symbole  = '5'; tableauDigits[5].segments  = B11101100;
    tableauDigits[6].symbole  = '6'; tableauDigits[6].segments  = B11101101;
    tableauDigits[7].symbole  = '7'; tableauDigits[7].segments  = B00110100;
    tableauDigits[8].symbole  = '8'; tableauDigits[8].segments  = B11111101;
    tableauDigits[9].symbole  = '9'; tableauDigits[9].segments  = B11111100;
    tableauDigits[10].symbole = 'a'; tableauDigits[10].segments = B10111101;
    tableauDigits[11].symbole = 'b'; tableauDigits[11].segments = B11001101;
    tableauDigits[12].symbole = 'c'; tableauDigits[12].segments = B10001001;
    tableauDigits[13].symbole = 'd'; tableauDigits[13].segments = B10011101;
    tableauDigits[14].symbole = 'E'; tableauDigits[14].segments = B11101001;
    tableauDigits[15].symbole = 'F'; tableauDigits[15].segments = B11100001;
    tableauDigits[16].symbole = 'g'; tableauDigits[16].segments = B11111100;
    tableauDigits[17].symbole = 'h'; tableauDigits[17].segments = B11000101;
    tableauDigits[18].symbole = 'i'; tableauDigits[18].segments = B00000001;
    tableauDigits[19].symbole = 'J'; tableauDigits[19].segments = B00011100;
    tableauDigits[20].symbole = 'K'; tableauDigits[20].segments = B11010101;
    tableauDigits[21].symbole = 'L'; tableauDigits[21].segments = B01001001;
    tableauDigits[22].symbole = 'M'; tableauDigits[22].segments = B01110101;
    tableauDigits[23].symbole = 'n'; tableauDigits[23].segments = B10000101;
    tableauDigits[24].symbole = 'o'; tableauDigits[24].segments = B10001101;
    tableauDigits[25].symbole = 'p'; tableauDigits[25].segments = B11110001;
    tableauDigits[26].symbole = 'q'; tableauDigits[26].segments = B11110100;
    tableauDigits[27].symbole = 'r'; tableauDigits[27].segments = B10000001;
    tableauDigits[28].symbole = 'S'; tableauDigits[28].segments = B11101100;
    tableauDigits[29].symbole = 't'; tableauDigits[29].segments = B11001001;
    tableauDigits[30].symbole = 'U'; tableauDigits[30].segments = B01011101;
    tableauDigits[31].symbole = 'v'; tableauDigits[31].segments = B00001101;
    tableauDigits[32].symbole = 'W'; tableauDigits[32].segments = B00101101;
    tableauDigits[33].symbole = 'X'; tableauDigits[33].segments = B11010101;
    tableauDigits[34].symbole = 'Y'; tableauDigits[34].segments = B11011100;
    tableauDigits[35].symbole = 'Z'; tableauDigits[35].segments = B10111001;
    tableauDigits[36].symbole = '.'; tableauDigits[36].segments = B00000010;
    tableauDigits[37].symbole = '-'; tableauDigits[37].segments = B10000000;
    //double L
    tableauDigits[38].symbole = 'l'; tableauDigits[38].segments = B01010101;
    //vide
    tableauDigits[39].symbole = '/'; tableauDigits[39].segments = B00000000;

    //CALLBACKS===============================================================

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

      //Callback CC
      //midiBench.setHandleControlChange(handleControlChange);

    //PORTS SERIE=============================================================

      //Midi
      midiBench.begin(MIDI_CHANNEL_OMNI);

      //Serie
      while(!Serial);
      Serial.begin(115200);
      Serial.println("Arduino Ready");

      seriOut(Triggers, 0);

    //DEBUG===================================================================
      
      parametres.CanalLearn = 1;
      parametres.NotesLearn = Learn;
      
      parametres.ParamSortie[div_clock] = 24;

      for(int i=0; i<12; i++){
        parametres.ModeSortie[note[i]] = Gate;
      }
      parametres.ModeSortie[transport] = Trigger;
      
      for(int i=0; i<16; i++){
        parametres.DureeSortie[i] = 20;
      }
      parametres.DureeSortie[sync_clock] = 0;

      strcpy(donneesAffichage, "dv");
      creation_digits(donneesAffichage);

      

  }

/*****************************************************************************
        MAIN
*****************************************************************************/
  void loop() {
    midiBench.read();
    //On ne verifie les delais qu'une fois sur 10
    if(i >= 10) {
      verifier_delais();
      verifierBoutons();
      i = 0;
    }
    i++;
  }
