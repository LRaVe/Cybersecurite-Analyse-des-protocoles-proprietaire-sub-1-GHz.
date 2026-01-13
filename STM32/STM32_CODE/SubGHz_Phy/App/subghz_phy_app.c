/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    subghz_phy_app.c
  * @author  MCD Application Team
  * @brief   Application of the SubGHz_Phy Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "radio.h"

/* USER CODE BEGIN Includes */
#include "stm32_timer.h"
#include "stm32_seq.h"
#include "utilities_def.h"
#include "app_version.h"
#include "subghz_phy_version.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  RX,
  RX_TIMEOUT,
  RX_ERROR,
  TX,
  TX_TIMEOUT,
} States_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Configurations */
/*Timeout*/
#define RX_TIMEOUT_VALUE              3000
#define TX_TIMEOUT_VALUE              3000
/* PING string*/
#define PING "Signal Envoyé"
/* PONG string*/
#define PONG "Signal reçu ! "
/*Size of the payload to be sent*/
/* Size must be greater of equal the PING and PONG*/
#define MAX_APP_BUFFER_SIZE          255
#if (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE)
#error PAYLOAD_LEN must be less or equal than MAX_APP_BUFFER_SIZE
#endif /* (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE) */
/* wait for remote to be in Rx, before sending a Tx frame*/
#define RX_TIME_MARGIN                200
/* Afc bandwidth in Hz */
#define FSK_AFC_BANDWIDTH             83333
/* LED blink Period*/
#define LED_PERIOD_MS                 200

#define MAX_TX_BUFFER 64



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Radio events function pointer */
static RadioEvents_t RadioEvents;

/* USER CODE BEGIN PV */



/*Ping Pong FSM states */
static States_t State = RX;
/* App Rx Buffer*/
static uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
/* App Tx Buffer*/
static uint8_t BufferTx[MAX_APP_BUFFER_SIZE];
/* Last  Received Buffer Size*/
uint16_t RxBufferSize = 0;
/* Last  Received packer Rssi*/
int8_t RssiValue = 0;
/* Last  Received packer SNR (in Lora modulation)*/
int8_t SnrValue = 0;
/* Led Timers objects*/
static UTIL_TIMER_Object_t timerLed;
/* device state. Master: true, Slave: false*/
bool isMaster = true;
/* random delay to make sure 2 devices will sync*/
/* the closest the random delays are, the longer it will
   take for the devices to sync when started simultaneously*/
static int32_t random_delay;


/* Variables globales pour le recopiage du signal */
uint8_t LastRxBuffer[256];  // Stocke le dernier signal reçu
uint16_t LastRxSize = 0;    // Taille du dernier signal
bool SignalDisponible = false; // Indique si un signal est prêt à être renvoyé

static uint8_t LastTxBuffer[MAX_TX_BUFFER];   // dernier message envoyé
static uint8_t LastTxSize = 0;               // taille du dernier message
static uint8_t TxBufferSize = 0;             // taille du message courant à envoyer
static bool VerificationEnCours = false;     // flag pour vérification

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Function to be executed on Radio Tx Done event
 */
static void OnTxDone(void);

/**
  * @brief Function to be executed on Radio Rx Done event
  * @param  payload ptr of buffer received
  * @param  size buffer size
  * @param  rssi
  * @param  LoraSnr_FskCfo
  */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo);

/**
  * @brief Function executed on Radio Tx Timeout event
  */
static void OnTxTimeout(void);

/**
  * @brief Function executed on Radio Rx Timeout event
  */
static void OnRxTimeout(void);

/**
  * @brief Function executed on Radio Rx Error event
  */
static void OnRxError(void);

/* USER CODE BEGIN PFP */
/**
  * @brief  Function executed on when led timer elapses
  * @param  context ptr of LED context
  */
static void OnledEvent(void *context);

/**
  * @brief PingPong state machine implementation
  */
static void PingPong_Process(void);

/**
  * @brief PingPong TX configure and process
  */
static void RadioSend(void);

/**
  * @brief PingPong RX configure and process
  */
static void RadioRx(void);

void ReenvoyerDernierSignal(void);

void SendTestSignal(void);

#define LORA_FIX_LENGTH_PAYLOAD_ON   false


/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  /* USER CODE BEGIN SubghzApp_Init_1 */

  APP_LOG(TS_OFF, VLEVEL_M, "\n\rPING PONG\n\r");
  /* Get SubGHY_Phy APP version*/
  APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:    V%X.%X.%X\r\n",
          (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  /* Led Timers*/
  UTIL_TIMER_Create(&timerLed, LED_PERIOD_MS, UTIL_TIMER_ONESHOT, OnledEvent, NULL);
  UTIL_TIMER_Start(&timerLed);
  /* USER CODE END SubghzApp_Init_1 */

  /* Radio initialization */
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);

  /* USER CODE BEGIN SubghzApp_Init_2 */
  /*calculate random delay for synchronization*/
  random_delay = (Radio.Random()) >> 22; /*10bits random e.g. from 0 to 1023 ms*/

  /* Radio Set frequency */
  Radio.SetChannel(RF_FREQUENCY);

  /* Radio configuration */
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
  APP_LOG(TS_OFF, VLEVEL_M, "---------------\n\r");
  APP_LOG(TS_OFF, VLEVEL_M, "LORA_MODULATION\n\r");
  APP_LOG(TS_OFF, VLEVEL_M, "LORA_BW=%d kHz\n\r", (1 << LORA_BANDWIDTH) * 125);
  APP_LOG(TS_OFF, VLEVEL_M, "LORA_SF=%d\n\r", LORA_SPREADING_FACTOR);
#elif ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  APP_LOG(TS_OFF, VLEVEL_M, "---------------\n\r");
  APP_LOG(TS_OFF, VLEVEL_M, "FSK_MODULATION\n\r");
  APP_LOG(TS_OFF, VLEVEL_M, "FSK_BW=%d Hz\n\r", FSK_BANDWIDTH);
  APP_LOG(TS_OFF, VLEVEL_M, "FSK_DR=%d bits/s\n\r", FSK_DATARATE);
#else
#error "Please define a modulation in the subghz_phy_app.h file."
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */

  /*fills tx buffer*/
  memset(BufferTx, 0x0, MAX_APP_BUFFER_SIZE);

  APP_LOG(TS_ON, VLEVEL_L, "rand=%d\n\r", random_delay);

  /*register task to to be run in while(1) after Radio IT*/
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), UTIL_SEQ_RFU, PingPong_Process);

  /*starts first process */
  HAL_Delay(RX_TIMEOUT_VALUE + random_delay);
  RadioRx();
  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone */
  APP_LOG(TS_ON, VLEVEL_L, "OnTxDone\n\r");
  /* Update the State of the FSM*/
  State = TX;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnTxDone */
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
{
  /* USER CODE BEGIN OnRxDone */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxDone\n\r");
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, SnrValue=%ddB\n\r", rssi, LoraSnr_FskCfo);
  /* Record payload Signal to noise ratio in Lora*/
  SnrValue = LoraSnr_FskCfo;
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
#if ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, Cfo=%dkHz\n\r", rssi, LoraSnr_FskCfo);
  SnrValue = 0; /*not applicable in GFSK*/
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
  /* Update the State of the FSM*/
  State = RX;
  /* Clear BufferRx*/
  memset(BufferRx, 0, MAX_APP_BUFFER_SIZE);
  /* Record payload size*/
  RxBufferSize = size;
  if (RxBufferSize <= MAX_APP_BUFFER_SIZE)
  {
    memcpy(BufferRx, payload, RxBufferSize);
  }
  /* Record Received Signal Strength*/
  RssiValue = rssi;
  /* Record payload content*/
  APP_LOG(TS_ON, VLEVEL_H, "payload. size=%d \n\r", size);
  for (int32_t i = 0; i < PAYLOAD_LEN; i++)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "%02X", BufferRx[i]);
    if (i % 16 == 15)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "\n\r");
    }
  }
  APP_LOG(TS_OFF, VLEVEL_H, "\n\r");
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxDone */
}

static void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout */
  APP_LOG(TS_ON, VLEVEL_L, "OnTxTimeout\n\r");
  /* Update the State of the FSM*/
  State = TX_TIMEOUT;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnTxTimeout */
}

static void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxTimeout\n\r");
  /* Update the State of the FSM*/
  State = RX_TIMEOUT;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxTimeout */
}

static void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxError\n\r");
  /* Update the State of the FSM*/
  State = RX_ERROR;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxError */
}

/* USER CODE BEGIN PrFD */
static void RadioSend(void)
{
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
  Radio.Sleep();
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
  Radio.SetMaxPayloadLength(MODEM_LORA, MAX_APP_BUFFER_SIZE);
#elif ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  Radio.SetTxConfig(MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                    FSK_DATARATE, 0,
                    FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, 0, TX_TIMEOUT_VALUE);


  Radio.SetMaxPayloadLength(MODEM_FSK, MAX_APP_BUFFER_SIZE);
#else
#error "Please define a modulation in the subghz_phy_app.h file."
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */

  Radio.Send(BufferTx, PAYLOAD_LEN);
}

static void RadioRx(void)
{
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
  Radio.Sleep();
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  if (LORA_FIX_LENGTH_PAYLOAD_ON == true)
  {
    Radio.SetMaxPayloadLength(MODEM_LORA, PAYLOAD_LEN);
  }
  else
  {
    Radio.SetMaxPayloadLength(MODEM_LORA, MAX_APP_BUFFER_SIZE);
  }
#elif ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  Radio.SetRxConfig(MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                    0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                    0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                    0, 0, false, true);
  if (LORA_FIX_LENGTH_PAYLOAD_ON == true)
  {
    Radio.SetMaxPayloadLength(MODEM_FSK, PAYLOAD_LEN);
  }
  else
  {
    Radio.SetMaxPayloadLength(MODEM_FSK, MAX_APP_BUFFER_SIZE);
  }
#else
#error "Please define a modulation in the subghz_phy_app.h file."
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */

  Radio.Rx(RX_TIMEOUT_VALUE);
}

// Code original

//static void PingPong_Process(void)
//{
//  Radio.Sleep();
//
//  switch (State)
//  {
//    case RX:
//
//      if (isMaster == true)
//      {
//        if (RxBufferSize > 0)
//        {
//
//
//          if (strncmp((const char *)BufferRx, PONG, sizeof(PONG) - 1) == 0)
//          {
//            UTIL_TIMER_Stop(&timerLed);
//            /* switch off green led */
//            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); /* LED_GREEN */
//            /* master toggles red led */
//            HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); /* LED_RED */
//            /* Add delay between RX and TX */
//            HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);
//            /* master sends PING*/
//            APP_LOG(TS_ON, VLEVEL_L, "..."
//                    "Signal reçu ! "
//                    "\n\r");
//           APP_LOG(TS_ON, VLEVEL_L, "Master Tx start\n\r");
//            memcpy(BufferTx, PING, sizeof(PING) - 1);
//            RadioSend();
//
//          }
//          else if (strncmp((const char *)BufferRx, PING, sizeof(PING) - 1) == 0)
//          {
//            /* A master already exists then become a slave */
//            isMaster = false;
//            APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
//            RadioRx();
//          }
//          else /* valid reception but neither a PING or a PONG message */
//          {
//            /* Set device as master and start again */
//            isMaster = true;
//            APP_LOG(TS_ON, VLEVEL_L, "Master Rx start\n\r");
//            RadioRx();
//          }
//        }
//      }
//      else
//      {
//        if (RxBufferSize > 0)
//        {
//          if (strncmp((const char *)BufferRx, PING, sizeof(PING) - 1) == 0)
//          {
//            UTIL_TIMER_Stop(&timerLed);
//            /* switch off red led */
//            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
//            /* slave toggles green led */
//            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin); /* LED_GREEN */
//            /* Add delay between RX and TX */
//            HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);
//            /*slave sends PONG*/
//            APP_LOG(TS_ON, VLEVEL_L, "..."
//                    "Signal Envoyé ! "
//                    "\n\r");
//            APP_LOG(TS_ON, VLEVEL_L, "Slave  Tx start\n\r");
//            memcpy(BufferTx, PONG, sizeof(PONG) - 1);
//            RadioSend();
//          }
//          else /* valid reception but not a PING as expected */
//          {
//            /* Set device as master and start again */
//            isMaster = true;
//            APP_LOG(TS_ON, VLEVEL_L, "Master Rx start\n\r");
//            RadioRx();
//          }
//        }
//      }
//      break;
//    case TX:
//      APP_LOG(TS_ON, VLEVEL_L, "Rx start\n\r");
//      RadioRx();
//      break;
//    case RX_TIMEOUT:
//    case RX_ERROR:
//      if (isMaster == true)
//      {
//        /* Send the next PING frame */
//        /* Add delay between RX and TX*/
//        /* add random_delay to force sync between boards after some trials*/
//        HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN + random_delay);
//        APP_LOG(TS_ON, VLEVEL_L, "Master Tx start\n\r");
//        /* master sends PING*/
//        memcpy(BufferTx, PING, sizeof(PING) - 1);
//        RadioSend();
//      }
//      else
//      {
//        APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
//        RadioRx();
//      }
//      break;
//    case TX_TIMEOUT:
//      APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
//      RadioRx();
//      break;
//    default:
//      break;
//  }
//}


// ========= DEBUT CODE : Envoi et réception

//Code de la carte qui recopie le signal

static void PingPong_Process(void)
{
    Radio.Sleep();

    switch (State)
    {
        case RX:
            if (RxBufferSize > 0)
            {
                /* Affichage du signal reçu */
                APP_LOG(TS_ON, VLEVEL_L,
                        "Signal LoRa recu ! Les informations sont : Taille=%d RSSI=%d\n\r",
                        RxBufferSize, RssiValue);


                APP_LOG(TS_ON, VLEVEL_L,
                        "  first=%02X %02X %02X %02X\n\r",
                        BufferRx[0],
                        BufferRx[1],
                        BufferRx[2],
                        BufferRx[3]);

                /* --- Nouveau : affichage ASCII --- */
                /* Ajouter le caractère nul pour l'affichage */
                if (RxBufferSize < sizeof(BufferRx))  // sécurité pour ne pas dépasser le buffer
                {
                    BufferRx[RxBufferSize] = '\0';
                }
                else
                {
                    BufferRx[sizeof(BufferRx)-1] = '\0';
                }

                APP_LOG(TS_ON, VLEVEL_L,
                        "  Message ASCII : %s\n\r", BufferRx);
                /* Stocker le signal pour usage ultérieur */
                memcpy(LastRxBuffer, BufferRx, RxBufferSize);
                LastRxSize = RxBufferSize;
                SignalDisponible = true;  // Signal prêt à être renvoyé plus tard

                /* --- Ici tu peux déclencher l'action locale, ex: allumer LED/sonnette --- */
                HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); // exemple LED
            }
            break;

        case TX:
            APP_LOG(TS_ON, VLEVEL_L, "TX fini, retour RX\n\r");
            RadioRx();
            break;

        case RX_TIMEOUT:
        case RX_ERROR:
            RadioRx();
            break;

        case TX_TIMEOUT:
            APP_LOG(TS_ON, VLEVEL_L, "TX timeout\n\r");
            RadioRx();
            break;

        default:
            break;
    }
}

//Code de la carte qui envoie et reçoit le signal recopié

//static void PingPong_Process(void)
//{
//    Radio.Sleep();
//
//    switch (State)
//    {
//        case RX:
//            if (RxBufferSize > 0)
//            {
//            	/* Affichage de tout signal reçu (original ou recopié) */
//            	APP_LOG(TS_ON, VLEVEL_L,
//            	        "Signal recu ! \n\r");
//
//            	APP_LOG(TS_ON, VLEVEL_L,
//            	        "RX OK size=%d RSSI=%d first=%02X %02X %02X %02X\n\r",
//            	        RxBufferSize,
//            	        RssiValue,
//            	        BufferRx[0],
//            	        BufferRx[1],
//            	        BufferRx[2],
//            	        BufferRx[3]);
//
//            	/* Affichage ASCII du message reçu */
//            	/* Ajouter le caractère nul pour l'affichage */
//            	if (RxBufferSize < sizeof(BufferRx))  // sécurité pour ne pas dépasser le buffer
//            	{
//            	    BufferRx[RxBufferSize] = '\0';
//            	}
//            	else
//            	{
//            	    BufferRx[sizeof(BufferRx)-1] = '\0';
//            	}
//
//            	APP_LOG(TS_ON, VLEVEL_L,
//            	        "  Message ASCII : %s\n\r", BufferRx);
//
//
//                /* Feedback visuel */
//                HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
//
//                /* Pas de stockage, pas de renvoi automatique */
//                RadioRx();
//            }
//            break;
//
//        case TX:
//            /* Si un TX a eu lieu (ex: test manuel), retour RX */
//            APP_LOG(TS_ON, VLEVEL_L, "TX termine, retour RX\n\r");
//            RadioRx();
//            break;
//
//        case RX_TIMEOUT:
//        case RX_ERROR:
//            /* Carte toujours en ecoute */
//            RadioRx();
//            break;
//
//        case TX_TIMEOUT:
//            RadioRx();
//            break;
//
//        default:
//            break;
//    }
//}

// ========= FIN CODE


/* Fonction pour envoyer le signal test */

void SendTestSignal(void)
{
    const char testMsg[] = "TEST_SIGNAL";

    memcpy(BufferTx, testMsg, sizeof(testMsg) - 1); // copie le message dans Buffer Tx
    TxBufferSize = sizeof(testMsg) - 1;

    memcpy(LastTxBuffer, BufferTx, TxBufferSize); //Sauvegarde le message
    LastTxSize = TxBufferSize;
    VerificationEnCours = true;

    APP_LOG(TS_ON, VLEVEL_L,
            "Envoi du signal de test\n\r");
    APP_LOG(TS_ON, VLEVEL_L,
            "  Taille=%d, premiers octets=%02X %02X %02X %02X\n\r",
            TxBufferSize,
            BufferTx[0],
            BufferTx[1],
            BufferTx[2],
            BufferTx[3]);

    APP_LOG(TS_ON, VLEVEL_L,
            "  Message ASCII : %s\n\r", BufferTx);

    RadioSend();
}

/* Fonction pour envoyer le dernier signal reçu */

void ReenvoyerDernierSignal(void)
{
    if (SignalDisponible && LastRxSize > 0)
    {
        memcpy(BufferTx, LastRxBuffer, LastRxSize); // Copie le signal dans BufferTx pour l'envoyer

        HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);

        APP_LOG(TS_ON, VLEVEL_L, "Je renvoie le signal :  Taille=%d\n\r", LastRxSize);

        RadioSend();

        SignalDisponible = false; // Le signal est envoyé, il n'y en a plus de disponible

    }
    else
    {
        APP_LOG(TS_ON, VLEVEL_L, "Aucun signal à renvoyer ... \n\r");
    }
}



static void OnledEvent(void *context)
{
  HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin); /* LED_GREEN */
  HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); /* LED_RED */
  UTIL_TIMER_Start(&timerLed);
}

/* USER CODE END PrFD */
