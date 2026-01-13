#ifndef LIN__HPP_
#define LIN__HPP_


#include <stdio.h>
#include "main.h"

const unsigned int linmaxlen = 8;

class Lin {

private:
  uint8_t iam;
  UART_HandleTypeDef huarthandler;
  uint8_t linpid;
  uint8_t linlen;

  unsigned int linindex;
  uint8_t lindatarecv[linmaxlen];
  uint8_t lindatasend[linmaxlen];
  
public:
  Lin(uint8_t,UART_HandleTypeDef);
  void init(uint16_t);
  uint8_t addparity(uint8_t);
  uint8_t checksum(const uint8_t *, uint8_t);

  void lintransmitbyte(uint8_t);
  int linreceivebyte();

  int lintransmit(uint8_t*, uint8_t);
  int linreceiveheader();
  int linreceivebody();
  int linsendbody();

  int read();


  void setdata(uint8_t, uint8_t);
  uint8_t getdata(uint8_t);
  uint8_t getlen();
  
  uint8_t getpid();
  uint8_t getsource();
  uint8_t getdest();
  
  
};


  
#endif
