#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_SAVES 8
#define MAX_PATH 512
#define MONEY_OFFSET 0x1E4BB8
#define MONEY_MAX 999999

static char g_saves[MAX_SAVES][MAX_PATH];
static int g_saveCount = 0;
static u32 g_money = 0;
static long g_offset = MONEY_OFFSET;
static int g_found = 0;
static char g_msg[256] = "";

static int contains_ci(const char *s, const char *needle) {
    if (!s || !needle) return 0;
    size_t nl = strlen(needle);
    for (const char *p = s; *p; p++) {
        size_t i = 0;
        for (; i < nl; i++) {
            if (!p[i]) break;
            if (tolower((unsigned char)p[i]) != tolower((unsigned char)needle[i])) break;
        }
        if (i == nl) return 1;
    }
    return 0;
}

static int ends_with_ci(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t sl = strlen(s), su = strlen(suffix);
    if (su > sl) return 0;
    const char *e = s + sl - su;
    for (size_t i = 0; i < su; i++) {
        if (tolower((unsigned char)e[i]) != tolower((unsigned char)suffix[i])) return 0;
    }
    return 1;
}

static int is_save_name(const char *name) {
    if (!name) return 0;
    if (contains_ci(name, "savedataarc")) return 1;
    if (ends_with_ci(name, ".sav")) return 1;
    if (ends_with_ci(name, ".bin")) return 1;
    if (ends_with_ci(name, ".txt")) return 1;
    return 0;
}

static void add_save(const char *path) {
    if (g_saveCount >= MAX_SAVES) return;
    for (int i = 0; i < g_saveCount; i++) if (strcmp(g_saves[i], path) == 0) return;
    strncpy(g_saves[g_saveCount], path, MAX_PATH - 1);
    g_saves[g_saveCount][MAX_PATH - 1] = '\0';
    g_saveCount++;
}

static void scan_dir(const char *base, int depth) {
    if (depth > 6) return;
    DIR *d = opendir(base);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        char full[MAX_PATH];
        snprintf(full, sizeof(full), "%s/%s", base, e->d_name);
        struct stat st;
        if (stat(full, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            scan_dir(full, depth + 1);
        } else if (S_ISREG(st.st_mode)) {
            if (is_save_name(e->d_name)) {
                if (contains_ci(full, "tomodachi") || contains_ci(full, "0008c3") || contains_ci(full, "0008c4") || contains_ci(full, "TomodachiReal")) {
                    if (!contains_ci(e->d_name, ".bak") && !contains_ci(e->d_name, "backup")) {
                        add_save(full);
                    }
                }
            }
        }
    }
    closedir(d);
}

static int find_all_saves(void) {
    g_saveCount = 0;
    memset(g_saves, 0, sizeof(g_saves));
    scan_dir("sdmc:/3ds/Checkpoint/saves", 0);
    scan_dir("sdmc:/3ds/Checkpoint/extdata", 0);
    scan_dir("sdmc:/Nintendo 3DS", 0);
    scan_dir("sdmc:/3ds/TomodachiReal", 0);
    scan_dir("sdmc:/Tomodachi", 0);
    return g_saveCount;
}

static int read_money_from(const char *path, long offset, u32 *out) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < offset + 4) { fclose(f); return 0; }
    fseek(f, offset, SEEK_SET);
    u8 b[4] = {0,0,0,0};
    if (fread(b, 1, 4, f) != 4) { fclose(f); return 0; }
    fclose(f);
    u32 v = (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
    *out = v;
    return 1;
}

static int write_money_to(const char *path, long offset, u32 money) {
    FILE *f = fopen(path, "r+b");
    if (!f) return 0;
    fseek(f, offset, SEEK_SET);
    u8 b[4];
    b[0] = money & 0xFF;
    b[1] = (money >> 8) & 0xFF;
    b[2] = (money >> 16) & 0xFF;
    b[3] = (money >> 24) & 0xFF;
    int ok = (fwrite(b, 1, 4, f) == 4);
    fclose(f);
    return ok;
}

static void make_backup(const char *src) {
    char dst[MAX_PATH];
    snprintf(dst, sizeof(dst), "%s.bak", src);
    FILE *in = fopen(src, "rb");
    if (!in) return;
    FILE *out = fopen(dst, "wb");
    if (out) {
        u8 buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
        fclose(out);
    }
    fclose(in);
    mkdir("sdmc:/3ds/TomodachiReal", 0777);
    mkdir("sdmc:/3ds/TomodachiReal/backup", 0777);
    FILE *in2 = fopen(src, "rb");
    if (in2) {
        FILE *out2 = fopen("sdmc:/3ds/TomodachiReal/backup/savedataArc_backup.txt", "wb");
        if (out2) {
            u8 buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), in2)) > 0) fwrite(buf, 1, n, out2);
            fclose(out2);
        }
        fclose(in2);
    }
}

static void restore_backup(void) {
    for (int i = 0; i < g_saveCount; i++) {
        char bak[MAX_PATH];
        snprintf(bak, sizeof(bak), "%s.bak", g_saves[i]);
        FILE *in = fopen(bak, "rb");
        if (!in) continue;
        FILE *out = fopen(g_saves[i], "wb");
        if (out) {
            u8 buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
            fclose(out);
        }
        fclose(in);
    }
    if (g_saveCount > 0) read_money_from(g_saves[0], g_offset, &g_money);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    printf("=== Tomodachi Life Save Editor ===\n");
    printf("for 3DS by Darkness Metal - V2\n");
    printf("\nProcurando saves...\n");
    find_all_saves();
    if (g_saveCount > 0) {
        g_found = 1;
        for (int i = 0; i < g_saveCount; i++) make_backup(g_saves[i]);
        if (!read_money_from(g_saves[0], g_offset, &g_money)) g_money = 0;
        if (g_money > 99999999) {
            u32 alt = 0;
            if (read_money_from(g_saves[0], 0x1E4C00, &alt) && alt <= 99999999) {
                g_offset = 0x1E4C00;
                g_money = alt;
            }
        }
        snprintf(g_msg, sizeof(g_msg), "Backup auto feito! %d save(s)", g_saveCount);
    } else {
        g_found = 0;
        snprintf(g_msg, sizeof(g_msg), "Nenhum save achado.");
    }
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            if (g_found) {
                int ok = 0;
                for (int i = 0; i < g_saveCount; i++) {
                    if (write_money_to(g_saves[i], g_offset, g_money)) ok++;
                }
                consoleClear();
                printf("=== Tomodachi Life Save Editor ===\n");
                printf("for 3DS by Darkness Metal - V2\n\n");
                printf("Salvo em %d/%d!\n", ok, g_saveCount);
                printf("Se editou so o backup,\n");
                printf("abre o Checkpoint e faz Restore!\n\n");
                printf("Saindo...\n");
                gfxFlushBuffers();
                gfxSwapBuffers();
                gspWaitForVBlank();
                svcSleepThread(1500000000);
            }
            break;
        }
        if (kDown & KEY_SELECT) {
            find_all_saves();
            if (g_saveCount > 0) {
                g_found = 1;
                if (!read_money_from(g_saves[0], g_offset, &g_money)) g_money = 0;
                snprintf(g_msg, sizeof(g_msg), "Re-escaneado: %d save(s)", g_saveCount);
            } else {
                g_found = 0;
                snprintf(g_msg, sizeof(g_msg), "Nenhum save achado.");
            }
        }
        if (g_found) {
            if (kDown & KEY_A) { g_money = MONEY_MAX; snprintf(g_msg, sizeof(g_msg), "MAX 999999! Aperte START p/ salvar"); }
            if (kDown & KEY_B) { g_money = 0; snprintf(g_msg, sizeof(g_msg), "Zerado! Aperte START p/ salvar"); }
            if (kDown & KEY_UP) { if (g_money + 1000 <= 99999999) g_money += 1000; }
            if (kDown & KEY_DOWN) { if (g_money >= 1000) g_money -= 1000; else g_money = 0; }
            if (kDown & KEY_RIGHT) { if (g_money + 10000 <= 99999999) g_money += 10000; }
            if (kDown & KEY_LEFT) { if (g_money >= 10000) g_money -= 10000; else g_money = 0; }
            if (kDown & KEY_X) {
                for (int i = 0; i < g_saveCount; i++) make_backup(g_saves[i]);
                snprintf(g_msg, sizeof(g_msg), "Backup feito de novo!");
            }
            if (kDown & KEY_Y) { restore_backup(); snprintf(g_msg, sizeof(g_msg), "Backup restaurado!"); }
            if (kDown & KEY_L) {
                g_offset = (g_offset == MONEY_OFFSET) ? 0x1E4C00 : MONEY_OFFSET;
                u32 v = 0;
                if (read_money_from(g_saves[0], g_offset, &v)) g_money = v;
                snprintf(g_msg, sizeof(g_msg), "Offset: 0x%lX", (unsigned long)g_offset);
            }
        }
        consoleClear();
        printf("=== Tomodachi Life Save Editor ===\n");
        printf("for 3DS by Darkness Metal - V2\n");
        printf("--------------------------------\n");
        if (!g_found) {
            printf("Save nao achado!\n\n");
            printf("Eu procurei em:\n");
            printf(" - /3ds/Checkpoint/saves/\n");
            printf(" - /3ds/Checkpoint/extdata/\n");
            printf(" - /Nintendo 3DS/.../0008C300 US\n");
            printf(" - /Nintendo 3DS/.../0008C400 EU\n");
            printf(" - /3ds/TomodachiReal/\n\n");
            printf("Faca assim no Checkpoint:\n");
            printf("1. Abre Checkpoint\n");
            printf("2. Acha Tomodachi Life\n");
            printf("3. Aperta Backup\n");
            printf("4. Se for extdata, aperta X\n");
            printf("5. Volta aqui e aperta SELECT\n\n");
            printf("SELECT = procurar de novo\n");
            printf("START = sair\n");
        } else {
            printf("Saves achados: %d\n", g_saveCount);
            for (int i = 0; i < g_saveCount && i < 3; i++) {
                printf("%d: %s\n", i+1, g_saves[i]);
            }
            if (g_saveCount > 3) printf("... + %d mais\n", g_saveCount - 3);
            printf("\nOffset: 0x%lX\n", (unsigned long)g_offset);
            printf("Dinheiro: %lu\n", (unsigned long)g_money);
            printf("\nCIMA +1000  BAIXO -1000\n");
            printf("DIREITA +10000  ESQUERDA -10000\n");
            printf("A = MAX 999999  B = 0\n");
            printf("X = backup  Y = restaurar\n");
            printf("L = trocar offset\n");
            printf("SELECT = procurar de novo\n");
            printf("START = SALVAR EM TODOS + sair\n\n");
            printf("Se editou so o backup do\n");
            printf("Checkpoint, abre o Checkpoint\n");
            printf("e faz Restore p/ o jogo ver!\n");
        }
        printf("\n%s\n", g_msg);
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
