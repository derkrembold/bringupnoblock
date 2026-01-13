#include "main.h"
#include "lin.hpp"
#include "addresses.hpp"
#include "errors.hpp"


#define BIT(addr, num) ((addr >> num) & 0x01)


Lin::Lin(uint8_t iam, UART_HandleTypeDef huart) {
  Lin::iam = iam;
  linpid = 0;
  linlen = 0;
  linindex = 0;
  huarthandler = huart;
}

void Lin::init(uint16_t baud) {

	  /* USER CODE BEGIN UART4_Init 0 */

	  /* USER CODE END UART4_Init 0 */

	  /* USER CODE BEGIN UART4_Init 1 */

	  /* USER CODE END UART4_Init 1 */
	huarthandler.Instance = UART4;
	huarthandler.Init.BaudRate = 19200;
	huarthandler.Init.WordLength = UART_WORDLENGTH_8B;
	  //huart.Init.StartBits = UART_STARTBITS_0;
	huarthandler.Init.StopBits = UART_STOPBITS_1;
	huarthandler.Init.Parity = UART_PARITY_NONE;
	huarthandler.Init.Mode = UART_MODE_TX_RX;
	huarthandler.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huarthandler.Init.OverSampling = UART_OVERSAMPLING_16;
	huarthandler.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huarthandler.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huarthandler.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	  if (HAL_UART_Init(&huarthandler) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  if (HAL_UARTEx_SetTxFifoThreshold(&huarthandler, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  if (HAL_UARTEx_SetRxFifoThreshold(&huarthandler, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  if (HAL_UARTEx_DisableFifoMode(&huarthandler) != HAL_OK)
	  {
	    Error_Handler();
	  }
}

void Lin::lintransmitbyte(uint8_t data)
{


	HAL_UART_Transmit(&huarthandler, (uint8_t*)&data, 1,0xFFFF);
}

int Lin::linreceivebyte(void)
{
	uint8_t data;


	HAL_StatusTypeDef status = HAL_UART_Receive(&huarthandler, (uint8_t*)&data, 1, 0xFFF);

	  if (status == HAL_OK) {

			return data;
	  } else if (status == HAL_TIMEOUT) {

		  return  LIN_TIM_ERR;

	  } else {

		  return LIN_GEN_ERR;
	  }


}

int Lin::lintransmit(uint8_t* data, uint8_t len) {

  for (unsigned int i = 0; i < len; i++) {
    lintransmitbyte(data[i]);
  }

  lintransmitbyte(checksum(data,len));
  
  return LIN_OK;
}

int Lin::linreceiveheader() {

	int ret = 0;
	  for(;;) {

	    ret = linreceivebyte();
	    if (ret < 0) {
	      return LIN_TIM_ERR;
	    }
	    uint8_t sync = (uint8_t) ret;
	    if (sync == 0x55) {
	      break;
	    }
	  }


	  ret = linreceivebyte();
	  if (ret < 0) {
	    return LIN_TIM_ERR;
	  }
	  linpid = (uint8_t) ret;
	  if (linpid != addparity(linpid & 0x3f)) {
	    linpid = 0;
	    linlen = 0;
	    linindex = 0;
	    return LIN_PAR_ERR;
	  }
	  linpid &= 0x3f;

	  linindex = 0;
	  bool pidfound = false;
	  for (unsigned int i = 0; i < sizeof(pids); i++) {
	    if (pids[i] == linpid) {
	      pidfound = true;
	      linindex = i;
	      break;
	    }
	  }
	  if (pidfound == false) {
	    //linpid = 0;
	    linlen = 0;
	    linindex = 0;
	    return LIN_PID_ERR;
	    //return (int)(-linpid);
	  }

	  linlen = messagebytes[linindex];
	  if (linlen > linmaxlen) {
	    //linpid = 0;
	    linlen = 0;
	    return LIN_MSI_ERR;
	  }

	  return LIN_OK;

 
}


int Lin::linreceivebody() {

  
  for (unsigned int i = 0; i < linlen; i++) {

    int ret = linreceivebyte();
    if (ret < 0) {
      return LIN_TIM_ERR;
    }
    
    lindatarecv[i] = (uint8_t) ret;
  }

  int ret = linreceivebyte();
  if (ret < 0) {
    return LIN_TIM_ERR;
  }
  
  uint8_t check = (uint8_t) ret;
  uint8_t recvd = checksum(lindatarecv, linlen);

  if(check != recvd) {
    linpid = 0;
    linlen = 0;
    return LIN_CHK_ERR;
  }

  return LIN_OK;
}


int Lin::linsendbody() {

  uint8_t rcvcheck = 0;
  for (unsigned int i = 0; i < linlen; i++) {

    lintransmitbyte(lindatasend[i]);
    rcvcheck = linreceivebyte();
    if (lindatasend[i] != rcvcheck) {
    	return LIN_RCV_ERR;
    }
  }

  uint8_t check = checksum(lindatasend, linlen);

  lintransmitbyte(check);
  rcvcheck = linreceivebyte();
  if (check != rcvcheck) {
  	return LIN_RCV_ERR;
  }

  return LIN_OK;
}


void Lin::setdata(uint8_t value, uint8_t index) {

  lindatasend[index] = value;
}

uint8_t Lin::getdata(uint8_t index) {

  return lindatarecv[index];
}

uint8_t Lin::getpid() {
  return linpid;
}

uint8_t Lin::getsource() {
  return sources[linindex];
}
uint8_t Lin::getdest() {
  return destinations[linindex];
}

uint8_t Lin::getlen() {

  return Lin::linlen;
}

uint8_t Lin::addparity(uint8_t addr) {
  uint8_t temp = addr & 0x3f;
  uint8_t p0 = BIT(temp,0) + BIT(temp,1) + BIT(temp,2) + BIT(temp,4);
  p0 &= 0x01;
  uint8_t p1 = BIT(temp,1) + BIT(temp,3) + BIT(temp,4) + BIT(temp,5);
  p1 = (~p1) & 0x01;

  return addr | (p0 << 6) | (p1 << 7);
}


uint8_t Lin::checksum(const uint8_t *data, uint8_t len) {
	uint16_t checksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        checksum += data[i];
        if (checksum > 0xFF) {
            checksum = (checksum & 0xFF) + 1; // Carry addieren
        }
    }

    return (uint8_t)(~checksum);
}

/*
uint8_t Lin::checksum(const uint8_t *data, uint8_t len) {
  uint8_t temp = 0;
  while (len-- > 0)
    temp += *(data++);

  while (temp >> 8) // In case adding the carry causes another carry
    temp = (temp & 255) + (temp >> 8);
  return (~temp);
}
*/
