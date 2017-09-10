/*
 * Project name:
     GSM2Click
 * Revision History:
     V1.0 20170715: Kijelzes
     V1.1 20170909: rxbuff-ba nem megy 0d, 0a, 20
     V1.2
     V1.3

 * Description:
    - LCD-re kiiras 4 bites mod, Click2 adas-vetel
    -
    
 * Test configuration:
     MCU:             PIC18F45K22
                      http://ww1.microchip.com/downloads/en/DeviceDoc/41412F.pdf

     Dev.Board:       Breadboard
                         Billentyu csatlakozas: K0 - Fel
                                                K4 - OK/Menu
                                                K8 - Le
                         UART1 (HW):            Click2        (115200)
                                                RC6 (Pin 25)   = TX
                                                RC7 (Pin 26)   = RX

                      GSM2 Click:               PWR_Key (zold) = RA0 (Pin 2)
                                                Stat (fek)     = RA1 (Pin 3)
                                                TXD (kek)      = RC7 (Pin 26)
                                                RXD (feh)      = RC6 (Pin 25)

                         
     Oscillator:      HS 8.0000 MHz Crystal   4X PLL
     Ext. Modul:      GSM Click2
                      https://shop.mikroe.com/gsm-2-click
                      

     SW:              mikroC PRO for PIC   v6.6.3
 * NOTES:

 */


/*******************************************************************************
* Változok
*******************************************************************************/

// unsigned char hours, minutes, seconds, day, week, month, year;  // Orahoz

// int select, select_op, selection, submenu;                      // Menuhoz

/*******************************************************************************
* Állandók
*******************************************************************************/

// const Fel_gomb = 1, OK_gomb = 2, Le_gomb =3;

// Lcd csatlakozasai
sbit LCD_RS at LATB0_bit;
sbit LCD_EN at LATB1_bit;
sbit LCD_D4 at LATB2_bit;
sbit LCD_D5 at LATB3_bit;
sbit LCD_D6 at LATB4_bit;
sbit LCD_D7 at LATB5_bit;

sbit LCD_RS_Direction at TRISB0_bit;
sbit LCD_EN_Direction at TRISB1_bit;
sbit LCD_D4_Direction at TRISB2_bit;
sbit LCD_D5_Direction at TRISB3_bit;
sbit LCD_D6_Direction at TRISB4_bit;
sbit LCD_D7_Direction at TRISB5_bit;

// GSM Click2
sbit GSM_Pwr_Key at LATA0_bit; // Pin for powering the module on/off
sbit GSM_Pwr_Key_Dir at TRISA0_bit;

// Szovegek
//char txt0[20] = "";         //State LCD Print
char txt1[] = "GSM module ON";         //State LCD Print
char txt2[] = "PROGRAMMING";        //State LCD Print
char txt3[] = "NETWORK OK";              //State LCD Print
char txt4[] = "MASSAGES SENT";          //State LCD Print

char *txt5 = "Hello Feri";

char PIN[] = "6355";
char Osszefuz[20] = "";

//M95 parancsok
// const char atc0[] = "AT";                        // Every AT command starts with "AT"
char Echo_off[] = "ATE0";                      // Echo off
// const char atc2[] = "AT+CMGF=1";                 // TXT messages
char Van_PIN[] = "AT+CPIN";                  // PIN lekerdezes
char Ido[] = "AT+QNITZ";                             // datum, Ido
char Letesz[] = "ATH0";                                //Bejovo hivast leteszi
char Hivo[] = "AT+CLCC";                             //Bejovo hivo szama

char Kerdes[] = "?";
char Egyenlo[] = "=";


// Status LED
sbit M95_PW_LED at LATD0_bit;                       // ON kijelzes
sbit M95_PW_LED_Dir at TRISD0_bit;

sbit GSM_OK_LED at LATD1_bit;
sbit GSM_OK_LED_Dir at TRISD1_bit;

sbit ERROR_LED at LATD2_bit;
sbit ERROR_LED_Dir at TRISD2_bit;

// UART
char rxbuff[160];              // Buffer variable for storing data sent from master
char rxidx;                   // Counter for data written in buffer
char temp_rxidx;

//eletjelhez
unsigned char q = 0;

/*******************************************************************************
* Megszakitas rutin
*******************************************************************************/
void interrupt () {
     if(RC1IF_bit == 1) {             // Checks for Receive Interrupt Flag bit

        if ((UART1_Read() != 0X0D) && (UART1_Read() != 0X0A) && (UART1_Read() != 0X20))
           {
           rxbuff[rxidx] = UART1_Read();   // Storing read data
           rxidx++;                       // Incresing counter of read data
           }
        if(rxidx >= 160)                // Checks if data is larger than buffer
           rxidx = 0;                   // Reset counter
        }
}
/******************************************************************************/
void setup () {
      ANSELA = 0;             // Set all ports as digital
      ANSELB = 0;
      ANSELC = 0;
      ANSELD = 0;
      ANSELE = 0;

      SLRCON = 0;             // Set output slew rate on all ports at standard rate

      TRISE = 0;
      LATE = 0;
      TRISD = 0;
      LATD = 0;
      TRISC = 0;
      LATC = 0;

      IPEN_bit = 0;           //Interrupt priority disable

      Lcd_Init();
      Lcd_Cmd(_LCD_CLEAR);
      Lcd_Cmd(_LCD_CURSOR_OFF);
      
      GSM_Pwr_Key_Dir = 0;
      M95_PW_LED_Dir = 0;

      UART1_Init(115200);                  // 9600 baud -nal "lemarad" ?
      Delay_ms(100);
      
      rxidx  = 0;
}
/******************************************************************************/
void M95re_kuld(char *m) {
     while(*m) {
          UART1_Write(*m++);
          }
     UART1_Write(0x0D);
}
/******************************************************************************/
void GSM_PWR_On () {                      /* Bekapcsolas, PIN ellenorzes: ha a valasz:
                "READY", akkor STAT_LED on es kilep a rutinbol
                "SIM PIN", akkor var a PIN-re, STAT_LED villog 200 msec, ujbol ellenorizni a bevitel utan PIN statust,
                "SIM PUK", akkor LCD-re "Megszivtad", kikapcsolni a modult, STAT_LED villog 1 sec,
                minden mas valasz eseten lasd SIM PUK.
                                          */
     Delay_ms (4000);
     GSM_Pwr_Key = 1;
     Delay_ms (2000);                     // > 1 sec
//     GSM_Pwr_Key = 0;
     Delay_ms (4000);
}
/*LCD 1 soranak torlese********************************************************/
void LCD_clear_row (unsigned char r) {
     LCD_Out(r,1,"                    ");
     switch (r){
            case 1:
                 LCD_Cmd (_LCD_FIRST_ROW);
                 break;
            case 2:
                 LCD_Cmd (_LCD_SECOND_ROW);
                 break;
            default:
                 ERROR_LED = 1 ; // Csak jelezzem a hibat
                 break;
     }
}
/******************************************************************************/
void LCDre_kuld_adas(char *m1) {
     LCD_clear_row (1);
     while(*m1) {
          LCD_Chr_Cp(*m1++);
     }
}
/******************************************************************************/
void LCDre_kuld_vetel (char *m2){
     LCD_clear_row (2);
     temp_rxidx = rxidx;
     while(temp_rxidx > 0)  {
          temp_rxidx--;
          LCD_Chr_CP(*m2++);
     }
}
/*PIN kiirasa EEPROM-ba********************************************************/
void PINtoEEPROM(char *m3){
     char k = 0;
//     LCD_clear_row(1);
     while(*m3){
//           LCD_CHR(1,1+k,*m3);
          EEPROM_Write(0x00+k++, *m3++);
     }
}
/*Uzenet osszeallitasa 2 stringbol*********************************************/
void OsszefuzMSG2 (char *elso, char *masodik){       //Az eredmeny max 20 karakter!
     Osszefuz[0] = 0;
     strcat(Osszefuz, elso);
     strcat(Osszefuz, masodik);
}
/*Uzenet osszeallitasa 3 stringbol*********************************************/
void OsszefuzMSG3 (char *elso, char *masodik, char *harmadik) {   //Az eredmeny max 20 karakter!
     Osszefuz[0] = 0;
     strcat(Osszefuz, elso);
     strcat(Osszefuz, masodik);
     strcat(Osszefuz, harmadik);
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
void main() {

     setup ();
     

     GSM_PWR_On ();
     LCD_Out (1,1,txt1);
     M95_PW_LED = 1;
     
     RC1IE_bit = 1;                    // turn ON interrupt on UART1 receive
     RC1IF_bit = 0;                    // Clear interrupt flag
     PEIE_bit  = 1;                    // Enable peripheral interrupts
     GIE_bit   = 1;                    // Enable GLOBAL interrupts

     M95re_kuld (Echo_off);
     Delay_ms (3000);
      LCDre_kuld_adas (Echo_off);
      LCDre_kuld_vetel(rxbuff);

/* Ide jon a kerdesre a valasz */

     rxidx  = 0;   // ovatosan, hátha hivas, SMS jon

     OsszefuzMSG2 (Van_PIN, Kerdes);      //Kell PINt megadni?
     M95re_kuld (Osszefuz);
     Delay_ms (5000);
      LCDre_kuld_adas (Van_PIN);
      LCDre_kuld_vetel (rxbuff);
     
     rxidx  = 0;  // ovatosan, hátha hivas, SMS jon

//     RC1IE_bit = 0;

     OsszefuzMSG3 (Van_PIN, Egyenlo, PIN);
     M95re_kuld (Osszefuz);
     Delay_ms (5000);

      LCDre_kuld_adas (Osszefuz);
      LCDre_kuld_vetel (rxbuff);

//     LCD_clear_row (2);
     rxidx  = 0;  // ovatosan, hátha hivas, SMS jon


    while(1){

         if (q != 200) {
            GSM_OK_LED = !GSM_OK_LED;
            Delay_ms (300);
            q++;
            }
         else {
            q = 0;
            M95re_kuld (Echo_off);
            
            GSM_OK_LED = !GSM_OK_LED;
            Delay_ms (300);
              }
            
            }
}