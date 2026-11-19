#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_SAVES 20

typedef struct {
    char caminho[600];
    char nome[128];
} SaveInfo;

SaveInfo saves[MAX_SAVES];
int totalSaves = 0;
int saveSelecionado = 0;
int dinheiroAtual = 0;
int dinheiroNovo = 999999;
char mensagem[256] = "";

bool arquivoExiste(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

void adicionarSave(const char *caminho, const char *nome) {
    if (totalSaves >= MAX_SAVES) return;
    for (int i = 0; i < totalSaves; i++) {
        if (strcmp(saves[i].caminho, caminho) == 0) return;
    }
    strncpy(saves[totalSaves].caminho, caminho, 599);
    saves[totalSaves].caminho[599] = '\0';
    strncpy(saves[totalSaves].nome, nome, 127);
    saves[totalSaves].nome[127] = '\0';
    totalSaves++;
}

void buscarSavesCorrigido() {
    totalSaves = 0;
    const char *raizes[] = {
        "sdmc:/3ds/Checkpoint/saves",
        "sdmc:/Checkpoint/saves"
    };

    for (int r = 0; r < 2; r++) {
        DIR *d = opendir(raizes[r]);
        if (!d) continue;

        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') continue;
            if (strstr(e->d_name, "Tomodachi") == NULL && strstr(e->d_name, "008C3") == NULL) continue;

            char pastaJogo[512];
            snprintf(pastaJogo, sizeof(pastaJogo), "%s/%s", raizes[r], e->d_name);

            DIR *d2 = opendir(pastaJogo);
            if (!d2) continue;

            struct dirent *b;
            while ((b = readdir(d2)) != NULL) {
                if (b->d_name[0] == '.') continue;

                char pastaBackup[512];
                snprintf(pastaBackup, sizeof(pastaBackup), "%s/%s", pastaJogo, b->d_name);

                char p1[600], p2[600];
                snprintf(p1, sizeof(p1), "%s/savedataArc", pastaBackup);
                snprintf(p2, sizeof(p2), "%s/savedataArc.txt", pastaBackup);

                if (arquivoExiste(p1)) {
                    adicionarSave(p1, b->d_name);
                } else if (arquivoExiste(p2)) {
                    adicionarSave(p2, b->d_name);
                }
            }
            closedir(d2);
        }
        closedir(d);
    }
}

bool fazerBackup(const char *caminho) {
    char destino[620];
    snprintf(destino, sizeof(destino), "%s.bak", caminho);
    FILE *orig = fopen(caminho, "rb");
    if (!orig) return false;
    FILE *copia = fopen(destino, "wb");
    if (!copia) { fclose(orig); return false; }
    unsigned char buf[4096];
    size_t lidos;
    while ((lidos = fread(buf, 1, sizeof(buf), orig)) > 0) {
        fwrite(buf, 1, lidos, copia);
    }
    fclose(orig);
    fclose(copia);
    return true;
}

int aplicarDinheiro(const char *caminho, int atual, int novo) {
    FILE *f = fopen(caminho, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (tamanho <= 0) { fclose(f); return -1; }

    unsigned char *dados = (unsigned char*)malloc(tamanho);
    if (!dados) { fclose(f); return -1; }
    fread(dados, 1, tamanho, f);
    fclose(f);

    int achados = 0;
    unsigned int vAtual = (unsigned int)atual;
    unsigned int vNovo = (unsigned int)novo;

    for (long i = 0; i + 4 <= tamanho; i++) {
        unsigned int v;
        memcpy(&v, dados + i, 4);
        if (v == vAtual) {
            memcpy(dados + i, &vNovo, 4);
            achados++;
        }
    }

    if (achados > 0) {
        FILE *out = fopen(caminho, "wb");
        if (!out) { free(dados); return -1; }
        fwrite(dados, 1, tamanho, out);
        fclose(out);
    }

    free(dados);
    return achados;
}

int main(int argc, char **argv) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    buscarSavesCorrigido();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;

        if (totalSaves > 0) {
            if (kDown & KEY_UP) {
                saveSelecionado--;
                if (saveSelecionado < 0) saveSelecionado = totalSaves - 1;
            }
            if (kDown & KEY_DOWN) {
                saveSelecionado++;
                if (saveSelecionado >= totalSaves) saveSelecionado = 0;
            }
            if (kDown & KEY_LEFT) {
                dinheiroAtual -= 10;
                if (dinheiroAtual < 0) dinheiroAtual = 0;
            }
            if (kDown & KEY_RIGHT) {
                dinheiroAtual += 10;
            }
            if (kDown & KEY_L) {
                dinheiroAtual -= 1000;
                if (dinheiroAtual < 0) dinheiroAtual = 0;
            }
            if (kDown & KEY_R) {
                dinheiroAtual += 1000;
            }
            if (kDown & KEY_Y) {
                dinheiroNovo = 999999;
                snprintf(mensagem, sizeof(mensagem), "Novo = 999999");
            }
            if (kDown & KEY_X) {
                if (fazerBackup(saves[saveSelecionado].caminho)) {
                    snprintf(mensagem, sizeof(mensagem), "Backup feito! .bak criado");
                } else {
                    snprintf(mensagem, sizeof(mensagem), "Erro no backup!");
                }
            }
            if (kDown & KEY_A) {
                if (dinheiroAtual <= 0) {
                    snprintf(mensagem, sizeof(mensagem), "Coloca seu dinheiro atual primeiro!");
                } else {
                    int r = aplicarDinheiro(saves[saveSelecionado].caminho, dinheiroAtual, dinheiroNovo);
                    if (r > 0) {
                        snprintf(mensagem, sizeof(mensagem), "Achou %d lugar(es), trocou!", r);
                    } else if (r == 0) {
                        snprintf(mensagem, sizeof(mensagem), "Achou 0 lugares. Confere o valor Atual!");
                    } else {
                        snprintf(mensagem, sizeof(mensagem), "Erro ao abrir o save!");
                    }
                }
            }
        }

        consoleClear();
        printf("Tomodachi Life - Save Editor\n");
        printf("by Darkness Metal - V3\n");
        printf("--------------------------------\n");

        if (totalSaves == 0) {
            printf("\nNenhum save achado/checkpoint\n");
            printf("Faca backup no Checkpoint\n");
            printf("antes de abrir aqui.\n");
            printf("\nSTART: sair\n");
        } else {
            printf("Saves encontrados: %d\n\n", totalSaves);
            for (int i = 0; i < totalSaves; i++) {
                if (i == saveSelecionado) printf("> %s\n", saves[i].nome);
                else printf("  %s\n", saves[i].nome);
            }
            printf("\nAtual: %d\n", dinheiroAtual);
            printf("Novo: %d\n", dinheiroNovo);
            printf("\n%s\n", mensagem);
            printf("\nCima/Baixo: trocar save\n");
            printf("Esq/Dir: Atual +-10  L/R: +-1000\n");
            printf("Y: Novo 999999  X: Backup\n");
            printf("A: Aplicar  START: sair\n");
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
