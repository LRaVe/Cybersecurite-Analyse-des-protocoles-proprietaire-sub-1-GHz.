# Outil d'Attaque RollJam

## ⚠️ À DES FINS DE RECHERCHE EN SÉCURITÉ ET D'ÉDUCATION UNIQUEMENT ⚠️

Cette application Flipper Zero démontre des techniques permettant de **contourner la sécurité des codes roulants** utilisée dans les clés de voiture, les ouvreurs de portes de garage et autres systèmes d'accès sans fil.

## L'Attaque RollJam Expliquée

Les codes roulants ont été conçus pour prévenir les simples attaques par rejeu. Cependant, **l'attaque RollJam** (découverte par Samy Kamkar) peut contourner cette protection.

### Comment Fonctionnent les Codes Roulants
```
Fonctionnement Normal :
┌─────────────────┐         ┌─────────────────┐
│   Clé           │  Code1  │   Voiture/Garage│
│   Compteur: 5   │ ──────► │   Compteur: 5   │
│   Compteur → 6  │         │   Compteur → 6  │
└─────────────────┘         └─────────────────┘

Attaque par Rejeu (BLOQUÉE):
┌─────────────────┐         ┌─────────────────┐
│   Attaquant     │  Code1  │   Voiture/Garage│
│   (ancien code) │ ──────► │   Compteur: 6   │
│                 │         │   REJETÉ!       │
└─────────────────┘         └─────────────────┘
```

### L'Attaque RollJam
```
Étape 1 : La victime appuie sur le bouton
┌─────────────────┐         ┌─────────────────┐
│   Clé           │  Code1  │   BROUILLÉ!     │
│   Compteur: 5   │ ──╳───► │   (pas de signal)
│   Compteur → 6  │         │                 │
└─────────────────┘         └─────────────────┘
         │
         ▼ L'attaquant capture Code1

Étape 2 : La victime appuie à nouveau (pense que la première fois n'a pas marché)
┌─────────────────┐         ┌─────────────────┐
│   Clé           │  Code2  │   Reçoit        │
│   Compteur: 6   │ ──╳───► │   Code1!        │ ← L'attaquant rejoue
│   Compteur → 7  │         │   Compteur → 6  │
└─────────────────┘         └─────────────────┘
         │
         ▼ L'attaquant capture Code2

Résultat : L'attaquant a Code2 VALIDE NON UTILISÉ!
┌─────────────────┐         ┌─────────────────┐
│   Attaquant     │  Code2  │   Voiture/Garage│
│   (code frais!) │ ──────► │   Compteur: 6   │
│                 │         │   ACCEPTÉ!      │
└─────────────────┘         └─────────────────┘
```

## Fonctionnalités

### 1. Mode Attaque RollJam
Attaque automatisée complète :
- Active et attend le signal de la victime
- Brouille le récepteur tout en capturant le premier code
- Attend le deuxième appui de la victime
- Rejoue le premier code (la victime pense que ça a marché)
- Capture le deuxième code (votre code valide non utilisé !)
- Prêt à utiliser à tout moment

### 2. Mode Capture de Code
Capture passive sans brouillage :
- Écouter et enregistrer les signaux
- Stocker plusieurs codes
- Enregistrer sur carte SD sous forme de fichiers `.sub`

### 3. Mode Rejeu
Transmettre les codes capturés :
- Sélectionner à partir des codes capturés
- Rejouer à la demande
- Suivre le nombre de rejeux

### 4. Brouillage Continu
Bloquer les signaux à la fréquence cible :
- Activer/désactiver le brouillage
- Utile pour tester la portée

### 5. Analyser les Captures
Afficher les statistiques :
- Codes capturés
- Signaux détectés
- Emplacements des fichiers

## Utilisation

### Attaque RollJam
1. Sélectionner **Attaque RollJam** dans le menu
2. Appuyer sur **OK** pour activer
3. Attendre près de la cible (voiture/garage de la victime)
4. Quand la victime appuie sur sa télécommande :
   - L'app brouille + capture Code1
   - La victime pense que ça n'a pas marché
5. La victime appuie à nouveau :
   - L'app capture Code2, rejoue Code1
   - L'appareil de la victime s'ouvre (elle s'en va)
6. **SUCCÈS !** Vous avez Code2 non utilisé
7. Appuyer sur **OK** pour utiliser Code2 à tout moment

### Capture Simple
1. Sélectionner **Capturer les Codes**
2. Appuyer sur **OK** pour commencer à écouter
3. Les codes capturés sont sauvegardés sur la carte SD
4. Utiliser le mode **Rejeu** pour transmettre

## Détails Techniques

- **Fréquence** : 433,92 MHz (configurable)
- **Décalage de Brouillage** : +50 kHz (brouillage sélectif)
- **Taille du Tampon** : 16 KB pour la capture de signal
- **Codes Max** : 10 stockés en mémoire
- **Fichiers Sauvegardés** : `/ext/subghz/rolljam/`

## Ajuster pour Votre Cible

Éditer ces defines dans `rolljam_app.c` :

```c
#define TARGET_FREQUENCY    433920000   // Courant : 433.92, 315, 868 MHz
#define JAM_OFFSET          50000       // Décalage de fréquence de brouillage
#define RSSI_THRESHOLD      -65.0f      // Sensibilité de détection de signal
```

Fréquences courantes :
- **433,92 MHz** - Portes de garage en Europe, nombreuses clés de voiture
- **315 MHz** - Clés de voiture US/Japon
- **868 MHz** - Systèmes plus récents en Europe

## Fichiers

```
rolljam_app/
├── application.fam      # Manifeste de l'app
├── rolljam_app.c        # Source principale (~900 lignes)
├── rolljam.png          # Icône de l'app
└── images/              # Dossier d'assets
```

## Compilation

```bash
cd rolljam_app
ufbt
ufbt launch  # Pour déployer sur Flipper
```


## Références

- Présentation RollJam de Samy Kamkar (DEF CON 23)
- Papiers d'analyse des codes roulants / KeeLoq
- Documentation Flipper Zero Sub-GHz
