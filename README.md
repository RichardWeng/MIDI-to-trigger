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

####Note
Learn (Ln) : les notes sont assignées aux sorties dans leur ordre d'arrivée. Par exemple, si l'on presse C1, D2, E1, B#1... C1 sera assignée à la sortie 1, D2 à la sortie 2, E1 à la sortie 3 et ainsi de suite.
Auto (Au) : la première note reçue par l'interface se verra assignée à la sortie 1. Les autres sorties se verront assigner les notes suivantes dans la gamme chromatique ascendante.
Manuel (Ma) : pour chaque sortie, on choisit la note associée à l'aide de la molette.


