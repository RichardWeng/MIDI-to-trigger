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
 
//#include <string.h>
 
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
const int sorties_dataPin = 2;
const int affichage_dataPin = A0;
 
const int affichage_digit1 = A5;
const int affichage_digit2 = A4;
 
//MENUS=======================================================================
typedef struct{
  char nom[3];
  int id_parent;
  int valeur;
}menuItem;
 
//PARAMETRES==================================================================
byte refreshAffichage = 10;  //taux de rafraichissement de l'affichage (en ms)
boolean notes_learn = true;  //activer ou desactiver l'apprentissage de la note de base (actif par defaut)
boolean canal_learn = true;  //activer ou desactiver l'apprentissage du canal midi (actif par defaut)
byte canal_canal = 0;  //canal midi selectionne
//                          1   2   3   4   5   6   7   8   9   10  11  12  Acc P/S Div Clk     Facteurs de division : 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 96
byte sorties_valeur[16] =  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  80, 0,  12, 0  }; //Triggers : notes autorisees / Accent : velocite seuil / P/S : NC / Div : facteur de division /Clk : NC
byte sorties_mode[16]   =  {1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1  }; //mode gate (0) ou trigger (1) pour chaque sortie (trigger par defaut, sauf pour l'accent)
byte sorties_duree[16]  =  {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 }; //duree du trigger pour chaque sortie (10ms par defaut)
 
//============================================================================
unsigned long millisPrecedent[16];
unsigned long millisAffichage;
 
byte
octet_entrant = 0,  //octet lu sur le port MIDI
prochain_byte = 0,
running_status = 0,
numero_note = 0,
velocite = 0;
 
byte compteurTic = 0;
 
//AFFICHAGE===================================================================
typedef struct{
  byte segments;
  char symbole;
}digits;
 
byte affichage_digits[2] = {0};  //donnees a afficher sur les digits
 
enum sortie {TRIGGERS, AFFICHAGE};  //la sortie sur laquelle on envoie des donnees
enum bouton_presse {ENTREE, ECHAP}; //le bouton qu'on vient de presser
 
word etat_sorties = 0 ;
 
//BOUTONS=====================================================================
byte
etat_bouton[4], //0 = bouton entree / 1 = bouton echap / 2 & 3 = entrees encodeur
dernier_etat_bouton[4], //etat precedent du bouton
nouvel_appui[4],  //nouvel appui sur un bouton ou interaction avec l'encodeur
tempsDebounce = 20;
unsigned long bouton_millisPrecedent[4];  //temps de debouncing pour les boutons et l'encodeur
 
 
boolean
nouvelle_note = false,  //indicateur de reception d'une nouvelle note
nouveau_transport = false,  //indicateur de reception d'un nouveau message de transport
clock_tic = false,  //tic de synchronisation
play = false,  //statut play/stop (stop au demarrage)
learn_ok = false;  //si on a fini l'apprentissage
 
byte
MIDI_noteon = 144,  //commande note on
MIDI_noteoff = 128;  //commande note on
const byte
MIDI_clock = 248,  //commande midi clock
MIDI_play = 250,  //commande midi play
MIDI_continue = 251,  //commande midi continue
MIDI_stop = 252;  //commande midi stop
 
/*****************************************************************************
        SETUP
*****************************************************************************/
 
void setup() {
 
//MENU========================================================================
menuItem menu[20];
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
 
//AFFICHAGE===================================================================
/*
 
 AAAA
F    B
F    B
 GGGG
E    C
E    C
 DDDD .
 
*/
 
digits tableauDigits[40];
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
  tableauDigits[28].symbole = 's'; tableauDigits[28].segments = B11101100;
  tableauDigits[29].symbole = 't'; tableauDigits[29].segments = B11001001;
  tableauDigits[30].symbole = 'U'; tableauDigits[30].segments = B01011101;
  tableauDigits[31].symbole = 'v'; tableauDigits[31].segments = B00001101;
  tableauDigits[32].symbole = 'W'; tableauDigits[32].segments = B00101101;
  tableauDigits[33].symbole = 'X'; tableauDigits[33].segments = B11010101;
  tableauDigits[34].symbole = 'Y'; tableauDigits[34].segments = B11011100;
  tableauDigits[35].symbole = 'Z'; tableauDigits[35].segments = B10111001;
  tableauDigits[36].symbole = '.'; tableauDigits[36].segments = B00000010;
  tableauDigits[37].symbole = '-'; tableauDigits[37].segments = B10000000;
  tableauDigits[38].symbole = 'l'; tableauDigits[38].segments = B01010101; //double L
  tableauDigits[39].symbole = '/'; tableauDigits[39].segments = B00000000; //vide
 
 
  //=====================================DEBUG
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
 
  //affichage_digits[0] = digits[10];
  //affichage_digits[1] = digits[38];  //affiche "all"
  //=====================================DEBUG
 
  Serial.begin(31250);
  //TODO ici, recuperer les parametres dans la memoire
  if (canal_learn || notes_learn) {  //si au moins l'un des deux modes learn est actif
    MIDI_learn();
  }
  else {
    //recuperer la valeur du canal dans la memoire
  }
 
  MIDI_noteon = MIDI_noteon + canal_canal;
  MIDI_noteoff = MIDI_noteoff + canal_canal;
}
 
/*****************************************************************************
        MAIN
*****************************************************************************/
 
void loop() {
  lecture_MIDI();
  if (clock_tic) {  //si on a recu une midi clock
    sortie_clock();
  }
  else if (nouveau_transport) {  //si on a recu un nouveau message de transport
    sortie_transport();
  }
  else if (nouvelle_note) {  //si on a recu une nouvelle note
    sortie_note();
  }
  verifier_delais();
     
  //=====================================DEBUG
 
  //=====================================DEBUG
 
}
 
/*****************************************************************************
        FONCTIONS
*****************************************************************************/
 
//LECTURE MIDI================================================================
void lecture_MIDI() {
  if (Serial.available()){
    octet_entrant = Serial.read();
     
    if (octet_entrant == MIDI_noteon || octet_entrant == MIDI_noteoff) {  //si c'est une commande note on ou note off
      running_status = octet_entrant;  //on definit le statut en cours
      prochain_byte++;  //on incremente le numero du prochain byte a recevoir
    }
     
    else if (octet_entrant < 128) {  //si c'est une valeur et qu'on est en running status note on
      switch (prochain_byte)
      {
        case 1:
          numero_note = octet_entrant;
          prochain_byte++;
          break;
           
        case 2:
          velocite = octet_entrant;
          nouvelle_note = true;  //marqueur indiquant qu'on a reçu une nouvelle note
          prochain_byte = 0;
          break;
      }
    }
     
    else if (octet_entrant == MIDI_clock) {  //si c'est un MIDI clock
      clock_tic = true;
      //digitalWrite(11, HIGH);
      //delay(50);
      //digitalWrite(11, LOW);
    }
     
    else if (octet_entrant >= 250 && octet_entrant <= 252) {  //si c'est un message de transport
      switch(octet_entrant)
      {
        case MIDI_play:  //message play
          if(!play) {  //si on n'est pas deja en mode play
            nouveau_transport = true;
            play = true;
          }
          break;
           
        case MIDI_continue:  //message continue
          if(!play) {  //si on n'est pas deja en mode play
            nouveau_transport = true;
            play = true;
          }
          break;
           
        case MIDI_stop:  //message stop
          if(play) {  //si on n'est pas deja en mode stop
            nouveau_transport = true;
            play = false;
          }
          break;
      }
    }
     
  }
}
 
//MIDI LEARN==================================================================
 
void MIDI_learn()  {
  do
  {
    //=====================================DEBUG
    digitalWrite(13, HIGH);
    delay(10);
    digitalWrite(13, LOW);
    //=====================================DEBUG
   
    if (Serial.available()){
      octet_entrant = Serial.read();
       
      if (octet_entrant >= 128 && octet_entrant <= 159) {  //si c'est une commande note on ou note off
        running_status = octet_entrant;  //on definit le statut en cours
        if (canal_learn) {  //si on a active l'apprentissage du canal midi
          canal_canal = octet_entrant & 0x0F;
        }
        prochain_byte++;  //on incremente le numero du prochain byte a recevoir
      }
     
      else if (octet_entrant < 128) {  //si c'est une valeur
        switch (prochain_byte)
        {
          case 1:
            numero_note = octet_entrant;
            prochain_byte++;
            break;
           
          case 2:
            velocite = octet_entrant;
            nouvelle_note = true;  //marqueur indiquant qu'on a reçu une nouvelle note
            learn_ok = true;  //on a fini l'apprentissage
            prochain_byte = 0;
            break;
        }
      }
     
      else if (octet_entrant >= 160 && octet_entrant <= 239) {  //c'est un autre message contenant le numero du canal
        if (!notes_learn) {  //si on n'a pas active l'apprentissage de la note de base
          learn_ok = true;  //on a fini l'apprentissage
        }
        if (canal_learn) {  //si on a active l'apprentissage du canal midi
          canal_canal = octet_entrant & 0x0F;
        }
      }
 
    }
  }while(!learn_ok);
  if (notes_learn) {  //si on a active l'apprentissage de la note de base
    for(byte compteur = 0; compteur < 12; compteur++) {
      sorties_valeur[compteur] = numero_note + compteur;
    }
  }
}
 
//SORTIES=====================================================================
 
void sortie_clock() {
    clock_tic = false;
    bitSet(etat_sorties, 16);
    millisPrecedent[16] = millis();
    if(compteurTic == 0) {
      bitSet(etat_sorties, 15);
      millisPrecedent[15] = millis();
      compteurTic = sorties_valeur[15];
    }
    compteurTic--;
    seriOut(TRIGGERS, etat_sorties);
}
 
void sortie_transport() {
  nouveau_transport = false;
  bitWrite(etat_sorties, 14, !bitRead(etat_sorties, 14));  //on inverse l'etat de cette sortie
  compteurTic = 0;
  if(sorties_mode[14]) {  //si on est en mode trig
    millisPrecedent[14] = millis();
  }
  seriOut(TRIGGERS, etat_sorties);
}
 
void sortie_note() {
 
  nouvelle_note = false;
 
  for(byte compteur = 0; compteur < 12; compteur++) {
    if(sorties_valeur[compteur] == numero_note) {  //si c'est une des 8 notes assignees a une sortie
      if(running_status == MIDI_noteon && velocite > 0) {  //si on a bien affaire a une note on
        bitSet(etat_sorties, compteur);
        seriOut(TRIGGERS, etat_sorties);  //on envoie cette donnee sur le port serie des sorties
        if(sorties_mode[compteur]) {  //si cette sortie est en mode trig
          millisPrecedent[compteur] = millis(); //on enregistre le temps actuel pour pouvoir eteindre cette sortie en temps voulu
        }
      }
      else if(!sorties_mode[compteur]) {  //sinon c'est une note off et on ne la prend en compte que si on est en mode gate
        bitClear(etat_sorties, compteur);
        seriOut(TRIGGERS, etat_sorties); //on envoie cette donnee sur le port serie des sorties
      }
    }
  }
}
 
//SORTIE SERIE================================================================
 
byte seriOut(byte sortie, byte donnees) {
  byte dataPin;
  byte clockPin;
  byte latchPin;
  switch (sortie) {
      case TRIGGERS:
        dataPin = sorties_dataPin;
        clockPin = sorties_clockPin;
        latchPin = sorties_latchPin;
        break;
      case AFFICHAGE:
        dataPin = affichage_dataPin;
        clockPin = affichage_clockPin;
        latchPin = affichage_latchPin;
        break;
  }
  digitalWrite(latchPin, LOW);
  if(sortie == TRIGGERS){
    shiftOut(dataPin, clockPin, MSBFIRST, (donnees >> 8));      
  }
  shiftOut(dataPin, clockPin, MSBFIRST, donnees);
  digitalWrite(latchPin, HIGH);
}
 
//DELAIS======================================================================
 
void verifier_delais() {
    unsigned long millisActuel = millis();
    //extinction des sorties
    for(int compteur=0; compteur<16; compteur++){
        //si on a depasse le temps et que la sortie est en mode trigger et qu'elle est actuellement active :
        if((millisActuel - millisPrecedent[compteur]) > sorties_duree[compteur] && sorties_mode[compteur] == 1 && bitRead(etat_sorties, compteur) == 1){
            bitClear(etat_sorties, compteur); //on passe cette sortie en off
            seriOut(TRIGGERS, etat_sorties); //on envoie cette donnee sur le port serie des sorties
        }
    }
    //rafraichissement de l'affichage
    if(millisActuel - millisAffichage > refreshAffichage){
        affichage();
    }
}
 
//AFFICHAGE===================================================================
 
void affichage() {
    byte pin_digitActif;
    byte pin_digitInactif;
    byte donnees;
 
    //On active le digit qui n'etait pas actif lors du rafraichissement precedent
    if(digitalRead(affichage_digit1)){
        pin_digitActif = affichage_digit2;
        pin_digitInactif = affichage_digit1;
        donnees = affichage_digits[1];
    }
    else{
        pin_digitActif = affichage_digit1;
        pin_digitInactif = affichage_digit2;
        donnees = affichage_digits[0];
    }
 
    digitalWrite(pin_digitInactif, LOW);  //desactivation du digit precedent
    seriOut(AFFICHAGE, donnees);  //transmission des donnees sur le port AFFICHAGE
    digitalWrite(pin_digitActif, HIGH); //activation du digit courant
    millisAffichage = millis();
}
 
//CREATION DIGITS=============================================================
 
void creation_digits(char caracteres[], int nbCaracteres) {
  //TODO
  //ici, rechercher correspondance entre les caracteres en entree et le
  //tableau des digits pour en deduire le numero du digit a afficher.
  //Penser a afficher du vide devant lorsqu'il n'y a qu'un seul digit.
  //Un truc comme ca (mais il faut utiliser strcmp()):
  /*
  int i = 0 ;
  while(strcmp(caracteres[1], digits[i]) != 0) {
    i++;
  }
  affichage_digit1 = i;
  int i = 0 ;
  while(caracteres[2] != digits[i]) {
    i++;
  }
  affichage_digit2 = i;
  */
}
 
//ENTREE BOUTON===============================================================
 
//TODO
/*
for(int compteur=0; compteur<4; compteur++){
  unsigned long millisActuel = millis();  //on enregistre le temps actuel
  //lire l'entree du bouton-poussoir
  byte lecture = digitalRead(buttonPin);
 
  // comparer l'etat du bouton a son etat precedent
  if (lecture != dernier_etat_bouton[compteur]) { //si le statut du bouton a change
    bouton_millisPrecedent = millisActuel;  //on enregistre le temps actuel
  }
 
  if((millisActuel - bouton_millisPrecedent) > tempsDebounce){
    //si l'etat du bouton a change
    if(lecture != etat_bouton[compteur]) {
      etat_bouton[compteur] = lecture;
 
      //si le bouton est presse
      if(etat_bouton[compteur] == true) {
        nouvel_appui[compteur] = true;
        //TODO reste du code ici
      }    
    }      
  }
  dernier_etat_bouton[compteur] = lecture;
   
}
*/