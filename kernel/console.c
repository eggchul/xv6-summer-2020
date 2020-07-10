//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

//
// send one character to the uart.
//
void
consputc(int c)
{
  extern volatile int panicked; // from printf.c

  if(panicked){
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else {
    uartputc(c);
  }
}

struct input {
  struct spinlock lock;
  
  // input
#define INPUT_BUF 128
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
};

static int active = 1;

struct input cons1;
struct input cons2;
struct input *consa;


//
// user write()s to the console go here.
//
int
consolewrite(struct input *cons, int user_src, uint64 src, int n)
{
  int i;

  acquire(&cons->lock);
  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    if (cons == consa) {
      consputc(c);
    }
  }
  release(&cons->lock);

  return n;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(struct input *cons, int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons->lock);
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while(cons->r == cons->w){
      if(myproc()->killed){
        release(&cons->lock);
        return -1;
      }
      sleep(&cons->r, &cons->lock);
    }

    c = cons->buf[cons->r++ % INPUT_BUF];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons->r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons->lock);

  return target - n;
}

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//
void
consoleintr(int c)
{
  int doconsoleswitch = 0;

  acquire(&consa->lock);

  switch(c){
  case C('P'):  // Print process list.
    procdump();
    break;
  case C('U'):  // Kill line.
    while(consa->e != consa->w &&
          consa->buf[(consa->e-1) % INPUT_BUF] != '\n'){
      consa->e--;
      consputc(BACKSPACE);
    }
    break;
  case C('H'): // Backspace
  case '\x7f':
    if(consa->e != consa->w){
      consa->e--;
      consputc(BACKSPACE);
    }
    break;
  case C('T'):
      doconsoleswitch = 1;
      break;

  default:
    if(c != 0 && consa->e-consa->r < INPUT_BUF){
      c = (c == '\r') ? '\n' : c;

      // echo back to the user.
      consputc(c);

      // store for consumption by consoleread().
      consa->buf[consa->e++ % INPUT_BUF] = c;

      if(c == '\n' || c == C('D') || consa->e == consa->r+INPUT_BUF){
        // wake up consoleread() if a whole line (or end-of-file)
        // has arrived.
        consa->w = consa->e;
        wakeup(&consa->r);
      }
    }
    break;
  }
  
  release(&consa->lock);
  
  if(doconsoleswitch){
    if (active == 1){
      active = 2;
      consa = &cons2;
    }else{
      active = 1;
      consa = &cons1;
    } 

    printf("\nActive console now: %d\n", active);
  }

}

int
cread1(int user_dst, uint64 dst, int n)
{
  return consoleread(&cons1, user_dst, dst, n);
}

int
cwrite1(int user_src, uint64 src, int n)
{
  return consolewrite(&cons1, user_src, src, n);
}

int
cread2(int user_dst, uint64 dst, int n)
{
  return consoleread(&cons2, user_dst, dst, n);
}

int
cwrite2(int user_src, uint64 src, int n)
{
  return consolewrite(&cons2, user_src, src, n);
}

void
consoleinit(void)
{
  initlock(&cons1.lock, "cons1");
  cons1.r = 0;
  cons1.w = 0;
  cons1.e = 0;

  initlock(&cons2.lock, "cons2");
  cons2.r = 0;
  cons2.w = 0;
  cons2.e = 0;

  consa = &cons1;
  active = 1;

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE1].read = cread1;
  devsw[CONSOLE1].write = cwrite1;

  devsw[CONSOLE2].read = cread2;
  devsw[CONSOLE2].write = cwrite2;
}
