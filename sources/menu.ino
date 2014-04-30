void menuSuivant() {
	Serial.println("Suivant");
	do {
		menuAffiche++;
		if(menuAffiche > nombreItems) {
		  menuAffiche = 0;
		}
	} while (menu[menuAffiche].id_parent != menuCourant);
}

void menuPrecedent() {
	Serial.println("Precedent");
  do {
    if(menuAffiche == 0) {
      menuAffiche = nombreItems;
    }
    menuAffiche--;
  } while (menu[menuAffiche].id_parent != menuCourant);
}

void menuParent() {
	Serial.println("Menu parent");
  //Si on n'est pas a la racine
  if(menuCourant > -1) {
  	menuAffiche = menuCourant;
    menuCourant = menu[menuCourant].id_parent;
  }
}

void menuEnfant() {
	Serial.println("Entree");
  //Si le menu sur lequel on vient de cliquer possede des enfants
  //if(possedeDesEnfants(menuAffiche)) {
    menuCourant = menuAffiche;
    (*menu[menuCourant].fonction)(Aucun);
  //}
}

void menuStandard(Boutons dernierBoutonPresse) {

	switch (dernierBoutonPresse) {

	  case Echap:
	    menuParent();
	    strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	    break;
	    
	  case Gauche:
	    menuPrecedent();
	    strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	    break;

	  case Droite:
	  	menuSuivant();
	  	strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	    break;

	  case Entree:
	    menuEnfant();
	    break;

	  case Aucun:
	  	menuSuivant();
	  	strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	  	break;
	  
	  default:
	    Serial.println("Erreur traitementBouton");
	}
	creation_digits(donneesAffichage);
}

void divisionHorloge(Boutons dernierBoutonPresse) {

	switch (dernierBoutonPresse) {

	  case Echap:
	    menuParent();
	    strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	    break;
	    
	  case Gauche:
	  	if (parametres.ParamSortie[div_clock] == 1) {
	  		parametres.ParamSortie[div_clock] = 97;
	  	}
	    parametres.ParamSortie[div_clock]--;
	    break;

	  case Droite:
	  	parametres.ParamSortie[div_clock]++;
	  	if (parametres.ParamSortie[div_clock] > 96) {
	  		parametres.ParamSortie[div_clock] = 1;
	  	}
	    break;

	  case Entree:
	    //menuEnfant();
	    break;

	  case Aucun:
	  Serial.println("Aucun");
	  	break;
	  
	  default:
	    Serial.println("Erreur traitementBouton");
	}

	if(dernierBoutonPresse != Echap) {
		if(parametres.ParamSortie[div_clock] == 0) {
			strncpy(donneesAffichage, "Ln", 3);
		}
		else {
			afficherChiffres(parametres.ParamSortie[div_clock]);
		}
	}

}

void choixCanal(Boutons dernierBoutonPresse) {

	switch (dernierBoutonPresse) {

	  case Echap:
	  	//On se place dans le menu parent
	    menuParent();
	    Serial.println(donneesAffichage);
	    //On affiche le nom du menu dans lequel on etait
	    strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	    Serial.println(donneesAffichage);
	    //Et on sort de la
	    return;
	    break;
	    
	  case Gauche:
	  	bufferParametre = changerValeur(REDUIRE, bufferParametre, 0, 16);
	    break;

	  case Droite:
	  	bufferParametre = changerValeur(AUGMENTER, bufferParametre, 0, 16);
	    break;

	  case Entree:
	  	//On enregistre la valeur actuelle du buffer
	    parametres.Canal = bufferParametre;
	    break;

	  case Aucun:
	  	//Si on vient d'entrer dans ce menu
	  	Serial.println("Aucun");
	  	bufferParametre = parametres.Canal;
	  	break;
	  
	  default:
	    Serial.println("Erreur traitementBouton");
	}

	if(bufferParametre == 0) {
		strncpy(donneesAffichage, "Ln", 3);
	}
	else {
		afficherChiffres(bufferParametre);
	}
	creation_digits(donneesAffichage);
	//Si le buffer est different de la valeur enregistree
	if(bufferParametre != parametres.Canal) {
		//On le signifie au moyen du point decimal des unites
		ajoutPointDecimal();
	}

}

//Verifie si le menu teste possede des enfants
boolean possedeDesEnfants(byte menuTeste) {
  boolean enfantsTrouves = false;
  for(byte i=0; i<nombreItems; i++){
    if(menu[i].id_parent == menuTeste) {
      enfantsTrouves = true;
    }
  }
  return enfantsTrouves;
}

void afficherChiffres(byte nombre) {
	//dizaines
	donneesAffichage[0] = tableauDigits[nombre / 10 % 10].symbole;
	//unites
	donneesAffichage[1] = tableauDigits[nombre % 10].symbole;
}

byte changerValeur(Operation operation, byte variable, byte minimum, byte maximum) {
	
	switch (operation) {
	    
	    case AUGMENTER:
	      variable++;
	  		if (variable > maximum) {
	  			variable = minimum;
	  		}
	      break;

	    case REDUIRE:
	      if (variable == minimum) {
	  			variable = maximum + 1;
	  		}
	    	variable--;
	      break;
	    
	    default:
	      Serial.println("Erreur changerValeur");
	}
	return variable;
}
