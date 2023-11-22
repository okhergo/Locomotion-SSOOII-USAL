// El siguiente bloque ifdef muestra la forma estAndar de crear macros que facilitan 
// la exportaciOn de archivos DLL. Todos los archivos de este archivo DLL se compilan
// con el sImbolo LOMO2_EXPORTS definido en la lInea de comandos. Este sImbolo no se 
// debe definir en ningUn proyecto que utilice este archivo DLL. De este modo, otros
// proyectos cuyos archivos de cOdigo fuente incluyan el archivo interpreta que las
// funciones LOMO2_API se importan de un archivo DLL, mientras que este archivo DLL
// interpreta los sImbolos definidos en esta macro como si fueran exportados.
#ifdef LOMO2_EXPORTS
#define LOMO2_API __declspec(dllexport)
#else
#define LOMO2_API __declspec(dllimport)
#endif

#define PERROR(a) \
    {             \
        LPVOID lpMsgBuf;                                         \
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |           \
                   FORMAT_MESSAGE_FROM_SYSTEM |                  \
                   FORMAT_MESSAGE_IGNORE_INSERTS, NULL,          \
                   GetLastError(),                               \
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),    \
                   (LPTSTR) &lpMsgBuf,0,NULL );                  \
        fprintf(stderr,"%s:(%d)%s\n",a,GetLastError(),lpMsgBuf); \
        LocalFree( lpMsgBuf );                                   \
    }   

#ifdef LOMO2_EXPORTS
extern "C" LOMO2_API  int LOMO_generar_mapa(char const* login1,
    char const* login2);
extern "C" LOMO2_API  int LOMO_inicio(int ret, int maxLongitud,
    char const* login1,
    char const* login2);
extern "C" LOMO2_API  int LOMO_trenNuevo(void);
extern "C" LOMO2_API  int LOMO_peticiOnAvance(int nt, int* xcab, int* ycab);
extern "C" LOMO2_API  int LOMO_avance(int nt, int* xcola, int* ycola);
extern "C" LOMO2_API const char* LOMO_getColor(int nt);
extern "C" LOMO2_API void LOMO_espera(int y, int yn);
extern "C" LOMO2_API  int LOMO_fin(void);
extern "C" LOMO2_API int refrescar(void);
extern "C" LOMO2_API void pon_error(char const* mensaje);
#endif