//MIDI======================================================================
    void handleNoteOn(byte inChannel, byte inNote, byte inVelocity) {

      //Si l'on doit apprendre le canal ou les notes
      if(learnFlag && (parametres.NotesLearn || parametres.CanalLearn)) {
        learn(inChannel, inNote);
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
          //On active la sortie accent si besoin
          if(inVelocity >= parametres.ParamSortie[accent]) {
            bitSet(etat_sorties, accent);
            millisTriggers[accent] = millis();
          }
        }
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

    void learn(byte inChannel, byte inNote) {

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