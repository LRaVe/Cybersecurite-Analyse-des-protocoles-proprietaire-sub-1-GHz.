Compte rendu Séance 30/09/2025

Suite à la dernière séance, et en attente de recevoir les deux Nucleo, nous avons profité de cette séance pour fixer deux groupes de travail ainsi que les sorties de notre projet. 

On a décidé de former deux groupes : 

- Un premier groupe qui va concentrer ses efforts sur le **Flipper** et qui va coder une nouvelle application qui utilise le sub-GHz. Ce groupe est composé de Lucas et d’Awa.   
- Le second groupe, composé d’Elhoussaine, Wassim et Hevi, travaillera avec les **STM32**, l’idée est de reproduire le comportement du Flipper Zero avec une Nucleo.

Les cibles choisies pour les deux groupes sont : 

- cible facile : sonnette   
- cible difficile, avec du rolling-code : clé de voiture.

Les sorties du projet sont : 

- **Quatre démonstrations** : D’abord en Flipper puis sur STM32 avec les deux cibles (facile et difficile) afin de comparer la reproduction du flipper sur STM32 avec le système original.  
- la **création d’une application sur Flipper** capable passivement d’écouter les communications, et en déduire les faiblesses de sécurités des appareils écoutés.

Ainsi, nous avons débuté nos recherches pour voir ce qui a déjà été fait. 

Dans le groupe STM32 : 

- documentation : compréhension du fonctionnement pour mieux le reproduire

Nous avons découvert que, pour contrer le rolling-code, il existe une méthode « **Roll Jamming** » qui consiste à ce que l’attaquant brouille (jam) la première transmission pour que le récepteur ne la reçoive pas, puis envoie rapidement un deuxième code qu’il a lui-même transmis, le récepteur l’accepte comme valide. Pendant ce temps, l’attaquant capture la première trame brouillée et la conserve pour la rejouer ultérieurement. Cette méthode exploite le fait que l’algorithme de code tournant suppose généralement que chaque envoi réussit et ne détecte pas (ou ne gère pas bien) les transmissions manquantes, ce qui permet de dérégler la synchronisation émetteur-récepteur.

Nous avons trouvé des exemples de code d’attaque “Roll Jam” en C++, JavaScript et Arduino. On devra donc essayer de le coder en STM32. 

Piste de recherche : 

- profiter de la STM32 pour élargir le flipper à une méthode d'espionnage 

Dans le groupe Flipper : 

- Définit l’application à faire

Nous avons également structuré les différentes étapes nécessaires pour réussir à obtenir ces sorties : 

Pour la STM32 : ( Wassim, Elhoussaine, Hevi) 

objectif 1 : reproduire sonnette ( en jaune c’est à chercher)

- etape 1 : comprendre le fonctionnement flipper en SubGhz pour mieux le recoder  
- etape 2 : construire un code capable de   
  - capter le signal : voir si c’est possible de l’afficher pour le visualiser : allumer la led lorsque le signal est envoyé  
  - le reproduire : trouver un moyen d'accuser la réception : utiliser par exemple la deuxième Nucleo pour recevoir le signal (led qui s’allume)  
  - L’utiliser sur la sonnette

objectif 2 : Clé voiture

- étape 1 : comprendre le fonctionnement du rolling code  
- étape 2 : faire fonctionner le Roll Jam sans la voiture : afficher les clés simulés à chaque fois  
- étape 2 : l’adapter à la voiture

Pour le Flipper Zero : (Awa, Lucas) : 

Objectif 1: 

Nous allons créer une application qui nous permettra d’écouter les communications en sub 1GHZ et qui essaie de déduire les failles de sécurité des appareils connectés.

Objectif 2:

A partir de ces faiblesses nous allons effectuer des attaques avec le flipper zéro 

Nous avons construit un Gantt sous forme de excel pour nous poser des deadlines et mieux visualiser notre calendrier.


Ainsi, à la prochaine séance, on va : 

STM32 : commencer le code pour capter un signal

Flipper: Installation des package pour pouvoir développer une application sur le flipper 

