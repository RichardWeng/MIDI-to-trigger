  //AFFICHAGE=================================================================

    void affichage() {
      byte pin_digitActif;
      byte pin_digitInactif;
      byte donnees;
    
      //On active le digit qui n'etait pas actif lors de la lecture precedente
      boolean lecture = digitalRead(affichage_digit[0]);
      pin_digitActif = affichage_digit[lecture];
      pin_digitInactif = affichage_digit[!lecture];
      donnees = segmentsAffichage[lecture];
   
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
      
      //int nombre = strtol(caracteres,(char **)NULL,10);



      byte compteur = 0;

      for(byte i=0; i < 2; i++){
        compteur = 0;
        while(caracteres[i] != tableauDigits[compteur].symbole) {
          compteur++;
        }
        //Si le premier caractere est un zero, on ne l'affiche pas
        if(i == 0 && compteur == 0){
          segmentsAffichage[i] = 0;
        }
        else {
          segmentsAffichage[i] = tableauDigits[compteur].segments;
        }
      }
    }

    void ajoutPointDecimal() {
      segmentsAffichage[1] = segmentsAffichage[1] | tableauDigits[36].segments;
    }
