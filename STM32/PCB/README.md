Ce projet consiste en la conception d'un shield au format Arduino R3 pour la carte STM32 Nucleo-WL55JC2. 
Il permet l'affichage d'informations sur un écran OLED et le contrôle manuel de fonctions de lecture/sauvegarde (Rx) et d'émission (Tx) via deux boutons poussoirs.
##  Spécifications Matérielles
Microcontrôleur cible : STM32 Nucleo-WL55JC2 (Logique $3.3 \text{ V}$).
Affichage : Écran OLED SSD1315 via connecteur J5 (7 pins).
Protocole d'affichage : SPI (plus rapide que l'I2C pour les animations).
Contrôles : 2x Boutons poussoirs tactiles avec résistances de rappel externes.
## Logique des Boutons (Active High)
Ce projet utilise des résistances Pull-Down externes pour garantir une immunité au bruit maximale et un signal stable, plus fiable de les configurées.

Composant >  S1 (Read), Valeur > N/A, Fonction > Déclenche la lecture (Rx),État au repos > LOW (0 V), État pressé > HIGH (3.3 V)

Composant >S2 (Send), Valeur > N/A, Fonction > Déclenche l'émission (Tx),État au repos > LOW (0 V), État pressé > HIGH (3.3 V)

Composant > "R1, R2", Valeur > 10 kΩ, Fonction > Résistances Pull-Down,État au repos > Maintenir le signal à 0 V, État pressé > N/A
## Alimentation
Le shield est alimenté par le rail $+3V3$ de la Nucleo.Les boutons tirent leur alimentation en parallèle de la ligne de l'écran OLED pour simplifier le routage.
> L'utilisation du $+5 \text{ V}$ est strictement interdite pour éviter d'endommager les GPIO du STM32.
## Brochage (Pinout)
Signal > BTN_RX (S1),Pin Shield (Arduino) > D2,Pin STM32 (Nucleo) > PA10

Signal > BTN_TX (S2),Pin Shield (Arduino) > D3,Pin STM32 (Nucleo) > PB3

Signal > OLED_CS,Pin Shield (Arduino) > D10,Pin STM32 (Nucleo) > PA4

Signal > SPI_MOSI,Pin Shield (Arduino) > D11,Pin STM32 (Nucleo) > PA7

Signal > SPI_SCK,Pin Shield (Arduino) > D13,Pin STM32 (Nucleo) > PA5
