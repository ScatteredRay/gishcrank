#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string.h>

const char* charList = "abcdefghijklmnopqrstuvwxyz *";

const int keyList[] = {KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K,
KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_SPACE, KEY_ENTER};

const int div = 1000;
const int minSwap = 5;
const int bDebug = 0;

int sign(int x) {
  if(x >= 0) return 1;
  else return -1;
}

void emitKey(int c, int ofd) {

  struct input_event e;

  e.type = EV_KEY;
  e.code = c;
  e.value = 1;
  e.time.tv_sec = 0;
  e.time.tv_usec = 0;

  write(ofd, &e, sizeof(e));
  
  e.type = EV_SYN;
  e.code = SYN_REPORT;
  e.value = 0;
  
  write(ofd, &e, sizeof(e));

  //sleep(1);

  e.type = EV_KEY;
  e.code = c;
  e.value = 0;

  write(ofd, &e, sizeof(e));

  e.type = EV_SYN; 
  e.code = SYN_REPORT;
  e.value = 0;

  write(ofd, &e, sizeof(e));
}

void emit(int c, int ofd, int bNewChar) {

  if(bDebug) {
    return;
  }

  if(!bNewChar) {
    emitKey(KEY_BACKSPACE, ofd);
  }

  int keyListLen = sizeof(keyList)/sizeof(int);
  emitKey(keyList[c%keyListLen], ofd);
}

int main(int argc, char** argv) {
  int f;
  int ofd;
  struct input_event evnt;
  struct uinput_user_dev uud;
  int acc = 0;
  int c = 0;
  int s = 1;
  int ret = 0;


  if(!bDebug) {
    ofd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    ret = ioctl(ofd, UI_SET_EVBIT, EV_KEY);
    ret = ioctl(ofd, UI_SET_EVBIT, EV_SYN);

    int keyListLen = sizeof(keyList)/sizeof(int); 
    for(int i = 0; i < keyListLen; i++) { 
      ret = ioctl(ofd, UI_SET_KEYBIT, keyList[i]);
    }

    ret = ioctl(ofd, UI_SET_KEYBIT, KEY_BACKSPACE);

    memset(&uud, 0, sizeof(uud));
    snprintf(uud.name, UINPUT_MAX_NAME_SIZE, "gishcrank-keyboard");
    uud.id.bustype = BUS_HOST;
    uud.id.vendor = 1;
    uud.id.product = 2;
    uud.id.version = 1;

    write(ofd, &uud, sizeof(uud));
    sleep(1);

    ret = ioctl(ofd, UI_DEV_CREATE);
    sleep(1);
  }

  f = open(argv[1], O_RDONLY);

  while(1) {
    size_t readSize = read(f, &evnt, sizeof(evnt));

    if(evnt.type == EV_REL && evnt.code == REL_Y) {
      //printf("Y: %d %d %d\n", evnt.value, s, sign(evnt.value));
      if(sign(evnt.value) != s) {
        if(evnt.value * sign(evnt.value) >= minSwap) {
          s = sign(evnt.value);
          // Commit the value!
          if(bDebug) {
            printf("-\n");
          }
          c = 0;
          if(bDebug) {
            printf("%c", charList[c]);
          }
          emit(c, ofd, 1);
        }
        else continue;
      }
      acc += evnt.value * s;
      int inc = acc/div;
      if(inc > 0) {
        acc = acc % div;
        c = (c + inc) % strlen(charList); 
        if(bDebug) {
          printf("%c", charList[c]);
        }
        emit(c, ofd, 0);
      }
    }
  }

  if(!bDebug) {
    ioctl(ofd, UI_DEV_DESTROY);
    close(ofd);
  }

  close(f);
  return 0;
}
