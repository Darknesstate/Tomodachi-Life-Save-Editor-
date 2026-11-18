/*
 * Tomodachi Life - Editor REAL v1 - Dinheiro
 * 1o Tomodachi, 2o PKMN, 3o Smash
 */
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SAVE_PATH "sdmc:/3ds/Checkpoint/saves/0x00EC7/Tomodachi Life/main"
#define BACKUP_PATH "sdmc:/3ds/TomodachiEditor/backup_main_v1.bin"
#define BACKUP_DIR "sdmc:/3ds/TomodachiEditor"

int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    printf("=== TOMODACHI EDITOR REAL v1 ===\n");
    printf("1o Tomodachi, 2o PKMN, 3o Smash\n");
    mkdir(BACKUP_DIR, 0777);
    FILE *src = fopen(SAVE_PATH, "rb");
    FILE *dst = fopen(BACKUP_PATH, "wb");
    if (src && dst) {
        u8 buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
            fwrite(buf, 1, n, dst);
        }
        printf("Backup feito!\n");
    }
    if (src) fclose(src);
    if (dst) fclose(dst);
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) {
        printf("Save nao achado. Faca backup no Checkpoint.\n");
        printf("Aperte START.\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 0;
    }
    fseek(f, 0, SEEK_END);
    u32 saveSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    u8 *saveData = malloc(saveSize);
    fread(saveData, 1, saveSize, f);
    fclose(f);
    printf("Save: %lu bytes\n", (unsigned long)saveSize);
    printf("CIMA/BAIXO: mudar valor\n");
    printf("A: colocar 999999\n");
    printf("START: salvar\n");
    int valor = 1000;
    int pos = -1;
    printf("Tentando: %d\n", valor);
    while (aptMainLoop()) {
        hidScanInput();
        u32 k = hidKeysDown();
        if (k & KEY_DUP) {
            valor += 100;
            printf("Tentando: %d\n", valor);
        }
        if (k & KEY_DDOWN) {
            if (valor >= 100) valor -= 100;
            printf("Tentando: %d\n", valor);
        }
        if ((k & KEY_DUP) || (k & KEY_DDOWN) || (k & KEY_A)) {
            pos = -1;
            for (u32 i = 0; i + 4 < saveSize; i++) {
                u32 v = saveData[i] | (saveData[i+1] << 8) | (saveData[i+2] << 16) | (saveData[i+3] << 24);
                if ((int)v == valor) {
                    pos = i;
                    printf("Achei %d no offset %lu\n", valor, (unsigned long)i);
                    break;
                }
            }
            if (pos < 0) {
                printf("Nao achei %d\n", valor);
            }
        }
        if (k & KEY_A) {
            if (pos >= 0) {
                u32 novo = 999999;
                saveData[pos] = novo & 0xFF;
                saveData[pos+1] = (novo >> 8) & 0xFF;
                saveData[pos+2] = (novo >> 16) & 0xFF;
                saveData[pos+3] = (novo >> 24) & 0xFF;
                printf("Trocado para 999999! Aperte START.\n");
            }
        }
        if (k & KEY_START) {
            FILE *out = fopen(SAVE_PATH, "wb");
            fwrite(saveData, 1, saveSize, out);
            fclose(out);
            printf("Salvo! Abra o Tomodachi.\n");
            break;
        }
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    free(saveData);
    gfxExit();
    return 0;
}
