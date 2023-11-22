#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "lomo2.h"

//Prototipos DLL
int (*LOMO_inicio)(int ret, int longitudMAxima, char const* login1, char const* login2);
void (*pon_error)(char const* mensaje);
int (*LOMO_generar_mapa)(char const* login1, char const* login2);
int (*LOMO_trenNuevo)(void);
int (*LOMO_peticiOnAvance)(int nt, int* xcab, int* ycab);
int (*LOMO_avance)(int nt, int* xcola, int* ycola);
char (*LOMO_getColor)(int nt);
void (*LOMO_espera)(int y, int yn);
int (*LOMO_fin)(void);

DWORD WINAPI ftrenes (LPVOID param);

void avance_tren(int id);
void verificar_pos(int x, int y, int id);

//Variables globales
HANDLE S1, S2, celdaLiberada;
HANDLE buffer, buffercola, bufferinterbloq;

typedef struct info {
    int id;
    int espera;
}info;

info (* matriz)[17];

int *colaEspera;

int *interbloq;

int main(int argc, char* argv[]){
    HINSTANCE libreria;
    int vel, tam, num; //Parametros
	char const *login1 = "i0919457", *login2 = "i2318151";
    int i, f, c;

    if ((libreria = LoadLibrary("lomo2.dll")) == NULL) {
        PERROR("Carga de libreria");
        return -1;
    }

    if ((LOMO_inicio = (int(*) (int, int, char const*, char const*))GetProcAddress(libreria, "LOMO_inicio")) == NULL) {
        PERROR("Carga de LOMO_inicio");
        return -1;
    }

    if ((pon_error = (void(*) (char const*))GetProcAddress(libreria, "pon_error")) == NULL) {
        PERROR("Carga de pon_error");
        return -1;
    }

    if ((LOMO_generar_mapa = (int(*) (char const*, char const*))GetProcAddress(libreria, "LOMO_generar_mapa")) == NULL) {
        PERROR("Carga de LOMO_generar_mapa");
        return -1;
    }

    if ((LOMO_trenNuevo = (int(*) (void))GetProcAddress(libreria, "LOMO_trenNuevo")) == NULL) {
        PERROR("Carga de LOMO_trenNuevo");
        return -1;
    }

    if ((LOMO_peticiOnAvance = (int(*) (int, int*, int*))GetProcAddress(libreria, "LOMO_peticiOnAvance")) == NULL) {
        PERROR("Carga de LOMO_peticiOnAvance");
        return -1;
    }

    if ((LOMO_avance = (int(*) (int, int*, int*))GetProcAddress(libreria, "LOMO_avance")) == NULL) {
        PERROR("Carga de LOMO_avance");
        return -1;
    }

    if ((LOMO_getColor = (char(*) (int))GetProcAddress(libreria, "LOMO_getColor")) == NULL) {
        PERROR("Carga de LOMO_getColor");
        return -1;
    }

    if ((LOMO_espera = (void(*) (int, int))GetProcAddress(libreria, "LOMO_espera")) == NULL) {
        PERROR("Carga de LOMO_espera");
        return -1;
    }

    if ((LOMO_fin = (int(*) (void))GetProcAddress(libreria, "LOMO_fin")) == NULL) {
        PERROR("Carga de LOMO_fin");
        return -1;
    }

    //Gestion parametros
	if (strcmp(argv[1], "--mapa") == 0){
		LOMO_generar_mapa(login1, login2);
		return 0;
	} else if (argc == 4){
		vel = atoi(argv[1]);
        tam = atoi(argv[2]);
		num = atoi(argv[3]);
		if (vel < 0 && vel > 99 && tam < 3 && tam > 19 && num <= 0 && num > 99){
			pon_error("Parametro no valido");
			return 1;
		}
	} else {
		pon_error("Parametro no valido");
		return 1;
	}

    //Semaforos y evento
    if((S1 = CreateSemaphore(NULL,1,1,"Sem1")) == NULL){
        pon_error("Error en creacion de semaforo 1");
        return 1;
    }

    if((S2 = CreateSemaphore(NULL,1,1,"Sem2")) == NULL){
        pon_error("Error en creacion de semaforo 2");
        return 1;
    }

    if ((celdaLiberada = CreateEvent(NULL, TRUE, FALSE, "celdaLiberada")) == NULL) {
        pon_error("Error en creacion de evento");
        return 1;
    }

    //Matriz en memoria compartida
    if ((buffer = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, sizeof(info) * 75 * 17, "buffer")) == NULL) {
        pon_error("Error en creacion de matriz memoria compartida");
        return 1;
    }

    if ((matriz = (info(*)[17])MapViewOfFile(buffer, FILE_MAP_WRITE, 0, 0, sizeof(info) * 75 * 17)) == NULL) {
        pon_error("Error en creacion de matriz en memoria compartida");
        CloseHandle(buffer);
        return 1;
    }

    for (c = 0; c <= 74; c++) {
        for (f = 0; f <= 16; f++) {
            matriz[c][f].id = -1;
            matriz[c][f].espera = 0;
        }
    }

    //Cola de Espera en memoria compartida
    if ((buffercola = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, sizeof(int) * 99, "buffercola")) == NULL) {
        pon_error("Error en creacion de cola en memoria compartida");
        return 1;
    }

    if ((colaEspera = ((int*)MapViewOfFile(buffercola, FILE_MAP_WRITE, 0, 0, sizeof(int) * 99))) == NULL) {
        pon_error("Error en creacion de cola en memoria compartida");
        CloseHandle(buffercola);
        return 1;
    }

    for (c = 0; c < 99; c++) {
        colaEspera[c] = -1;
    }

    //Vectir de interbloqueados en memoria compartida
    if ((bufferinterbloq = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, sizeof(int) * 99, "bufferinterbloq")) == NULL) {
        pon_error("Error en creacion de cola en memoria compartida");
        return 1;
    }

    if ((interbloq = ((int*)MapViewOfFile(bufferinterbloq, FILE_MAP_WRITE, 0, 0, sizeof(int) * 99))) == NULL) {
        pon_error("Error en creacion de cola en memoria compartida");
        CloseHandle(bufferinterbloq);
        return 1;
    }

    for (c = 0; c < 99; c++) {
        interbloq[c] = -1;
    }

    // LOMO INICIO
    LOMO_inicio(vel, tam, login1, login2);

    //Creacion de trenes
    int const numTrenes = 15;
    HANDLE tren[numTrenes];
    for (i = 0; i < numTrenes; i++) {
        if ((tren[i] = CreateThread(NULL, 0, &ftrenes, NULL, NULL, NULL)) == NULL) {
            pon_error("Error en la creación de trenes");
        }
    }

    WaitForMultipleObjects(numTrenes, tren, FALSE, 60000);

    for (i = 0; i < numTrenes; i++) {
        CloseHandle(tren[i]);
    }

    UnmapViewOfFile(matriz);
    UnmapViewOfFile(colaEspera);
    CloseHandle(buffer);
    CloseHandle(S1);
    CloseHandle(S2);
    CloseHandle(celdaLiberada);
    
    LOMO_fin();

    printf("Los id de los trenes interbloqueados son: ");
    for (i = 0; i < numTrenes; i++) {
        if(interbloq[i] != -1) printf("%d ", interbloq[i]);
    }

    return 0;
}

DWORD WINAPI ftrenes(LPVOID param) {
    int id;

    WaitForSingleObject(S1, INFINITE);
    id = LOMO_trenNuevo();
    if (id == -1) {
        pon_error("Fallo en LOMO_trenNuevo");
        return 1;
    }

    avance_tren(id);

    ReleaseSemaphore(S1, 1, NULL);

    while (1) {
        avance_tren(id);
    }
}

void avance_tren(int id){
    int xp,yp,xa,ya;
    if (LOMO_peticiOnAvance(id,&xp,&yp) == -1){
        pon_error("Fallo en LOMO_peticiOnAvance");
        exit(1);
    }

    verificar_pos(xp,yp,id);

    if (LOMO_avance(id,&xa,&ya) == -1){
        pon_error("Fallo en LOMO_avance");
        exit(1);
    } 

    if (xa == -1 && ya == -1) {
        ReleaseSemaphore(S2, 1, NULL);
        LOMO_espera(ya, yp);
        avance_tren(id);
    }
    else {
        matriz[xa][ya].id = -1;
        if (matriz[xa][ya].espera == 1) PulseEvent(celdaLiberada);
        ReleaseSemaphore(S2, 1, NULL);
    }
    LOMO_espera(ya, yp);
    return;
}

void verificar_pos(int x, int y, int id){
    WaitForSingleObject(S2, INFINITE);
    matriz[x][y].espera = 0;
    colaEspera[id] = -1;
	if (matriz[x][y].id != -1){
		//Esperar celda liberada
        matriz[x][y].espera = 1;
        colaEspera[id] = matriz[x][y].id;

        int buscado = id, i = 0;
        while (buscado != -1) {
            interbloq[i] = buscado;
            if (colaEspera[buscado] != -1) {
                if (colaEspera[buscado] == id) {
                    //Hay interbloqueo
                    pon_error("Interbloqueo detectado");
                    ExitThread(0);
                }
            }
            buscado = colaEspera[buscado];
            i++;
        }

        while (interbloq[i] != -1) {
            interbloq[i] = -1;
            i++;
        }
        ReleaseSemaphore(S2, 1, NULL);

        WaitForSingleObject(celdaLiberada, INFINITE);
		verificar_pos(x, y, id);
	} else {
        colaEspera[id] = -1;
		matriz[x][y].id = id;
	}
}