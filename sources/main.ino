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
  enum Boutons {Echap, Gauche, Droite, Entree};

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

//MENUS=======================================================================
  typedef struct{
    char nom[3];
    int id_parent;
    int valeur;
  }menuItem;

  //Nombre d'items dans le menu
  const byte nombreItems = 6;

  menuItem menu[nombreItems];

  //Le menu dans lequel on est actuellement
  int menuCourant = -1;

  //Le menu actuellement affiche sur l'ecran
  int menuAffiche = 0;

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
        INCLUDES
*****************************************************************************/


#include "sorties.h"
#include "messages_midi.h"
#include "affichage.h"




/*****************************************************************************
        FONCTIONS
*****************************************************************************/
  

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




  //ENTREE BOUTON=============================================================

    void verifierBoutons() {

      for(byte compteur = 0; compteur < 4; compteur++){

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
              traitementBouton((Boutons) compteur);
              strcpy(donneesAffichage, menu[menuAffiche].nom);
              creation_digits(donneesAffichage);
            }    
          }      
        }
        dernier_etat_bouton[compteur] = lecture;
         
      }
    }

  //TRAITEMENT BOUTON=========================================================

    void traitementBouton(Boutons dernierBoutonPresse) {

      switch (dernierBoutonPresse) {

        case Echap:
          Serial.println("Echap");
          //Si on n'est pas a la racine
          if(menuCourant > -1) {
            menuCourant = menu[menuCourant].id_parent;
            do {
              menuAffiche++;
              if(menuAffiche > nombreItems) {
                menuAffiche = 0;
              }
            } while (menu[menuAffiche].id_parent != menuCourant);
          }
          // do something
          break;
          
        case Gauche:
          Serial.println("Gauche");
          do {
            if(menuAffiche == 0) {
              menuAffiche = nombreItems;
            }
            menuAffiche--;
          } while (menu[menuAffiche].id_parent != menuCourant);
          break;

        case Droite:
          Serial.println("Droite");
          do {
            menuAffiche++;
            if(menuAffiche > nombreItems) {
              menuAffiche = 0;
            }
          } while (menu[menuAffiche].id_parent != menuCourant);
          break;

        case Entree:
          Serial.println("Entree");

          //Si le menu sur lequel on vient de cliquer possede des enfants
          if(possedeDesEnfants(menuAffiche)) {
            menuCourant = menuAffiche;
            do {
              menuAffiche++;
              if(menuAffiche > nombreItems) {
                menuAffiche = 0;
              }
            } while (menu[menuAffiche].id_parent != menuCourant);
          }
          // do something
          break;
        
        default:
          Serial.println("Erreur traitementBouton");
      }

    }

  //MENUS=====================================================================

    //Verifie si le menu teste possede des enfants
    boolean possedeDesEnfants(byte menuTeste) {
      boolean enfantsTrouves = false;
      for(int i=0; i<nombreItems; i++){
        if(menu[i].id_parent == menuTeste) {
          enfantsTrouves = true;
        }
      }
      return enfantsTrouves;
    }

/*****************************************************************************
        SETUP
*****************************************************************************/
  void setup() {

    //MENU====================================================================
      
      //Juste pour faciliter l'ajout de menus
      byte id = 0;

      //Clock division
        id = 0;
        strncpy(menu[id].nom, "di", 3);
        menu[id].id_parent = -1;
        menu[id].valeur = 24;  //1->96
      //Root note
        id = 1;
        strncpy(menu[id].nom, "no", 3);
        menu[id].id_parent = -1;
        menu[id].valeur = 0;
      //Canal
        id = 2;
        strncpy(menu[id].nom, "ch", 3);
        menu[id].id_parent = -1;
        menu[id].valeur = 0; //-1 = learn / 1->16
      //Start / Stop;
        id = 3;
        strncpy(menu[id].nom, "St", 3);
        menu[id].id_parent = -1;
        menu[id].valeur = 0; //0 = trig / 1 = gate
      //Start / Stop;
        id = 4;
        strncpy(menu[id].nom, "tr", 3);
        menu[id].id_parent = 3;
        menu[id].valeur = 0; //0 = trig / 1 = gate
      //Start / Stop;
        id = 5;
        strncpy(menu[id].nom, "gt", 3);
        menu[id].id_parent = 3;
        menu[id].valeur = 0; //0 = trig / 1 = gate
       
       
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

      strcpy(donneesAffichage, menu[menuAffiche].nom);
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
