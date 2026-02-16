
#ifndef __PT_RP2040_SERIAL_H__
#define __PT_RP2040_SERIAL_H__
#include <string.h> // for memset
// === serial input thread ================================
#define pt_buffer_size 255
static pt_t pt_serialin, pt_serialout ;
#define UART_ID uart0
#define pt_backspace 0x7f // make sure your backspace matches this!

static PT_THREAD (pt_serialin_polled(pt_t *pt))
{ char* buffer = (char*)pt->data;
  PT_BEGIN(pt);
  static uint8_t ch ;
  static int pt_current_char_count ;

  memset(buffer, 0, pt_buffer_size);       // clear the string
  pt_current_char_count = 0 ;
  while(uart_is_readable(UART_ID)) uart_getc(UART_ID);       // clear uart fifo

  while(pt_current_char_count < pt_buffer_size) // build the output string
  { PT_YIELD_UNTIL(pt, (int)uart_is_readable(UART_ID)); ch = uart_getc(UART_ID); // read one character
    PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, ch);  // echo back
    if (ch == '\r' ) // check for <enter> or <backspace>
    { // advances the cursor to the next line, then exits
      buffer[pt_current_char_count] = 0 ; // <enter>> character terminates string,
      PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, '\n') ;
      break; 
    }

    if (ch == pt_backspace) // check for <backspace>
    {
      PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, ' ') ;
      PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, pt_backspace);
      pt_current_char_count-- ;
      if (pt_current_char_count<0) pt_current_char_count = 0 ;
      continue;
    }
    buffer[pt_current_char_count++] = ch; // must be a real character, build the output string
  } // END WHILe
  PT_EXIT(pt); // kill this input thread, to allow spawning thread to execute
  PT_END(pt);
} // serial input thread

// ================================================================
// === serial output thread
//
int pt_serialout_polled(pt_t *pt)
{   char* buffer = (char*)pt->data;
    static int num_send_chars ;
    PT_BEGIN(pt);
    num_send_chars = 0;
    while (buffer[num_send_chars] != 0)
    {
        PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)) ;
        uart_putc(UART_ID, buffer[num_send_chars]) ;
        num_send_chars++;
    }
    PT_EXIT(pt);
    PT_END(pt);
}
// ================================================================
// package the spawn read/write macros to make them look better
#define serial_write(buffer) PT_SPAWN(pt,&pt_serialout,(buffer),pt_serialout_polled(&pt_serialout));
#define serial_read(buffer)  PT_SPAWN(pt,&pt_serialin,(buffer),pt_serialin_polled(&pt_serialin));

#endif /* __PT_RP2040_SERIAL_H__ */
