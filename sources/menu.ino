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
}

void divisionHorloge(Boutons dernierBoutonPresse) {

	switch (dernierBoutonPresse) {

	  case Echap:
	    menuParent();
	    strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	    break;
	    
	  case Gauche:
	  	if (parametres.Canal == 0) {
	  		parametres.Canal = 17;
	  	}
	    parametres.Canal--;
	    break;

	  case Droite:
	  	parametres.Canal++;
	  	if (parametres.Canal > 16) {
	  		parametres.Canal = 0;
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
		if(parametres.Canal == 0) {
			strncpy(donneesAffichage, "Ln", 3);
		}
		else {
			afficherChiffres(parametres.Canal);
		}
	}

}

void choixCanal(Boutons dernierBoutonPresse) {

	switch (dernierBoutonPresse) {

	  case Echap:
	    menuParent();
	    strncpy(donneesAffichage, menu[menuAffiche].nom, 3);
	    break;
	    
	  case Gauche:
	  	if (parametres.Canal == 0) {
	  		parametres.Canal = 17;
	  	}
	    parametres.Canal--;
	    break;

	  case Droite:
	  	parametres.Canal++;
	  	if (parametres.Canal > 16) {
	  		parametres.Canal = 0;
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
		if(parametres.Canal == 0) {
			strncpy(donneesAffichage, "Ln", 3);
		}
		else {
			afficherChiffres(parametres.Canal);
		}
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
