Ce projet consiste en la conception d'un shield (bouclier) au format Arduino R3 pour la carte STM32 Nucleo-WL55JC2. 
Il permet l'affichage d'informations sur un écran OLED et le contrôle manuel de fonctions de lecture (Rx) et d'émission (Tx) via deux boutons poussoirs.
## 📋 Spécifications Matérielles
Microcontrôleur cible : STM32 Nucleo-WL55JC2 (Logique $3.3 \text{ V}$).
Affichage : Écran OLED SSD1315 via connecteur J5 (7 pins).
Protocole d'affichage : SPI (plus rapide que l'I2C pour les animations).
Contrôles : 2x Boutons poussoirs tactiles avec résistances de rappel externes.
## Logique des Boutons (Active High)
Contrairement aux montages pull-up internes classiques, ce projet utilise des résistances Pull-Down externes pour garantir une immunité au bruit maximale et un signal stable.
Composant,Valeur,Fonction,État au repos,État pressé
S1 (Read),N/A,Déclenche la lecture (Rx),LOW (0 V),HIGH (3.3 V)
S2 (Send),N/A,Déclenche l'émission (Tx),LOW (0 V),HIGH (3.3 V)
"R1, R2",10 kΩ,Résistances Pull-Down,Maintenir le signal à 0 V,N/A
## Alimentation
Le shield est alimenté par le rail $+3V3$ de la Nucleo.Les boutons tirent leur alimentation en parallèle de la ligne de l'écran OLED pour simplifier le routage.
>> L'utilisation du $+5 \text{ V}$ est strictement interdite pour éviter d'endommager les GPIO du STM32.
## Brochage (Pinout)
Signal,Pin Shield (Arduino),Pin STM32 (Nucleo)
BTN_RX (S1),D2,PA10
BTN_TX (S2),D3,PB3
OLED_CS,D10,PA4
SPI_MOSI,D11,PA7
SPI_SCK,D13,PA5
