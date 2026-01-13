/**
  @page SubGHz_Phy_SignalRelay Readme file

  *******************************************************************************
  * @file    Applications/SubGHz_Phy/SubGHz_Phy_SignalRelay/readme.txt
  * @brief   Application SubGHz PHY – Transmission et recopie de signal LoRa
  *******************************************************************************
  *
  * Cette application a été développée dans le cadre d’un projet et est basée
  * sur l’exemple SubGHz_Phy_PingPong fourni par STMicroelectronics.
  * Elle a été adaptée afin de mettre en œuvre une communication LoRa simple
  * avec mémorisation et recopie du signal entre deux cartes STM32WL.
  * L’objectif du projet étant de reproduire le comportement du Flipper Zero.
  *
  *******************************************************************************

  @par Description

  Cette application démontre une communication LoRa entre deux
  cartes STM32WL utilisant le middleware SubGHz PHY.

  Deux rôles sont définis :

  - Carte émettrice (carte de test) :
    Envoie un message LoRa de test ("TEST_SIGNAL") lors de l’appui sur le bouton U2 et reçoit le signal recopié par la carte Flipper avec le bouton U2.

  - Carte Flipper :
    Écoute le canal LoRa à l'appui du bouton U1, affiche les informations du message
    reçu via l’UART, mémorise le dernier message reçu
    et le renvoie lors de l’appui sur le bouton U2.

  *******************************************************************************

  @par Fonctionnalités principales

  - Communication LoRa RX / TX via SubGHz PHY
  - Affichage UART des messages reçus (taille, RSSI, données)
  - Mémorisation du dernier message LoRa reçu
  - Recopie et retransmission manuelle du message reçu
  - Interaction utilisateur via boutons poussoirs
  - Indication visuelle par LEDs

  *******************************************************************************

  @par Sélection du rôle de la carte

  Le rôle de la carte est défini dans la boucle principale du programme.
  Une seule des fonctions suivantes doit être activée :

    - ReenvoyerDernierSignal() : carte Flipper Zero
    - SendTestSignal()        : carte émettrice (test)

  Une seule fonction doit être active à la fois.

  *******************************************************************************

  @par Interaction utilisateur

  - Bouton 1 :
    Active l'écoute du canal LoRa sur les deuc cartes.

  - Bouton 2 :
    Déclenche l’action principale selon le rôle de la carte :
      - Envoi du signal de test
      - Renvoi du dernier signal reçu

  *******************************************************************************

  @par Configuration UART

  - Débit : 115200 bauds
  - Bits de données : 8
  - Bit de stop : 1
  - Parité : aucune
  - Contrôle de flux : aucun

  *******************************************************************************

  @par Contenu du répertoire

  La structure du projet et les fichiers listés ci-dessous sont repris de
  l’exemple SubGHz_Phy_PingPong fourni par STMicroelectronics.
  Ils constituent la base logicielle du projet, sur laquelle les
  modifications spécifiques à la recopie du signal LoRa ont été apportées.

  - SubGHz_Phy_PingPong/Core/Inc/dma.h
    Prototypes des fonctions du fichier dma.c.

  - SubGHz_Phy_PingPong/Core/Inc/gpio.h
    Prototypes des fonctions du fichier gpio.c.

  - SubGHz_Phy_PingPong/Core/Inc/main.h
    Définitions communes de l’application.

  - SubGHz_Phy_PingPong/Core/Inc/platform.h
    Configuration générale des instances matérielles.

  - SubGHz_Phy_PingPong/Core/Inc/rtc.h
    Prototypes des fonctions du fichier rtc.c.

  - SubGHz_Phy_PingPong/Core/Inc/stm32wlxx_hal_conf.h
    Fichier de configuration de la HAL STM32WL.

  - SubGHz_Phy_PingPong/Core/Inc/stm32wlxx_it.h
    Déclarations des routines d’interruptions.

  - SubGHz_Phy_PingPong/Core/Inc/stm32wlxx_nucleo_conf.h
    Configuration spécifique aux cartes Nucleo STM32WL.

  - SubGHz_Phy_PingPong/Core/Inc/stm32_lpm_if.h
    Interface du gestionnaire de basse consommation.

  - SubGHz_Phy_PingPong/Core/Inc/subghz.h
    Prototypes liés au périphérique SubGHz.

  - SubGHz_Phy_PingPong/Core/Inc/sys_app.h
    Fonctions de gestion système.

  - SubGHz_Phy_PingPong/Core/Inc/sys_conf.h
    Configuration applicative (debug, traces, basse consommation).

  - SubGHz_Phy_PingPong/Core/Inc/sys_debug.h
    Configuration du débogage.

  - SubGHz_Phy_PingPong/Core/Inc/timer_if.h
    Interface du gestionnaire de temporisation.

  - SubGHz_Phy_PingPong/Core/Inc/usart.h
    Prototypes de configuration USART.

  - SubGHz_Phy_PingPong/Core/Inc/usart_if.h
    Interface du pilote UART.

  - SubGHz_Phy_PingPong/Core/Inc/utilities_conf.h
    Configuration des utilitaires.

  - SubGHz_Phy_PingPong/Core/Inc/utilities_def.h
    Définitions des utilitaires.

  - SubGHz_Phy_PingPong/SubGHz_Phy/App/app_subghz_phy.c
    Implémentation de l’application SubGHz PHY.

  - SubGHz_Phy_PingPong/SubGHz_Phy/App/subghz_phy_app.c
    Logique applicative principale (RX, TX, recopie). 
    C'est le seul dossié qui a été réadapté pour notre projet.


@par Environnement matériel et logiciel

  - Cette application fonctionne sur les cartes STM32WL55JC2 Nucleo.

  - Mise en place des cartes STM32WL Nucleo :
    - Connecter chaque carte Nucleo à un PC via un câble USB type A vers micro-B
      sur le connecteur ST-LINK.
    - Vérifier que les cavaliers (jumpers) du ST-LINK sont correctement positionnés.

  - Configuration logicielle :
    - Les paramètres de l’application et de la radio peuvent être configurés via
      les fichiers suivants :
        - sys_conf.h
        - radio_conf.h
        - mw_log_conf.h
        - main.h
        - etc.

  - Schéma de principe :

             ------------------------------      ------------------------------
             |   Carte STM32WL (Émetteur) |      | Carte STM32WL (Flipper Zero) |
             |      Carte Test            |<---->|                              |
   ComPort<--|                            |      |                              |-->ComPort
             ------------------------------      ------------------------------

*******************************************************************************

@par Comment utiliser l’application ?

  Pour faire fonctionner l’application, suivre les étapes suivantes :

  - Ouvrir le projet avec STM32CubeIDE (ou un outil compatible).
  - Compiler le projet et programmer la première carte STM32WL.
  - Configurer le rôle de la carte (test ou flipepr) dans le code.
  - Compiler et programmer la seconde carte STM32WL.
  - Réinitialiser les deux cartes.
  - Connecter un terminal série à chaque carte.

  Configuration UART :
  - Débit : 115200 bauds
  - Bits de données : 8
  - Bit de stop : 1
  - Parité : aucune
  - Contrôle de flux : aucun

*******************************************************************************

@par Débogage

  - Vérifier que le flag DEBUGGER_ENABLED est positionné à 1 dans le fichier
    sys_conf.h.
  - Il est recommandé de définir également le flag LOW_POWER_DISABLE à 1 afin
    de simplifier le débogage.
  - Compiler, programmer la carte, puis attacher le débogueur.

*******************************************************************************

@par Utilisation de STM32CubeMX pour modifier la configuration RF

  Cette application est compatible avec STM32CubeMX (avec certaines limitations).
  Les paramètres de l’application RF et du middleware SubGHz PHY peuvent être
  modifiés via l’interface graphique.

  Recommandations importantes :

  - Le fichier .ioc fourni dans le projet peut être ouvert avec STM32CubeMX
    version 6.7.0 ou supérieure.

  - Attention : lors de la régénération du code à partir du fichier .ioc,
    les chemins vers les fichiers HAL et Middleware peuvent être incorrectement
    définis.
    Pour éviter ce problème :
      - Décocher l’option "Use Default Firmware Location" dans l’onglet
        "Project Manager".
      - Renseigner manuellement le chemin vers le répertoire racine du package
        STM32CubeWL (exemple : C:\myDir\STM32Cube_FW_WL_V1.3.0\).

  - Le fichier .extSettings permet d’ajouter des fichiers supplémentaires
    non générés nativement par CubeMX (par exemple des fichiers BSP).

  - Lors d’une régénération sur un projet existant :
      - STM32CubeMX met à jour les fichiers générés.
      - Les sections USER CODE (entre /* USER CODE BEGIN */ et
        /* USER CODE END */) sont conservées.

  - Lors d’une régénération à partir du seul fichier .ioc dans un dossier vide :
      - STM32CubeMX génère un projet par défaut.
      - Il appartient à l’utilisateur d’ajouter manuellement le code applicatif
        dans les sections USER CODE.

  Ce projet SubGHz_Phy est basé sur le modèle "Advanced template" de STM32CubeMX.

*******************************************************************************

@par Utilisation avec Azure ThreadX RTOS

  Cette application peut être combinée avec Azure ThreadX RTOS via STM32CubeMX.
  Cependant, le portage complet de l’exemple nécessite des modifications
  manuelles.

  Après génération du projet avec ThreadX, l’utilisateur doit modifier le
  fichier subghz_phy_app.c comme suit :

  - Supprimer l’inclusion du fichier "stm32_seq.h".
  - Supprimer l’enregistrement de la tâche :
      UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process),
                       UTIL_SEQ_RFU,
                       PingPong_Process);
  - Appeler directement la fonction PingPong_Process() dans la boucle principale
    du thread applicatif :
      /* USER CODE App_Main_Thread_Entry_Loop */
  - Remplacer les appels à UTIL_SEQ_SetTask(..) par :
      tx_thread_resume(&App_MainThread);

