/*
 * Tomodachi Life - Editor REAL v1 - Dinheiro de verdade
 * 1o Tomodachi, 2o PKMN, 3o Smash
 */

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SAVE_PATH "sdmc:/3ds/Checkpoint/saves/0x00EC7/Tomodachi Life/main"
#define BACKUP_PATH "sdmc:/3ds/TomodachiEditor/backup_main_v1.bin"
#define BACKUP_DIR "sdmc:/3ds/TomodachiEditor"

int dinheiroAtual = -1;
int dinheiroEncontradoEm = -1;
u8 *saveData = NULL;
u32 saveSize = 0;

int fazerBackup() {
    mkdir(BACKUP_DIR, 0777);
    FILE *src = fopen(SAVE_PATH, "rb");
    FILE *dst = fopen(BACKUP_PATH, "wb");
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        printf("ERRO: Nao achei o save.\n");
        printf("Faca backup no Checkpoint primeiro.\n");
        return 0;
    }
    u8 buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, n, dst);
    fclose(src); fclose(dst);
    printf("Backup feito em SD:/3ds/TomodachiEditor/\n");
    return 1;
}

int carregarSave() {
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    saveSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    saveData = malloc(saveSize);
    fread(saveData, 1, saveSize, f);
    fclose(f);
    printf("Save carregado: %lu bytes\n", (unsigned long)saveSize);
    return 1;
}

void salvarSave() {
    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f) { printf("ERRO ao salvar!\n"); return; }
    fwrite(saveData, 1, saveSize, f);
    fclose(f);
    printf("Save salvo com sucesso!\n");
}

void procurarDinheiro(int valorProcurado) {
    dinheiroEncontradoEm = -1;
    for (u32 i = 0; i + 4 < saveSize; i++) {
        u32 v = saveData[i] | (saveData[i+1] << 8) | (saveData[i+2] << 16) | (saveData[i+3] << 24);
        if ((int)v == valorProcurado) {
            dinheiroEncontradoEm = i;
            dinheiroAtual = v;
            printf("Achei dinheiro %d no offset 0x%X\n", v, i);
            return;
        }
    }
    printf("Nao achei o valor %d no save.\n", valorProcurado);
}

void colocarDinheiro999999() {
    if (dinheiroEncontradoEm < 0) {
        printf("Primeiro precisa achar o dinheiro.\n");
        return;
    }
    u32 novo = 999999;
    saveData[dinheiroEncontradoEm] = novo & 0xFF;
    saveData[dinheiroEncontradoEm+1] = (novo >> 8) & 0xFF;
    saveData[dinheiroEncontradoEm+2] = (novo >> 16) & 0xFF;
    saveData[dinheiroEncontradoEm+3] = (novo >> 24) & 0xFF;
    printf("Dinheiro trocado para 999999 no offset 0x%X\n", dinheiroEncontradoEm);
    printf("Aperte START para salvar.\n");
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    printf("=== TOMODACHI EDITOR REAL v1 ===\n");
    printf("1o da fila: Tomodachi, depois PKMN, depois Smash\n");
    printf("--------------------------------\n");
    if (!fazerBackup()) {
        printf("\nAperte START para sair.\n");
        while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; gfxSwapBuffers(); gspWaitForVBlank(); }
        gfxExit(); return 0;
    }
    if (!carregarSave()) {
        printf("Save nao achado.\n");
        while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; gfxSwapBuffers(); gspWaitForVBlank(); }
        gfxExit(); return 0;
