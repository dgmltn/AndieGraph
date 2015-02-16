// See:
// http://android.wooyd.org/JNIExample/files/JNIExample.pdf

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include "Z80/Z80.h"
#include "TI85.h"

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libti8x"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define PALETTE_BITS   1
#define PALETTE_SIZE   (1 << PALETTE_BITS)
#define COLOR_OFF 0xB657

static JavaVM *gJavaVM;
static jobject gInterfaceObject;
static jobject gInterfaceClass;
const char *kInterfacePath = "net/supware/tipro/NativeLib";

static int  palette[PALETTE_SIZE];

int Running     = 0;
int ScreenReady = 0;
byte KeyReady   = 0;       /* 1: Key has been pressed        */
int TickSec = 0;
int TickNsec = 0;

static uint16_t  make565(int red, int green, int blue)
{
    return (uint16_t)( ((red   << 8) & 0xf800) |
                       ((green << 3) & 0x07e0) |
                       ((blue  >> 3) & 0x001f) );
}

static int make888(int red, int green, int blue)
{
    return (int)( (0xFF  << 24) |
                  (red   << 16) |
                  (green << 8)  |
                  (blue  << 0)  );
}

/** initClassHelper() ****************************************/
/** Initialize an object of NativeLib class                 **/
/** so we can call its methods later                        **/
/*************************************************************/
void initClassHelper(JNIEnv *env, const char *path, jobject *objptr) {
    jclass cls = (*env)->FindClass(env, path);
    if (!cls) {
        LOGE("initClassHelper: failed to get %s class reference", path);
        return;
    }
    jmethodID constr = (*env)->GetMethodID(env, cls, "<init>", "()V");
    if (!constr) {
        LOGE("initClassHelper: failed to get %s constructor", path);
        return;
    }
    jobject obj = (*env)->NewObject(env, cls, constr);
    if (!obj) {
        LOGE("initClassHelper: failed to create %s object", path);
        return;
    }
    (*objptr) = (*env)->NewGlobalRef(env, obj);
}

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void) {
  //LOGD("InitMachine");

  /* Initialize variables */
  KeyReady = 0;
  Running  = 1;

  /* Done */
  return 1;
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void) {
    //LOGD("TrashMachine");

    Running = 0;
}

/** SetColor() ***********************************************/
/** Allocate new color.                                     **/
/*************************************************************/
void SetColor(byte N,byte R,byte G,byte B)
{
    /* Set requested color */
    palette[N&1] = make888(R,G,B);
    //LOGD("SetColor called");
}

/** RefreshScreen() ******************************************/
/** Put an image on the screen.                             **/
/*************************************************************/
void RefreshScreen(void)
{
    //LOGD("RefreshScreen called");
    ScreenReady = 1;

    // Introduce an artificial delay to simulate CPU speed
    useconds_t periodUsec = 22222;

    struct timespec tock;
    clock_gettime(CLOCK_REALTIME, &tock);

    if (TickSec != 0) {
        int diffUsec = (tock.tv_sec - TickSec) * 100000L + (tock.tv_nsec - TickNsec) / 10000;
        if (diffUsec < periodUsec) {
            periodUsec -= diffUsec;
        }
        //LOGE("periodUsec = %lu, diff = %lu usec, sec = %d, ns = %d", periodUsec, diffUsec, tock.tv_sec - TickSec, tock.tv_nsec - TickNsec);
    }
    TickSec = tock.tv_sec;
    TickNsec = tock.tv_nsec;

    usleep(periodUsec);

    ScreenReady = 0;
}

/** Keypad() *************************************************/
/** Poll the keyboard.                                      **/ 
/*************************************************************/
byte Keypad(void) {
    //LOGD("Keypad called");
    return (IS_KBD(KBD_ON));
}

/** ShowBackdrop() *******************************************/
/** Show backdrop image with calculator faceplate.          **/
/*************************************************************/
// Nothing needed here. Backdrop is handled in JAVA.
int ShowBackdrop(const char *FileName) { }

/** OnLoad ***************************************************/
/** Called from JAVA when JNI is initialized.               **/
/*************************************************************/
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    gJavaVM = vm;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
        return -1;

    /* get class with (*env)->FindClass */
    /* register methods with (*env)->RegisterNatives */

    /* Create an object of type NativeLib as a cached reference
    so we can call its methods later. JNI doesn't like us to
    cache class references, but caching objects is ok */
    initClassHelper(env, kInterfacePath, &gInterfaceObject);

    return JNI_VERSION_1_4;
}

/** onResume() ***********************************************/
/** Called when the main activity gets onResume()'d         **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_onResume(
    JNIEnv * env,
    jobject thiz
) {
    /* Create an object of type NativeLib as a cached reference
    so we can call its methods later. JNI doesn't like us to
    cache class references, but caching objects is ok */
    initClassHelper(env, kInterfacePath, &gInterfaceObject);
}

/** onPause() ************************************************/
/** Called when the main activity gets onPause()'d          **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_onPause(
    JNIEnv * env,
    jobject thiz
) {
    // TODO: destroy the object instead of nullifying it?
    gInterfaceObject = 0;
    gInterfaceClass = 0;
}

/** keyDown() ************************************************/
/** Key handler for keyDown event, called from JAVA.        **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_keyDown(
    JNIEnv * env,
    jobject thiz,
    int key
) {
    if (!Running) return;

    //LOGD("keyboard set: %d", key);
    KBD_SET(key);
    KeyReady = 1;
    return;
}

/** keyUp() **************************************************/
/** Key handler for keyUp event, called from JAVA.          **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_keyUp(
    JNIEnv * env,
    jobject thiz,
    int key
) {
    if(CPU.Trace) return;

    if (!Running) return;

    //LOGD("keyboard reset: %d", key);
    KBD_RES(key);
    KeyReady = 1;
    return;
}

/** loadState() **********************************************/
/** Key handler for loadState event, called from JAVA.      **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_loadState(
    JNIEnv * env,
    jobject thiz,
    jstring filename
) {

    if (!Running) return;

    jboolean isCopy;  
    const char * szFilename = (*env)->GetStringUTFChars(env, filename, &isCopy);  
    LoadSTA(szFilename);
  
    (*env)->ReleaseStringUTFChars(env, filename, szFilename);  
}

/** saveState() **********************************************/
/** Key handler for saveState event, called from JAVA.      **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_saveState(
    JNIEnv * env,
    jobject thiz,
    jstring filename
) {

    if (!Running) return;

    jboolean isCopy;  
    const char * szFilename = (*env)->GetStringUTFChars(env, filename, &isCopy);  
    SaveSTA(szFilename);
  
    (*env)->ReleaseStringUTFChars(env, filename, szFilename);  
}

/** renderScreen() ********************************************/
/** JNI call to draw the screen                             **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_renderScreen(
    JNIEnv * env, 
    jobject thiz, 
    jintArray colors
) {
    void*    pixels;
    int      x, y;

    if (!Running) return;

    while (!ScreenReady) {
        if (!Running) return;
        usleep(1000);
    }

    //LOGD("RenderScreen called");

    byte *vram;
    byte mask;
    int i = 0;

    jsize len = (*env)->GetArrayLength(env, colors);
    
    jint *elems = (*env)->GetIntArrayElements(env, colors, NULL);

    // TI85, TI86 (128x64 screens)
    if (TI85_FAMILY) {
      if (SLEEP_ON) {
        for (y = 0; y < 64 * 128; y++)
          elems[i++] = COLOR_OFF;
      }
      else {
        for (y = 0; y < 64; y++) {
          for (x = 0; x < 128; x++) {
            vram = SCREEN_BUFFER + (x / 8) + (y * 16);
            mask = 0x80 >> (x & 7);
            elems[i++] = palette[*vram & mask ? 1:0];
          }
        }
      }
    }

    // TI82, TI83, TI83P, TI84P (96x64 screens)
    else {
      if (SLEEP_ON) {
        for (y = 0; y < 64 * 96; y++)
          elems[i++] = COLOR_OFF;
      }
      else {
        for (y = 0; y < 64; y++) {
          for (x = 0; x < 96; x++) {
            vram = LCD.Buffer + (x / 8) + (y * 16);
            mask = 0x80 >> (x & 7);
            elems[i++] = palette[*vram & mask ? 1:0];
          }
        }
      }
    }

    (*env)->ReleaseIntArrayElements(env, colors, elems, 0);
}

/** start() **************************************************/
/** JNI call to start the emulator                          **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_start(
    JNIEnv * env, 
    jobject thiz,
    int modelId,
    jstring romFilename,
    jstring ramFilename
) {
    jboolean isCopy;

    //LOGD("start() called");

    if (!InitMachine()) return;

    // handle different calculator models "Mode"
    Mode = modelId ? modelId : ATI_TI85;

    // fill ROM file name
    const char * szRomFilename = (*env)->GetStringUTFChars(
        env, romFilename, &isCopy);  
    strcpy(ROMPath, szRomFilename);
    (*env)->ReleaseStringUTFChars(
        env, romFilename, szRomFilename);  

    // fill RAM file name
    const char * szRamFilename = (*env)->GetStringUTFChars(
        env, ramFilename, &isCopy);  
    strcpy(RAMPath, szRamFilename);
    (*env)->ReleaseStringUTFChars(
        env, ramFilename, szRamFilename);  

    // This runs until the calc. turns off
    StartTI85();

    TrashTI85();
    TrashMachine();

    return;
}
/** stop() ***************************************************/
/** JNI call to start the emulator                          **/
/*************************************************************/
JNIEXPORT void JNICALL Java_net_supware_tipro_NativeLib_stop(
    JNIEnv * env, 
    jobject thiz
) {
    //LOGD("stop() called");
    ExitNow = 1;
    return;
}

