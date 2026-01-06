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

### Avancée Jan. 2026 - Application Fonctionnelle ✅

L'application est maintenant **complètement fonctionnelle** avec capture et replay de signaux radio.

#### 🏗️ Architecture de l'Application

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

#### 📡 APIs Flipper Zero Utilisées

| API | Fonction |
|-----|----------|
| `furi_hal_subghz_*` | Contrôle du module radio CC1101 (433.92 MHz) |
| `furi_hal_subghz_start_async_rx()` | Démarrage de la capture asynchrone |
| `furi_hal_subghz_tx()` / `furi_hal_subghz_idle()` | Émission/arrêt pour le replay |
| `furi_thread_*` | Threads pour capture/replay non-bloquants |
| `storage_*` / `stream_*` | Lecture/écriture fichiers .sub sur SD |
| `gui_*` / `canvas_*` | Interface utilisateur |

#### 🔧 Problèmes Rencontrés et Solutions

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

#### 📁 Format du Fichier .sub

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

#### ✅ Résultat Final

- ✅ Capture de signaux radio 433 MHz
- ✅ Filtrage intelligent du bruit
- ✅ Sauvegarde automatique après détection de silence
- ✅ Replay fonctionnel (testé avec succès sur une sonnette)
- ✅ Interface utilisateur réactive avec états CAPTURE → AFFICHAGE → REPLAY

