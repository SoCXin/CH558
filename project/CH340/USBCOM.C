/********************************** (C) COPYRIGHT *******************************
* File Name          : USBCOM.C
* Author             : WCH
* Version            : V1.0
* Date               : 2015/05/20
* Description        : CH558ģ�⴮��
*******************************************************************************/
#include <string.h>
#include "DEBUG.C"
#include "CH558.H"

#define THIS_ENDP0_SIZE         DEFAULT_ENDP0_SIZE

UINT8X	Ep0Buffer[THIS_ENDP0_SIZE] _at_ 0x0000;                                //�˵�0 OUT&IN��������������ż��ַ
UINT8X	Ep2Buffer[2*MAX_PACKET_SIZE] _at_ 0x0008;                              //�˵�2 IN&OUT������,������ż��ַ
UINT8X  Ep1Buffer[MAX_PACKET_SIZE] _at_ 0x00a0;

UINT8	  SetReqtp,SetupReq,SetupLen,UsbConfig,Flag;
PUINT8  pDescr;	                                                               
UINT8   num = 0;
UINT8   LEN = 0;
USB_SETUP_REQ	           SetupReqBuf;                                          //�ݴ�Setup��
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)

 
UINT8C DevDesc[18]={0x12,0x01,0x10,0x01,0xff,0x00,0x02,0x08,                   //�豸������
                    0x86,0x1a,0x23,0x55,0x04,0x03,0x00,0x00,
                    0x00,0x01};

UINT8C CfgDesc[39]={0x09,0x02,0x27,0x00,0x01,0x01,0x00,0x80,0xf0,              //�������������ӿ�������,�˵�������
	                  0x09,0x04,0x00,0x00,0x03,0xff,0x01,0x02,0x00,           
                    0x07,0x05,0x82,0x02,0x20,0x00,0x00,                        //�����ϴ��˵�
		                0x07,0x05,0x02,0x02,0x20,0x00,0x00,                        //�����´��˵�      
			              0x07,0x05,0x81,0x03,0x08,0x00,0x01};                       //�ж��ϴ��˵�

UINT8C DataBuf[26]={0x30,0x00,0xc3,0x00,0xff,0xec,0x9f,0xec,0xff,0xec,0xdf,0xec,
                    0xdf,0xec,0xdf,0xec,0x9f,0xec,0x9f,0xec,0x9f,0xec,0x9f,0xec,
                    0xff,0xec};
UINT8 RecBuf[64];
/*******************************************************************************
* Function Name  : USBDeviceCfg()
* Description    : USB�豸ģʽ����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceCfg()
{
    USB_CTRL = 0x00;                                                           //���USB���ƼĴ���
    USB_CTRL &= ~bUC_HOST_MODE;                                                //��λΪѡ���豸ģʽ
    USB_CTRL |=  bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;					           //USB�豸���ڲ�����ʹ��,���ж��ڼ��жϱ�־δ���ǰ�Զ�����NAK
    USB_DEV_AD = 0x00;                                                         //�豸��ַ��ʼ��

    UDEV_CTRL &= ~bUD_RECV_DIS;                                                //ʹ�ܽ�����
//  USB_CTRL |= bUC_LOW_SPEED;    
//  UDEV_CTRL |= bUD_LOW_SPEED;                                                //ѡ�����1.5Mģʽ

    USB_CTRL &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                               //ѡ��ȫ��12Mģʽ��Ĭ�Ϸ�ʽ

    UDEV_CTRL |= bUD_DP_PD_DIS | bUD_DM_PD_DIS;                                //��ֹDM��DP��������
    UDEV_CTRL |= bUD_PORT_EN;                                                  //ʹ�������˿�
}


/*******************************************************************************
* Function Name  : USBDeviceIntCfg()
* Description    : USB�豸ģʽ�жϳ�ʼ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceIntCfg()
{
    USB_INT_EN |= bUIE_SUSPEND;                                                //ʹ���豸�����ж�
    USB_INT_EN |= bUIE_TRANSFER;                                               //ʹ��USB��������ж�
    USB_INT_EN |= bUIE_BUS_RST;                                                //ʹ���豸ģʽUSB���߸�λ�ж�
    USB_INT_FG |= 0x1F;                                                        //���жϱ�־
    IE_USB = 1;                                                                //ʹ��USB�ж�
    EA = 1; 																                                   //������Ƭ���ж�
}


/*******************************************************************************
* Function Name  : USBDeviceEndPointCfg()
* Description    : USB�豸ģʽ�˵�����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceEndPointCfg()
{
    UEP2_DMA = Ep2Buffer;                                                      //�˵�2���ݴ����ַ																			                                         
    UEP2_3_MOD |= bUEP2_TX_EN;                                                 //�˵�2����ʹ��
    UEP2_3_MOD |= bUEP2_RX_EN;                                                 //�˵�2����ʹ��
    UEP2_3_MOD &= ~bUEP2_BUF_MOD;                                              //�˵�2��64�ֽڷ��ͻ���������64�ֽڽ��ջ���������128�ֽ�
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;								 //�˵�2�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����ACK
	
	
	  UEP1_DMA = Ep1Buffer;                                                      //�˵�1���ݴ����ַ																			                                         
    UEP4_1_MOD |= bUEP1_TX_EN;                                                 //�˵�1����ʹ��
//  UEP4_1_MOD |= bUEP1_RX_EN;                                                 //�˵�1����ʹ��
    UEP4_1_MOD &= ~bUEP1_BUF_MOD;                                              //�˵�1��64�ֽڷ��ͻ�����
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;								 //�˵�1�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����ACK
	
	
    UEP0_DMA = Ep0Buffer;                                                      //�˵�0���ݴ����ַ
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);								                 //�˵�0��64�ֽ��շ�������
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //OUT���񷵻�ACK��IN���񷵻�NAK
}


/*******************************************************************************
* Function Name  : SendData( PUINT8 SendBuf )
* Description    : �������ݸ���������
* Input          : PUINT8 SendBuf
* Output         : None
* Return         : None
*******************************************************************************/
void SendData( PUINT8 SendBuf )
{
	 if(Flag==1)                             
	 {
     while(LEN > 32){		 
     memcpy(&Ep2Buffer[MAX_PACKET_SIZE],SendBuf,32);
	   UEP2_T_LEN = 32;
	   UEP2_CTRL &= ~(bUEP_T_RES1 | bUEP_T_RES0);	
     while(( UEP2_CTRL & MASK_UEP_T_RES ) == UEP_T_RES_ACK);                  //
     LEN -= 32;
     }
     memcpy(&Ep2Buffer[MAX_PACKET_SIZE],SendBuf,LEN);
	   UEP2_T_LEN = LEN;
	   UEP2_CTRL &= ~(bUEP_T_RES1 | bUEP_T_RES0);		 		
     Flag = 0;		 
   }
}


/*******************************************************************************
* Function Name  : RecieveData()
* Description    : USB�豸ģʽ�˵�����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RecieveData()
{
	  memcpy(RecBuf,Ep2Buffer,USB_RX_LEN); 
	  UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;                    //Ĭ��Ӧ��ACK
	  Flag = 1;
}


/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB�жϴ�������
*******************************************************************************/
void	DeviceInterrupt( void ) interrupt INT_NO_USB using 1                       //USB�жϷ������,ʹ�üĴ�����1
{   
	UINT8 len; 
	if(UIF_TRANSFER)                                                               //USB������ɱ�־
  {
    switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
    {
			 case UIS_TOKEN_OUT | 2:                                                   //endpoint 2# �ж��´�					 
						LEN = USB_RX_LEN; 
			      RecieveData();
            SendData(RecBuf);			 
						break;
		   case UIS_TOKEN_IN | 2:                                                    //endpoint 2# �ж��ϴ�
            UEP2_T_LEN = 0;	                                                     //Ԥʹ�÷��ͳ���һ��Ҫ���						 
	          UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;                    //Ĭ��Ӧ��ACK					 
    			  UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;            //Ĭ��Ӧ��NAK
						break;
    	 case UIS_TOKEN_SETUP | 0:                                                  //SETUP����
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {   
							 SetReqtp = UsbSetupBuf->bRequestType;
               SetupLen = UsbSetupBuf->wLengthL;
               len = 0;                                                           //Ĭ��Ϊ�ɹ������ϴ�0����,��׼����                                                              
               SetupReq = UsbSetupBuf->bRequest;
               if(SetReqtp == 0xc0)
						   {
								  Ep0Buffer[0] = DataBuf[num];
								  Ep0Buffer[1] = DataBuf[num+1];
								  len = 2;
								  if(num<24)
								  {	
								    num += 2;
									}
									else
									{
										num = 24;
									}
						   }
					     else if(SetReqtp == 0x40)
						   {
							    len = 9;                                                        //��֤״̬�׶Σ�����ֻҪ��8���Ҳ�����0xff����
						   }
						   else
						   { 
							    switch(SetupReq)                                                //������
							    {
								     case USB_GET_DESCRIPTOR:
											    switch(UsbSetupBuf->wValueH)
											    {
													   case 1:	                                            //�豸������
																 pDescr = DevDesc;                                //���豸�������͵�Ҫ���͵Ļ�����
																 len = sizeof(DevDesc);								       
													   break;	 
													   case 2:									                            //����������
																 pDescr = CfgDesc;                                //�������������͵�Ҫ���͵Ļ�����
																 len = sizeof(CfgDesc);
													   break;	
													   default:
																 len = 0xff;                                      //��֧�ֵ�������߳���
													   break;
											     }
									         if ( SetupLen > len ) SetupLen = len;                  //�����ܳ���
									         len = SetupLen >= 8 ? 8 : SetupLen;                    //���δ��䳤��
									         memcpy(Ep0Buffer,pDescr,len);                          //�����ϴ�����
									         SetupLen -= len;
									         pDescr += len;
										       break;						 
							        case USB_SET_ADDRESS:
										       SetupLen = UsbSetupBuf->wValueL;                       //�ݴ�USB�豸��ַ
										       break;
							        case USB_GET_CONFIGURATION:
									         Ep0Buffer[0] = UsbConfig;
									         if ( SetupLen >= 1 ) len = 1;
									         break;
							        case USB_SET_CONFIGURATION:
									         UsbConfig = UsbSetupBuf->wValueL;
									         break;
							        default:
										       len = 0xff;                                            //����ʧ��
										       break;    
							       }
					        }
				      }
					    else
					    {
							    len = 0xff;                                                     //�����ȴ���
					    }

						  if(len == 0xff)
						  {
								  SetupReq = 0xFF;
								  UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL				     
						  }
						  else if(len <= 8)                                                         //�ϴ����ݻ���״̬�׶η���0���Ȱ�
						  {
								  UEP0_T_LEN = len;
								  UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;  //Ĭ�����ݰ���DATA1������Ӧ��ACK
						  }
						  else
						  {
								  UEP0_T_LEN = 0;                                                       //��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
								  UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;  //Ĭ�����ݰ���DATA1,����Ӧ��ACK				     
						  }
					    break;
				 case UIS_TOKEN_IN | 0:                                                         //endpoint0 IN
						  switch(SetupReq)
						  {
							   case USB_GET_DESCRIPTOR:
								      len = SetupLen >= 8 ? 8 : SetupLen;                               //���δ��䳤��
											memcpy( Ep0Buffer, pDescr, len );                                 //�����ϴ�����
											SetupLen -= len;
											pDescr += len;
											UEP0_T_LEN = len;
											UEP0_CTRL ^= bUEP_T_TOG;                                          //ͬ����־λ��ת
								      break;
							   case USB_SET_ADDRESS:
											USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
											UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
								      break;
							   default:
								      UEP0_T_LEN = 0;                                                    //״̬�׶�����жϻ�����ǿ���ϴ�0�������ݰ��������ƴ���
								      UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
								      break;
						  }
						  break;
				 case UIS_TOKEN_OUT | 0:                                                 // endpoint0 OUT
							len = USB_RX_LEN;
							UEP0_T_LEN = 0;                                                    //��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
							UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;                         //Ĭ�����ݰ���DATA0,����Ӧ��ACK									
						  break;
					default:
						  break;
				}
				UIF_TRANSFER = 0;                                                        //д0����ж�  
    }
    if(UIF_BUS_RST)                                                              //�豸ģʽUSB���߸�λ�ж�
    {
			USB_DEV_AD = 0x00;
			UIF_SUSPEND = 0;
			UIF_TRANSFER = 0;
			UIF_BUS_RST = 0;                                                           //���жϱ�־
    }
	  if (UIF_SUSPEND) 
		{                                                                            //USB���߹���/�������
			UIF_SUSPEND = 0;
			if ( USB_MIS_ST & bUMS_SUSPEND ) 
			{                                                                          //����
				while ( XBUS_AUX & bUART0_TX );                                          //�ȴ��������
				SAFE_MOD = 0x55;
				SAFE_MOD = 0xAA;
				WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO;                                  //USB����RXD0���ź�ʱ�ɱ�����
				PCON |= PD;                                                              //˯��
				SAFE_MOD = 0x55;
				SAFE_MOD = 0xAA;
				WAKE_CTRL = 0x00;
			}
    } 
	  else 
	  {                                                                             //������ж�,�����ܷ��������
		  USB_INT_FG = 0x00;                                                          //���жϱ�־
	  }      
}


void main()
{
	  mDelaymS(30);                                                                 //�ϵ���ʱ
//  CfgFsys( );                                                                   //CH559ʱ��ѡ������    
    mInitSTDIO( );                                                                //����0,�������ڵ���
	  USBDeviceCfg();                                                               //�豸ģʽ����
    USBDeviceEndPointCfg();														                            //�˵�����
    USBDeviceIntCfg();															                              //�жϳ�ʼ��
	  UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;	                                                              //Ԥʹ�÷��ͳ���һ��Ҫ���	
    UEP2_T_LEN = 0;	                                                      
    while(1)
    {   
//         SendData(RecBuf);
// 		    mDelaymS( 500 );                                                         //ģ�ⵥƬ����������				
    }
}
