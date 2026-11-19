#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define COL_RESET  "\x1b[0m"
#define COL_RED    "\x1b[31;1m"
#define COL_GREEN  "\x1b[32;1m"
#define COL_YELLOW "\x1b[33;1m"
#define COL_MAGENTA "\x1b[35;1m"
#define COL_CYAN   "\x1b[36;1m"
#define COL_WHITE  "\x1b[37;1m"
#define COL_GRAY   "\x1b[90m"

#define MAX_SAVES 20
#define MAX_PATH 512
#define MONEY_MAX_CENTS 99999999 // $999,999.99 max seguro

typedef struct {
    char name[64];
    char folder[MAX_PATH];
    char file[MAX_PATH];
    long size;
    u32 money;
    long moneyOffset;
    int moneyFound;
    int moneyCopies;
    char status[32];
} SaveInfo;

static SaveInfo saves[MAX_SAVES];
static int saveCount = 0;
static int selected = 0;
static char msg[128] = "";
static char msgColor[16] = "\x1b[32;1m";
static int needConfirm = 0;
static int needRedraw = 1;

static void setMsg(const char *m, const char *color) {
    snprintf(msg, sizeof(msg), "%.120s", m);
    snprintf(msgColor, sizeof(msgColor), "%s", color);
    needRedraw = 1;
}
static void truncLine(char *dst, const char *src, int max) {
    if ((int)strlen(src) <= max) { strcpy(dst, src); return; }
    strncpy(dst, src, max - 3); dst[max - 3] = '\0'; strcat(dst, "...");
}
static u32 readLE32(unsigned char *p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }
static void writeLE32(unsigned char *p, u32 v) { p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }

static int findMainFile(const char *folder, char *outFile, long *outSize) {
    DIR *d = opendir(folder); if (!d) return 0;
    struct dirent *e; char best[MAX_PATH] = ""; long bestSize = 0;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char full[MAX_PATH]; snprintf(full, sizeof(full), "%s/%s", folder, e->d_name);
        struct stat st; if (stat(full, &st) != 0) continue; if (S_ISDIR(st.st_mode)) continue;
        int isSave = (strstr(e->d_name, "savedata")!=NULL)||(strstr(e->d_name,"TL.")!=NULL)||(strstr(e->d_name,"main")!=NULL)||(strstr(e->d_name,"SAVE")!=NULL);
        if (st.st_size > bestSize && (isSave || st.st_size > 50000)) { bestSize = st.st_size; snprintf(best, sizeof(best), "%s", full); }
    }
    closedir(d); if (best[0]=='\0') return 0;
    snprintf(outFile, MAX_PATH, "%s", best); *outSize = bestSize; return 1;
}
static int addSave(const char *folder, const char *displayName) {
    if (saveCount >= MAX_SAVES) return 0;
    for (int i=0;i<saveCount;i++) if (strcmp(saves[i].folder, folder)==0) return 0;
    char mainFile[MAX_PATH]; long sz=0; if (!findMainFile(folder, mainFile, &sz)) return 0; if (sz < 10000) return 0;
    SaveInfo *s=&saves[saveCount]; memset(s,0,sizeof(*s));
    snprintf(s->name,sizeof(s->name),"%.60s",displayName);
    snprintf(s->folder,sizeof(s->folder),"%s",folder);
    snprintf(s->file,sizeof(s->file),"%s",mainFile);
    s->size=sz; snprintf(s->status,sizeof(s->status),"OK"); s->moneyOffset=-1;
    saveCount++; return 1;
}
static void scanSaves() {
    saveCount=0; selected=0;
    const char *bases[] = {
        "sdmc:/3ds/Checkpoint/saves/000400000008C300 Tomodachi Life",
        "sdmc:/3ds/Checkpoint/saves/000400000008C300",
        "sdmc:/3ds/Checkpoint/saves/000400000008C400 Tomodachi Life",
        "sdmc:/3ds/Checkpoint/saves/000400000008C400",
        "sdmc:/3ds/Checkpoint/saves/00040000000EC300 Tomodachi Life", NULL
    };
    for (int b=0; bases[b]!=NULL; b++) {
        DIR *d=opendir(bases[b]); if (!d) continue;
        struct dirent *e;
        while ((e=readdir(d))!=NULL) {
            if (e->d_name[0]=='.') continue;
            char sub[MAX_PATH]; snprintf(sub,sizeof(sub),"%s/%s",bases[b],e->d_name);
            struct stat st; if (stat(sub,&st)!=0||!S_ISDIR(st.st_mode)) continue;
            if (!addSave(sub,e->d_name)) {
                DIR *d2=opendir(sub); if (d2) { struct dirent *e2; while((e2=readdir(d2))!=NULL){ if(e2->d_name[0]=='.') continue; char sub2[MAX_PATH]; snprintf(sub2,sizeof(sub2),"%s/%s",sub,e2->d_name); struct stat st2; if(stat(sub2,&st2)==0&&S_ISDIR(st2.st_mode)) addSave(sub2,e2->d_name); } closedir(d2); }
            }
        }
        closedir(d);
    }
    DIR *d=opendir("sdmc:/3ds/Checkpoint/saves");
    if (d) { struct dirent *e; while((e=readdir(d))!=NULL){ if(e->d_name[0]=='.') continue; if(strstr(e->d_name,"Tomodachi")==NULL&&strstr(e->d_name,"008C3")==NULL&&strstr(e->d_name,"008C4")==NULL) continue; char g[MAX_PATH]; snprintf(g,sizeof(g),"sdmc:/3ds/Checkpoint/saves/%s",e->d_name); DIR *dg=opendir(g); if(!dg) continue; struct dirent *eb; while((eb=readdir(dg))!=NULL){ if(eb->d_name[0]=='.') continue; char bk[MAX_PATH]; snprintf(bk,sizeof(bk),"%s/%s",g,eb->d_name); struct stat stb; if(stat(bk,&stb)==0&&S_ISDIR(stb.st_mode)) addSave(bk,eb->d_name); } closedir(dg); } closedir(d); }
}
static void readMoneyFor(SaveInfo *s) {
    FILE *f=fopen(s->file,"rb"); if(!f){ snprintf(s->status,sizeof(s->status),"ERRO ABRIR"); return; }
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    if(sz<10000||sz>8000000){ fclose(f); snprintf(s->status,sizeof(s->status),"TAMANHO ESTRANHO"); return; }
    unsigned char *buf=(unsigned char*)malloc(sz); if(!buf){ fclose(f); snprintf(s->status,sizeof(s->status),"SEM MEMORIA"); return; }
    if(fread(buf,1,sz,f)!=(size_t)sz){ free(buf); fclose(f); snprintf(s->status,sizeof(s->status),"ERRO LER"); return; }
    fclose(f);
    long start = sz>131072 ? sz-131072 : 0; long foundOff=-1; u32 foundVal=0; int copies=0;
    for(long off=sz-4; off>=start; off-=4){ u32 v=readLE32(buf+off); if(v>0&&v<=MONEY_MAX_CENTS){ u32 nxt=(off+4<sz)?readLE32(buf+off+4):0; if(nxt==0xFFFFFFFF) continue; foundOff=off; foundVal=v; break; } }
    if(foundOff>=0){ for(long off=start; off+4<=sz; off+=4) if(readLE32(buf+off)==foundVal) copies++; s->money=foundVal; s->moneyOffset=foundOff; s->moneyFound=1; s->moneyCopies=copies; snprintf(s->status,sizeof(s->status),"OK"); }
    else { s->moneyFound=0; snprintf(s->status,sizeof(s->status),"DINH. NAO ACHADO"); }
    free(buf);
}
static void refreshAllMoney(){ for(int i=0;i<saveCount;i++) readMoneyFor(&saves[i]); }
static int doBackup(SaveInfo *s){
    char bak[MAX_PATH+16]; snprintf(bak,sizeof(bak),"%s.bak",s->file);
    FILE *in=fopen(s->file,"rb"); if(!in) return 0; FILE *out=fopen(bak,"wb"); if(!out){ fclose(in); return 0; }
    unsigned char tmp[8192]; size_t n; while((n=fread(tmp,1,sizeof(tmp),in))>0) fwrite(tmp,1,n,out); fclose(in); fclose(out); return 1;
}
static int editMoney(SaveInfo *s){
    if(!s->moneyFound) return -2; if(s->money==MONEY_MAX_CENTS) return 1;
    if(!doBackup(s)) return -3;
    FILE *f=fopen(s->file,"r+b"); if(!f) return -4;
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char *buf=(unsigned char*)malloc(sz); if(!buf){ fclose(f); return -5; }
    if(fread(buf,1,sz,f)!=(size_t)sz){ free(buf); fclose(f); return -6; }
    u32 old=s->money; int changed=0; long start=sz>131072?sz-131072:0;
    for(long off=start; off+4<=sz; off+=4) if(readLE32(buf+off)==old){ writeLE32(buf+off,MONEY_MAX_CENTS); changed++; }
    if(changed==0 && s->moneyOffset>=0){ writeLE32(buf+s->moneyOffset,MONEY_MAX_CENTS); changed=1; }
    fseek(f,0,SEEK_SET); if(fwrite(buf,1,sz,f)!=(size_t)sz){ free(buf); fclose(f); return -7; }
    fclose(f); free(buf); readMoneyFor(s); if(s->money==MONEY_MAX_CENTS) return 0; return -8;
}
static void drawUI(){
    PrintConsole *top=consoleGet(GFX_TOP); PrintConsole *bot=consoleGet(GFX_BOTTOM);
    consoleSelect(top); printf("\x1b[2J");
    printf(COL_CYAN "Tomodachi Life - Save Editor V4\n");
    printf(COL_MAGENTA "por Darkness Metal\n");
    printf(COL_GRAY "----------------------------------------\n");
    if(saveCount==0){
        printf(COL_RED "NENHUM SAVE ACHADO!\n");
        printf(COL_WHITE "1. Abra o Checkpoint\n");
        printf(COL_WHITE "2. Faca backup do Tomodachi\n");
        printf(COL_WHITE "3. Aperte X pra atualizar\n");
        printf(COL_GRAY "Procurado em:\n"); printf(COL_GRAY "3ds/Checkpoint/saves/...\n");
    } else {
        printf(COL_WHITE "Saves encontrados: " COL_GREEN "%d\n", saveCount);
        printf(COL_GRAY "----------------------------------------\n");
        for(int i=0;i<saveCount;i++){
            SaveInfo *s=&saves[i]; char nameT[44]; truncLine(nameT,s->name,40);
            char mark=(i==selected)?'>':' '; const char *selCol=(i==selected)?COL_GREEN:COL_WHITE;
            printf("%s%c [%d] %s\n",selCol,mark,i+1,nameT);
            if(s->moneyFound){
                printf(COL_WHITE "    Tamanho: " COL_CYAN "%ld KB " COL_WHITE "| " COL_YELLOW "[$] " COL_YELLOW "%u" COL_WHITE " cents\n", s->size/1024, s->money);
                printf(COL_WHITE "    Copias: " COL_CYAN "%d " COL_WHITE "| Status: ", s->moneyCopies);
                if(s->money==MONEY_MAX_CENTS) printf(COL_GREEN "JA NO MAX!\n"); else printf(COL_GREEN "%s\n",s->status);
            } else { printf(COL_WHITE "    Tamanho: " COL_CYAN "%ld KB\n", s->size/1024); printf(COL_RED "    Status: %s\n",s->status); }
            char fileT[40]; truncLine(fileT,s->file,36); printf(COL_GRAY "    Arq: %s\n",fileT);
        }
    }
    printf(COL_GRAY "----------------------------------------\n");
    if(needConfirm) printf(COL_YELLOW "Aperte A DE NOVO pra confirmar!\n"); else printf(COL_WHITE "Use " COL_CYAN "Cima/Baixo" COL_WHITE " pra escolher\n");
    consoleSelect(bot); printf("\x1b[2J");
    printf(COL_CYAN "--- CONTROLES ---\n");
    printf(COL_GREEN "A" COL_WHITE " - Editar dinheiro pra " COL_YELLOW "[$] 999.999\n");
    printf(COL_CYAN "B" COL_WHITE " - Fazer backup .bak\n");
    printf(COL_YELLOW "X" COL_WHITE " - Atualizar lista\n");
    printf(COL_MAGENTA "Y" COL_WHITE " - Restaurar backup\n");
    printf(COL_RED "START" COL_WHITE " - Sair\n");
    printf(COL_GRAY "--------------------------------------\n");
    printf(COL_WHITE "Selecionado: "); if(saveCount>0) printf(COL_GREEN "%s\n",saves[selected].name); else printf(COL_RED "nenhum\n");
    printf("%s%s\n",msgColor,msg); printf(COL_RESET "");
    gfxFlushBuffers(); gspWaitForVBlank();
}
int main(int argc, char **argv){
    gfxInitDefault(); consoleInit(GFX_TOP,NULL); consoleInit(GFX_BOTTOM,NULL);
    scanSaves(); refreshAllMoney();
    if(saveCount==0) setMsg("Nenhum save. Abra Checkpoint e faca backup.",COL_RED); else setMsg("V4 pronta. Escolha um save.",COL_GREEN);
    drawUI();
    while(aptMainLoop()){
        hidScanInput(); u32 kDown=hidKeysDown(); int acted=0;
        if(kDown & KEY_DUP){ if(saveCount>0) selected=(selected+saveCount-1)%saveCount; needConfirm=0; acted=1; }
        if(kDown & KEY_DDOWN){ if(saveCount>0) selected=(selected+1)%saveCount; needConfirm=0; acted=1; }
        if(kDown & KEY_X){ scanSaves(); refreshAllMoney(); setMsg("Lista atualizada.",COL_CYAN); needConfirm=0; acted=1; }
        if(kDown & KEY_A){
            if(saveCount==0){ setMsg("Sem save pra editar.",COL_RED); acted=1; }
            else if(!needConfirm){ needConfirm=1; setMsg("Aperte A de novo pra confirmar!",COL_YELLOW); acted=1; }
            else { needConfirm=0; SaveInfo *s=&saves[selected]; setMsg("Editando, aguarde...",COL_YELLOW); drawUI(); int r=editMoney(s);
                if(r==0) setMsg("OK! Dinheiro no MAX! Restaure no Checkpoint.",COL_GREEN);
                else if(r==1) setMsg("Ja estava no MAX!",COL_GREEN);
                else if(r==-2) setMsg("Dinheiro nao achado nesse save.",COL_RED);
                else if(r==-3) setMsg("Falha no backup. Nao editei.",COL_RED);
                else if(r==-4) setMsg("Arquivo travado. Feche o jogo.",COL_RED);
                else if(r==-8) setMsg("Verificacao falhou. Backup restaurado.",COL_RED);
                else setMsg("Erro ao editar. Veja backup .bak",COL_RED); acted=1; }
        }
        if(kDown & KEY_B){ if(saveCount>0){ if(doBackup(&saves[selected])) setMsg("Backup .bak criado!",COL_GREEN); else setMsg("Falha no backup.",COL_RED); } else setMsg("Sem save.",COL_RED); needConfirm=0; acted=1; }
        if(kDown & KEY_Y){
            if(saveCount>0){ SaveInfo *s=&saves[selected]; char bak[MAX_PATH+16]; snprintf(bak,sizeof(bak),"%s.bak",s->file); FILE *in=fopen(bak,"rb");
                if(!in) setMsg("Sem backup .bak pra restaurar.",COL_RED);
                else { FILE *out=fopen(s->file,"wb"); if(!out){ fclose(in); setMsg("Nao consegui restaurar.",COL_RED); } else { unsigned char tmp[8192]; size_t n; while((n=fread(tmp,1,sizeof(tmp),in))>0) fwrite(tmp,1,n,out); fclose(in); fclose(out); refreshAllMoney(); setMsg("Backup restaurado!",COL_GREEN); } }
            } needConfirm=0; acted=1;
        }
        if(kDown & KEY_START) break;
        if(acted||needRedraw){ drawUI(); needRedraw=0; }
        gspWaitForVBlank();
    }
    gfxExit(); return 0;
}
