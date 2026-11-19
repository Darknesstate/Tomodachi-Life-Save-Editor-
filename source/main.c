#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_SAVES 6
char caminhos[MAX_SAVES][256];
char nomes[MAX_SAVES][32];
int total = 0;
int selecionado = 0;
int dinheiroAtual = 1000;
int dinheiroNovo = 999999;
int editandoAtual = 1;
int precisaRedesenhar = 1;
char msg[64] = "";

int arquivoExiste(const char *p) {
    FILE *f = fopen(p, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

void adicionarSave(const char *caminho, const char *nomeCurto) {
    if (total >= MAX_SAVES) return;
    for (int i = 0; i < total; i++) {
        if (strcmp(caminhos[i], caminho) == 0) return;
    }
    strcpy(caminhos[total], caminho);
    strcpy(nomes[total], nomeCurto);
    total++;
}

void buscarSaves() {
    total = 0;
    DIR *d = opendir("sdmc:/Checkpoint/saves");
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (strstr(e->d_name, "Tomodachi") || strstr(e->d_name, "008C3")) {
                char base[256];
                snprintf(base, sizeof(base), "sdmc:/Checkpoint/saves/%s", e->d_name);
                char p1[256], p2[256];
                snprintf(p1, sizeof(p1), "%s/savedataArc", base);
                snprintf(p2, sizeof(p2), "%s/savedataArc.txt", base);
                if (arquivoExiste(p1)) adicionarSave(p1, "Save 1 - Principal");
                else if (arquivoExiste(p2)) adicionarSave(p2, "Save 1 - Principal");
            }
        }
        closedir(d);
    }
    if (arquivoExiste("sdmc:/Checkpoint/saves/0x008C3 Tomodachi Life/savedataArc"))
        adicionarSave("sdmc:/Checkpoint/saves/0x008C3 Tomodachi Life/savedataArc", "Save 1 - Principal");
    if (arquivoExiste("sdmc:/Checkpoint/saves/0x008C3 Tomodachi Life/savedataArc.txt"))
        adicionarSave("sdmc:/Checkpoint/saves/0x008C3 Tomodachi Life/savedataArc.txt", "Save 1 - Principal");
}

void fazerBackup() {
    if (total == 0) return;
    mkdir("sdmc:/TomodachiBackup", 0777);
    FILE *src = fopen(caminhos[selecionado], "rb");
    if (!src) { strcpy(msg, "Erro no backup!"); return; }
    char dstPath[256];
    snprintf(dstPath, sizeof(dstPath), "sdmc:/TomodachiBackup/bkp_%d.bin", selecionado + 1);
    FILE *dst = fopen(dstPath, "wb");
    if (!dst) { fclose(src); strcpy(msg, "Erro ao criar backup!"); return; }
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, n, dst);
    fclose(src);
    fclose(dst);
    strcpy(msg, "Backup feito!");
}

int procurarETrocar() {
    if (total == 0) return -2;
    if (dinheiroAtual == 0) { strcpy(msg, "Atual nao pode ser 0!"); return -1; }
    if (dinheiroAtual == dinheiroNovo) { strcpy(msg, "Atual = Novo!"); return -1; }

    FILE *f = fopen(caminhos[selecionado], "rb");
    if (!f) { strcpy(msg, "Erro ao abrir save!"); return -1; }
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);
    u8 *buf = malloc(tamanho);
    if (!buf) { fclose(f); strcpy(msg, "Sem memoria!"); return -1; }
    fread(buf, 1, tamanho, f);
    fclose(f);

    int trocas = 0;
    for (long i = 0; i <= tamanho - 4; i++) {
        u32 v = 0;
        memcpy(&v, buf + i, 4);
        if (v == (u32)dinheiroAtual) {
            u32 novo = (u32)dinheiroNovo;
            memcpy(buf + i, &novo, 4);
            trocas++;
        }
    }

    if (trocas > 0) {
        f = fopen(caminhos[selecionado], "wb");
        fwrite(buf, 1, tamanho, f);
        fclose(f);
        snprintf(msg, sizeof(msg), "Achou %d lugar(es), trocou!", trocas);
    } else {
        strcpy(msg, "Achou 0 lugares!");
    }
    free(buf);
    return trocas;
}

void desenhar() {
    consoleClear();
    printf("=== Tomodachi Life Save Editor ===\n");
    printf("by Darkness Metal - V2.2\n");
    printf("--------------------------------\n");
    if (total == 0) {
        printf("Nenhum save achado.\n");
        printf("Faca backup no Checkpoint\n");
        printf("antes de abrir aqui.\n");
        printf("\nSTART: sair\n");
        return;
    }
    printf("Save: %s\n", nomes[selecionado]);
    printf("--------------------------------\n");
    printf("%s Atual: %d\n", editandoAtual ? ">" : " ", dinheiroAtual);
    printf("%s Novo: %d\n", !editandoAtual ? ">" : " ", dinheiroNovo);
    printf("--------------------------------\n");
    printf("SELECT: trocar Atual / Novo\n");
    printf("Esq/Dir: -1000 / +1000\n");
    printf("Cima/Baixo: trocar save\n");
    printf("Y: Novo = 999999\n");
    printf("A: aplicar  X: backup\n");
    printf("START: sair\n");
    printf("\n%s\n", msg);
}

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    buscarSaves();
    desenhar();
    precisaRedesenhar = 0;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        if (total > 0) {
            if (kDown & KEY_UP) {
                selecionado--;
                if (selecionado < 0) selecionado = total - 1;
                strcpy(msg, "");
                precisaRedesenhar = 1;
            }
            if (kDown & KEY_DOWN) {
                selecionado++;
                if (selecionado >= total) selecionado = 0;
                strcpy(msg, "");
                precisaRedesenhar = 1;
            }
            if (kDown & KEY_SELECT) {
                editandoAtual = !editandoAtual;
                precisaRedesenhar = 1;
            }
            if (kDown & KEY_LEFT) {
                if (editandoAtual) { dinheiroAtual -= 1000; if (dinheiroAtual < 0) dinheiroAtual = 0; }
                else { dinheiroNovo -= 1000; if (dinheiroNovo < 0) dinheiroNovo = 0; }
                precisaRedesenhar = 1;
            }
            if (kDown & KEY_RIGHT) {
                if (editandoAtual) dinheiroAtual += 1000;
                else dinheiroNovo += 1000;
                precisaRedesenhar = 1;
            }
            if (kDown & KEY_Y) {
                dinheiroNovo = 999999;
                precisaRedesenhar = 1;
            }
            if (kDown & KEY_A) {
                procurarETrocar();
                precisaRedesenhar = 1;
            }
            if (kDown & KEY_X) {
                fazerBackup();
                precisaRedesenhar = 1;
            }
        }

        if (precisaRedesenhar) {
            desenhar();
            precisaRedesenhar = 0;
        }
        gfxFlushBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
