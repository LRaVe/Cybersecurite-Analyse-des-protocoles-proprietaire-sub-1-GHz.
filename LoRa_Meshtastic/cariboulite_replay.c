/**
 * cariboulite_replay.c
 * Lit un fichier .cs16 et le réémet via CaribouLite S1G
 *
 * Compilation :
 *   gcc cariboulite_replay.c -o cariboulite_replay \
 *       -I/usr/local/include -L/usr/local/lib \
 *       -Wl,-rpath,/usr/local/lib \
 *       -lcariboulite -lpthread -lm
 *
 * Usage :
 *   sudo ./cariboulite_replay -f 869525000 -r 4000000 capture.cs16
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include "cariboulite.h"
#include "cariboulite_radio.h"

#define BUFFER_SIZE 4096

static volatile int running = 1;

void sig_handler(int sig) {
    printf("\nSignal %d reçu, arrêt...\n", sig);
    running = 0;
}

void print_usage(const char *name) {
    printf("Usage: %s [options] fichier.cs16\n", name);
    printf("Options:\n");
    printf("  -f <freq_hz>    Fréquence TX en Hz (défaut: 869525000)\n");
    printf("  -r <rate>       Sample rate en sps (défaut: 4000000)\n");
    printf("  -l              Boucle infinie sur le fichier\n");
    printf("  -g <gain_db>    Gain TX en dB (défaut: 40)\n");
    printf("Exemple:\n");
    printf("  sudo %s -f 869525000 -r 4000000 capture.cs16\n", name);
}

int main(int argc, char *argv[]) {
    /* Paramètres par défaut */
    long freq      = 869525000;
    long rate      = 4000000;
    int  loop      = 0;
    int  gain      = 40;
    char *filename = NULL;

    /* Parse arguments */
    int opt;
    while ((opt = getopt(argc, argv, "f:r:g:lh")) != -1) {
        switch (opt) {
            case 'f': freq  = atol(optarg); break;
            case 'r': rate  = atol(optarg); break;
            case 'g': gain  = atoi(optarg); break;
            case 'l': loop  = 1;            break;
            case 'h':
            default:
                print_usage(argv[0]);
                return 0;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Erreur: fichier .cs16 requis\n");
        print_usage(argv[0]);
        return 1;
    }
    filename = argv[optind];

    printf("CaribouLite Replay\n");
    printf("Fichier    : %s\n", filename);
    printf("Fréquence  : %ld Hz\n", freq);
    printf("Sample rate: %ld sps\n", rate);
    printf("Gain TX    : %d dB\n", gain);
    printf("Boucle     : %s\n", loop ? "oui" : "non");
    printf("\n");

    /* Ouvrir le fichier */
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", filename);
        return 1;
    }

    /* Taille du fichier */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    long total_samples = file_size / 4; /* CS16 = 2x int16 = 4 bytes par sample */
    printf("Taille fichier : %ld bytes = %ld samples\n", file_size, total_samples);
    printf("Durée          : %.2f secondes\n", (double)total_samples / rate);

    /* Configurer les signaux */
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    /* FIX #2 : CARIBOULITE_CONFIG_DEFAULT n'existe pas dans cette version.
     *          cariboulite_init() n'a pas de paramètre de config ici.         */
    printf("\nInitialisation CaribouLite...\n");
    int ret = cariboulite_init(false, cariboulite_log_level_info);   /* false = ne pas forcer reprog FPGA */
    if (ret != 0) {
        fprintf(stderr, "Erreur: cariboulite_init() failed (%d)\n", ret);
        fclose(fp);
        return 1;
    }

    /* FIX #3 : cariboulite_get_radio_handle → cariboulite_get_radio */
    cariboulite_radio_state_st *radio = cariboulite_get_radio(cariboulite_channel_s1g);
    if (!radio) {
        fprintf(stderr, "Erreur: impossible d'obtenir le handle radio S1G\n");
        cariboulite_close();
        fclose(fp);
        return 1;
    }

    /* FIX #4 : cariboulite_radio_set_frequency attend un double* (pas double) */
    double freq_d = (double)freq;
    cariboulite_radio_set_frequency(radio, false, &freq_d);

    /* Vérification de la fréquence réellement accordée (optionnel) */
    double freq_actual = 0.0;
    cariboulite_radio_get_frequency(radio, false, NULL, &freq_actual);
    printf("Fréquence accordée : %.3f MHz\n", freq_actual / 1e6);

    cariboulite_radio_set_tx_bandwidth_flt(radio, (float)rate);
    cariboulite_radio_set_tx_samp_cutoff_flt(radio, (float)rate);
    cariboulite_radio_set_tx_power(radio, gain);

    /* Activer le canal TX */
    cariboulite_radio_activate_channel(radio, cariboulite_channel_dir_tx, true);

    printf("TX actif sur %.3f MHz\n", freq / 1e6);
    printf("Émission en cours... (Ctrl+C pour arrêter)\n\n");

    /* Buffer de samples */
    cariboulite_sample_complex_int16 *buffer =
        malloc(BUFFER_SIZE * sizeof(cariboulite_sample_complex_int16));
    if (!buffer) {
        fprintf(stderr, "Erreur: malloc\n");
        goto cleanup;
    }

    /* Boucle d'émission */
    do {
        fseek(fp, 0, SEEK_SET);
        size_t total_sent = 0;

        while (running) {
            /* Lire BUFFER_SIZE samples (4 bytes chacun : int16 I + int16 Q) */
            size_t n = fread(buffer, 4, BUFFER_SIZE, fp);
            if (n == 0) break; /* fin du fichier */

            int wret = cariboulite_radio_write_samples(radio, buffer, (int)n);
            if (wret < 0) {
                fprintf(stderr, "Erreur write_samples: %d\n", wret);
                running = 0;
                break;
            }
            total_sent += n;
        }

        printf("Émis: %zu samples (%.2f sec)\n",
               total_sent, (double)total_sent / rate);

        if (loop && running)
            printf("Reboucle...\n");

    } while (loop && running);

    printf("\nÉmission terminée.\n");
    free(buffer);

cleanup:
    /* Désactiver le TX */
    cariboulite_radio_activate_channel(radio, cariboulite_channel_dir_tx, false);

    /* FIX #5 : cariboulite_release_driver → cariboulite_close */
    cariboulite_close();
    fclose(fp);

    return 0;
}
