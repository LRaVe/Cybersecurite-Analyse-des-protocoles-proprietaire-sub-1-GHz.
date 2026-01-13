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

## Objectifs : 4 Démonstrations et une application

Démo A : Flipper Zero — attaque simple (sans chiffrement) sur la sonnette.

Démo B : Flipper Zero — tentative d’attaque sur la clé de voiture (rolling‑code).

Démo C : STM32WL55 — reproduction de l’attaque simple (sonnette).

Démo D : STM32WL55 — expérimentation et analyse du rolling‑code (clé de voiture).

- La création d’une application sur Flipper capable passivement d’écouter les communications, et en déduire les faiblesses de sécurités des appareils écoutés.


## Sous-Projet FlipperZero Application

Rappel Objectif : utiliser les composants inclus dans le FlipperZero pour reconsrtruire du début une application de capture et de relecture de signaux sub-GHz.

### Avancée Dec. 2025 

L'application parvient à récupérer des paquets envoyés de fréquence $\textbf{433.92\~ MHz}$. Il ne manque maintenant que la partie relecture, même s'il est possible de le récupérer directement dans les fichiers du flipper.

### Avancée Jan. 2026 - Application Fonctionnelle ✅

L'application est maintenant **complètement fonctionnelle** avec capture et replay de signaux radio.

#### Architecture de l'Application

```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION SUBGHZ                       │
├─────────────────────────────────────────────────────────────┤
│  CAPTURE (app_state=0)                                      │
│  └── furi_hal_subghz_start_async_rx() → capture_callback()  │
│      └── Filtrage du bruit → Stockage dans raw_buffer[]     │
│                                                             │
│  AFFICHAGE (app_state=1)                                    │
│  └── Lecture du fichier .sub sauvegardé                     │
│                                                             │
│  REPLAY (app_state=2)                                       │
│  └── Transmission manuelle TX/IDLE avec timings précis      │
└─────────────────────────────────────────────────────────────┘
```

#### APIs Flipper Zero Utilisées

| API | Fonction |
|-----|----------|
| `furi_hal_subghz_*` | Contrôle du module radio CC1101 (433.92 MHz) |
| `furi_hal_subghz_start_async_rx()` | Démarrage de la capture asynchrone |
| `furi_hal_subghz_tx()` / `furi_hal_subghz_idle()` | Émission/arrêt pour le replay |
| `furi_thread_*` | Threads pour capture/replay non-bloquants |
| `storage_*` / `stream_*` | Lecture/écriture fichiers .sub sur SD |
| `gui_*` / `canvas_*` | Interface utilisateur |

#### Problèmes Rencontrés et Solutions

##### 1. Capture de bruit en permanence
**Problème** : L'antenne capte énormément de bruit électromagnétique ambiant, rendant impossible la distinction d'un vrai signal.

**Solution** : Filtrage multi-niveaux dans `capture_callback()` :
```c
// Ignorer les durées < 150µs (bruit haute fréquence)
if(duration < 150) return;

// Ignorer les durées > 20ms (silence/pause)
if(duration > 20000) return;

// Attendre 30 impulsions valides consécutives avant d'enregistrer
if(consecutive_pulses < 30) return;
```

##### 2. API `subghz_devices` non fonctionnelle
**Problème** : L'API `subghz_devices` (plus récente) causait des blocages - l'appareil ne trouvait pas le device radio.

**Solution** : Utilisation de l'API bas niveau `furi_hal_subghz` qui fonctionne directement avec le CC1101 :
```c
furi_hal_subghz_reset();
furi_hal_subghz_idle();
furi_hal_subghz_load_custom_preset(subghz_device_cc1101_preset_ook_650khz_async_regs);
furi_hal_subghz_set_frequency_and_path(433920000);
```

##### 3. Crash lors du replay avec `async_tx`
**Problème** : L'utilisation de `furi_hal_subghz_start_async_tx()` avec un callback provoquait des crashes (contexte d'interruption incompatible avec certaines fonctions).

**Solution** : Transmission manuelle en alternant `tx()` et `idle()` avec des délais précis :
```c
for(size_t i = 0; i < tx_buffer_size; i++) {
    if(timing > 0) {
        furi_hal_subghz_tx();      // Émet
    } else {
        furi_hal_subghz_idle();    // Silence
    }
    furi_delay_us(abs(timing));    // Attend la durée exacte
}
```

##### 4. UI bloquée sur "ENVOI EN COURS"
**Problème** : Après la transmission, l'écran restait bloqué et n'affichait jamais "TERMINÉ".

**Cause** : L'appel `furi_hal_subghz_sleep()` bloquait indéfiniment le thread.

**Solution** : Suppression de `furi_hal_subghz_sleep()` - la radio reste en mode `idle` ce qui est suffisant.

##### 5. Synchronisation des flags entre threads
**Problème** : Les variables `is_replaying` et `replay_finished` n'étaient pas toujours visibles entre le thread de replay et le thread principal (UI).

**Solution** : Utilisation de `volatile` pour les variables partagées :
```c
volatile bool is_replaying;
volatile bool replay_finished;
```

#### Format du Fichier .sub

```
Filetype: Flipper SubGhz RAW File
Version: 1
Frequency: 433920000
Preset: FuriHalSubGhzPresetOok650Async
Protocol: RAW
RAW_Data: 1001 -352 379 -986 353 -10750 385 -954 999 -350 ...
RAW_Data: 379 -966 1007 -366 987 -368 987 -366 353 -984 ...
```

- **Valeurs positives** : durée HIGH (émission) en µs
- **Valeurs négatives** : durée LOW (silence) en µs

## Sous-Projet STM32

### Objectif

L’objectif principal de ce sous-projet est de reproduire le comportement du Flipper Zero à l’aide d’une carte Nucleo STM32, afin de comprendre le fonctionnement des communications radio Sub-GHz et d’évaluer la possibilité de hacker une sonnette sans fil fonctionnant à 433 MHz.

Pour cela, nous avons utilisé une carte **STM32WL55JC2**, qui intègre un module de communication radio Sub-GHz. Ce type de communication est couramment employé dans des applications telles que les capteurs sans fil, les stations météorologiques ou les systèmes à longue portée et faible consommation. La carte supporte principalement les modulations **LoRa** et **FSK**.


### Mise en place de la communication Sub-GHz  
*(Adaptation de l'exemple PingPong)*

Dans un premier temps, nous avons mis en place une communication simple entre deux cartes STM32 en utilisant un fonctionnement Ping-Pong. Une carte agit comme émetteur et envoie un message, tandis que l’autre agit comme récepteur et renvoie une réponse. Cette méthode permet de valider le bon fonctionnement de la transmission et de la réception radio, ainsi que la stabilité de la liaison.

La communication a ensuite été contrôlée à l’aide des boutons présents sur les cartes.

Pour la carte correspondant au **Flipper Zero** :

- **Bouton 2** : envoi du signal radio stocké  
- **Bouton 1** : réception et stockage d’un signal radio  

Plusieurs scénarios ont été testés en alternant les rôles des cartes afin de vérifier que l’émission et la réception fonctionnent correctement dans les deux sens. Ces tests ont permis de confirmer que la communication Sub-GHz entre deux STM32 est opérationnelle avec les modulations supportées.

### Schéma de principe

```text
┌──────────────────────────────┐      ┌────────────────────────────────┐
│  Carte STM32WL (Émetteur)    │      │  Carte STM32WL (Flipper Zero)   │
│        Carte Test            │<────>│                                │
│                              │      │                                │
│  ComPort <────────────────── │      │ ───────────────────> ComPort   │
└──────────────────────────────┘      └────────────────────────────────┘
```

### Tentative de hacking d’une sonnette 433 MHz

Dans un second temps, nous avons cherché à hacker une sonnette sans fil fonctionnant à 433 MHz, dans l’objectif de reproduire un cas d’attaque similaire à ceux réalisables avec le Flipper Zero. Bien que la fréquence utilisée par la sonnette soit identique à celle exploitée par la carte STM32, aucun signal exploitable n’a pu être reçu.

Cette limitation s’explique par une différence de modulation. La sonnette utilise une modulation de type **ASK/OOK**, tandis que le récepteur Sub-GHz intégré à la STM32 ne prend en charge que les modulations **LoRa** et **FSK**. Ainsi, même avec une fréquence porteuse identique, la modulation incompatible empêche toute réception ou décodage du signal.


### Limites de l’approche STM32

Cette expérimentation met en évidence une limite importante de notre approche : la fréquence seule ne suffit pas pour intercepter ou reproduire un signal radio. Le type de modulation joue un rôle déterminant dans la compatibilité des systèmes.

Contrairement au Flipper Zero, qui supporte un large éventail de modulations grâce à son transceiver dédié, la carte STM32 utilisée reste limitée aux modulations prévues par son matériel.


### Conclusion à mi-durée du sous-projet STM32

En conclusion, nous avons réussi à reproduire partiellement le comportement du Flipper Zero en mettant en place une communication Sub-GHz fonctionnelle entre deux cartes STM32. En revanche, le hacking de la sonnette 433 MHz n’a pas été possible en raison des contraintes matérielles liées aux modulations supportées, ce qui souligne l’importance du choix du transceiver dans les attaques radio Sub-GHz.


