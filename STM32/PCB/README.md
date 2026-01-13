Ce projet consiste en la conception d'un shield au format Arduino R3 pour la carte STM32 Nucleo-WL55JC2. 
Il permet l'affichage d'informations sur un écran OLED et le contrôle manuel de fonctions de lecture (Rx) et d'émission (Tx) via deux boutons poussoirs.
## 📋 Spécifications Matérielles
Microcontrôleur cible : STM32 Nucleo-WL55JC2 (Logique $3.3 \text{ V}$).
Affichage : Écran OLED SSD1315 via connecteur J5 (7 pins).
Protocole d'affichage : SPI (plus rapide que l'I2C pour les animations).
Contrôles : 2x Boutons poussoirs tactiles avec résistances de rappel externes.
## Logique des Boutons (Active High)
Contrairement aux montages pull-up internes classiques, ce projet utilise des résistances Pull-Down externes pour garantir une immunité au bruit maximale et un signal stable.
\begin{table}[h]
\centering
\begin{tabular}{|l|l|l|l|}
\hline
\textbf{Valeur} & \textbf{Avantages} & \textbf{Inconvénients} & \textbf{Usage} \\ \hline
$10\text{ k}\Omega$ & Basse consommation & Sensibilité aux bruits & \textbf{Recommandé} \\ \hline
$4.7\text{ k}\Omega$ & Compromis idéal & Consommation moyenne & Signal rapide \\ \hline
$1\text{ k}\Omega$ & Très stable & Consommation élevée & Milieu bruité \\ \hline
\end{tabular}
\caption{Choix des résistances Pull-Down pour S1 et S2}
\end{table}
## Alimentation
Le shield est alimenté par le rail $+3V3$ de la Nucleo.Les boutons tirent leur alimentation en parallèle de la ligne de l'écran OLED pour simplifier le routage.
>> L'utilisation du $+5 \text{ V}$ est strictement interdite pour éviter d'endommager les GPIO du STM32.
## Brochage (Pinout)
\begin{table}[h]
\centering
\begin{tabular}{|l|l|l|l|}
\hline
\textbf{Signal} & \textbf{Pin Shield} & \textbf{Pin STM32} & \textbf{Fonction} \\ \hline
BTN\_RX & D2 & PA10 & Commande Lecture \\ \hline
BTN\_TX & D3 & PB3 & Commande Émission \\ \hline
OLED\_CS & D10 & PA4 & Chip Select SPI \\ \hline
OLED\_DC & D9 & PA8 & Data/Command Select \\ \hline
OLED\_RES & D8 & PA9 & Reset Display \\ \hline
SPI\_MOSI & D11 & PA7 & Data Out \\ \hline
SPI\_SCK & D13 & PA5 & Horloge SPI \\ \hline
\end{tabular}
\caption{Assignation des broches du microcontrôleur}
\end{table}
