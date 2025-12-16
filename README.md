# Projet 2A -  Cybersecurite-Analyse-des-protocoles-proprietaire-sub-1-GHz

## Description du Projet
Bienvenue sur le dépôt GitHub de notre projet de cybersécurité, réalisé dans le cadre de notre projet de deuxième année. Ce projet repose sur l'utilisation du flipper zero, et ensuite de le reproduire avec une STM32WL55.

## 👨‍👩‍👦 Équipe du Projet
Nous sommes une équipe de cinq étudiants :

- **Lucas Raveloarinoro**
- **Hevisinda Top**
- **Wassim Makni**
- **Awa Fofana**
- **Elhoussaine Assanfe**

## 🎯 Objectifs : 4 Démonstrations et une application

Démo A : Flipper Zero — attaque simple (sans chiffrement) sur la sonnette.

Démo B : Flipper Zero — tentative d’attaque sur la clé de voiture (rolling‑code).

Démo C : STM32WL55 — reproduction de l’attaque simple (sonnette).

Démo D : STM32WL55 — expérimentation et analyse du rolling‑code (clé de voiture).

- La création d’une application sur Flipper capable passivement d’écouter les communications, et en déduire les faiblesses de sécurités des appareils écoutés.


## Sous-Projet FlipperZero

Rappel Objectif : utiliser les composants inclus dans le FlipperZero pour reconsrtruire du début une application de capture et de relecture de signaux sub-GHz.

### Avancée Dec. 2025 

L'application parvient à récupérer des paquets envoyés de fréquence $\textbf{433.92\~ MHz}$. Il ne manque maintenant que la partie relecture, même s'il est possible de le récupérer directement dans les fichiers du flipper.
