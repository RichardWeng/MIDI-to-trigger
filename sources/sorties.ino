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