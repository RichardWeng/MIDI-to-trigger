Interface MIDI-Trigger sur arduino
==================================

Convertit les notes midi entrantes en triggers et/ou gates pour contrôler synthétiseurs et boîtes à rythmes analogiques.

_**Développement en cours**_

## Fonctionnalités

### Sorties
* 12 sorties trigger / gate ;
* 1 sortie accent ;
* 1 sortie transport (play/stop) ;
* 1 sortie clock à 24ppqn ;
* 1 sortie clock divisible.

### Fonctions
* Division d'horloge de 1:1 à 1:96 ;
* Réglage du seuil de vélocité à partir duquel l'accent est déclenché ;
* assignation des notes aux sorties par apprentissage ou par sélection dans les options ;
* sélection du canal midi écouté par apprentissage ou manuellement ;
* sélection du mode start / stop (trigger ou gate) ;
* sélection de la durée du trigger pour chaque sortie séparément ;
* sélection du mode (trigger ou gate) pour chaque sortie séparément ;
* sauvegarde des paramètres pour les retrouver à chaque démarrage.

## Matériel

### Schémas

### Liste des composants

## Manuel d'utilisation

####Page note (no)
Permet d'assigner les notes reçues sur l'entrée MIDI aux différentes sorties.
* Learn (Ln) : les notes sont assignées aux sorties dans leur ordre d'arrivée. Par exemple, si l'on presse C1, D2, E1, B#1... C1 sera assignée à la sortie 1, D2 à la sortie 2, E1 à la sortie 3 et ainsi de suite.
* Auto (Au) : la première note reçue par l'interface se verra assignée à la sortie 1. Les autres sorties se verront assigner les notes suivantes dans la gamme chromatique ascendante.
* Manuel (Ma) : pour chaque sortie, on choisit la note associée à l'aide de la molette.

####Page clock division (di)
Permet de choisir le facteur de division de l'horloge MIDI
* Choix à l'aide de la molette : de 1 à 96 *(définir les incréments)*

####Page chanel (ch)
Permet de choisir avec la molette le canal midi écouté par le module.
* Learn (Ln) : le module se cale sur le canal du premier message MIDI qu'il reçoit.
* 1-16 : le canal MIDI écouté est défini manuellement

####Page start/stop mode (St)
Permet de choisir avec la molette le mode de sortie des signaux start/stop
* Trigger (tr) : la sortie start/stop produit une impulsion de xx microsecondes à chaque réception d'un signal MIDI start ou stop
* Gate (gt) : la sortie start/stop passe à l'état haut à la réception d'un signal MIDI start et passe à l'état bas lors de la réception d'un signal MIDI stop

####Page trig mode (tr)


## Documentation
*(Pour le développement)*

* http://arduinomidilib.fortyseveneffects.com/
* http://ralphniels.nl/arduino/using-enums-to-make-your-code-more-readable/
* http://projectgus.com/2010/07/eeprom-access-with-arduino/
* http://fr.openclassrooms.com/informatique/cours/les-pointeurs-sur-fonctions-1
* http://playground.arduino.cc/Main/MIDILibraryCallbacks
* http://www.notesandvolts.com/2013/01/fun-with-arduino-arduino-as-isp.html
* http://blogs.bl0rg.net/netzstaub/2008/08/18/sysex-bootloader-for-avr/
* http://www.gweep.net/~prefect/eng/reference/protocol/midispec.html

