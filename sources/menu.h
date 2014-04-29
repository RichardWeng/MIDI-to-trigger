void menuSuivant();

void menuPrecedent();

void menuParent();

void menuEnfant();

void menuStandard(Boutons dernierBoutonPresse);

void divisionHorloge(Boutons dernierBoutonPresse);

void choixCanal(Boutons dernierBoutonPresse);

//Verifie si le menu teste possede des enfants
boolean possedeDesEnfants(byte menuTeste);

void afficherChiffres(byte nombre);

byte changerValeur(Operation operation, byte variable, byte minimum, byte maximum);
