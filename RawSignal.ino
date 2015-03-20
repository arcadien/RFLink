#define NODO_PULSE_0                 500  // PWM: Tijdsduur van de puls bij verzenden van een '0' in uSec.#define NODO_PULSE_MID              1000  // PWM: Pulsen langer zijn '1'#define NODO_PULSE_1                1500  // PWM: Tijdsduur van de puls bij verzenden van een '1' in uSec. (3x NODO_PULSE_0)#define NODO_SPACE                   500  // PWM: Tijdsduur van de space tussen de bitspuls bij verzenden van een '1' in uSec.   /*********************************************************************************************\ * Deze routine zendt een RAW code via RF.  * De inhoud van de buffer RawSignal moet de pulstijden bevatten.  * RawSignal.Number het aantal pulsen*2 \*********************************************************************************************/void RawSendRF(void)  {  int x;  digitalWrite(PIN_RF_RX_VCC,LOW);                                              // Spanning naar de RF ontvanger uit om interferentie met de zender te voorkomen.  digitalWrite(PIN_RF_TX_VCC,HIGH);                                             // zet de 433Mhz zender aan  delay(TRANSMITTER_STABLE_TIME);                                               // kleine pauze om de zender de tijd te geven om stabiel te worden   // LET OP: In de Arduino versie 1.0.1 zit een bug in de funktie delayMicroSeconds(). Als deze wordt aangeroepen met een nul dan zal er  // een pause optreden van 16 milliseconden. Omdat het laatste element van RawSignal af sluit met een nul (omdat de space van de stopbit   // feitelijk niet bestaat) zal deze bug optreden. Daarom wordt deze op 1 gezet om de bug te omzeilen.   RawSignal.Pulses[RawSignal.Number]=1;  for(byte y=0; y<RawSignal.Repeats; y++)                                       // herhaal verzenden RF code    {    x=1;    noInterrupts();    while(x<RawSignal.Number)      {      digitalWrite(PIN_RF_TX_DATA,HIGH);      delayMicroseconds(RawSignal.Pulses[x++]*RawSignal.Multiply-5);            // min een kleine correctie        digitalWrite(PIN_RF_TX_DATA,LOW);      delayMicroseconds(RawSignal.Pulses[x++]*RawSignal.Multiply-7);            // min een kleine correctie      }    interrupts();    delay(RawSignal.Delay);// Delay buiten het gebied waar de interrupts zijn uitgeschakeld! Anders werkt deze funktie niet.    }  delay(TRANSMITTER_STABLE_TIME);                                               // kleine pause zodat de ether even schoon blijft na de stopbit  digitalWrite(PIN_RF_TX_VCC,LOW); // zet de 433Mhz zender weer uit  digitalWrite(PIN_RF_RX_VCC,HIGH); // Spanning naar de RF ontvanger weer aan.  #if NODO_MEGA  //Board specifiek: Genereer een korte puls voor omschakelen van de Aurel tranceiver van TX naar RX mode.  if((HW_Config&0xf) == BIC_HWMESH_NES_V1X)   {   delayMicroseconds(36);   digitalWrite(PIN_BSF_0,LOW);   delayMicroseconds(16);   digitalWrite(PIN_BSF_0,HIGH);   }  #endif  }/*********************************************************************************************\ * Deze routine zendt een RawSignal via IR.  * De inhoud van de buffer RawSignal moet de pulstijden bevatten.  * RawSignal.Number het aantal pulsen*2 * Pulsen worden verzonden op en draaggolf van 38Khz. * * LET OP: Deze routine is speciaal geschreven voor de Arduino Mega1280 of Mega2560 met een * klokfrequentie van 16Mhz. \*********************************************************************************************/void RawSendIR(void)  {  int pulse;                                                                    // pulse (bestaande uit een mark en een space) uit de RawSignal tabel die moet worden verzonden  int mod;                                                                      // pulsenteller van het 38Khz modulatie signaal    delay(10);                                                                    // kleine pause zodat verzenden event naar de USB poort gereed is, immers de IRQ's worden tijdelijk uitgezet    // LET OP: In de Arduino versie 1.0.1 zit een bug in de funktie delayMicroSeconds(). Als deze wordt aangeroepen met een nul dan zal er  // een pause optreden van 16 milliseconden. Omdat het laatste element van RawSignal af sluit met een nul (omdat de space van de stopbit   // feitelijk niet bestaat) zal deze bug optreden. Daarom wordt deze op 1 gezet om de bug te omzeilen.   RawSignal.Pulses[RawSignal.Number]=1;    for(int repeat=0; repeat<RawSignal.Repeats; repeat++)                         // herhaal verzenden IR code    {    pulse=1;    noInterrupts();    while(pulse<(RawSignal.Number))      {      // Mark verzenden. Bereken hoeveel pulsen van 26uSec er nodig zijn die samen de lengte van de mark/space zijn.      mod=(RawSignal.Pulses[pulse++]*RawSignal.Multiply)/26;                    // delen om aantal pulsen uit te rekenen      while(mod)        {        // Hoog        #if NODO_MEGA        bitWrite(PORTH,0, HIGH);        #else        bitWrite(PORTB,3, HIGH);        #endif        delayMicroseconds(12);        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");// per nop 62.6 nano sec. @16Mhz          // Laag        #if NODO_MEGA        bitWrite(PORTH,0, LOW);            #else        bitWrite(PORTB,3, LOW);        #endif        delayMicroseconds(12);        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");// per nop 62.6 nano sec. @16Mhz        mod--;        }      // Laag      delayMicroseconds(RawSignal.Pulses[pulse++]*RawSignal.Multiply);      }    interrupts(); // interupts weer inschakelen.      delay(RawSignal.Delay);// Delay buiten het gebied waar de interrupts zijn uitgeschakeld! Anders werkt deze funktie niet    }  }/*********************************************************************************************\ * Deze routine berekend de RAW pulsen van een Nodo event en plaatst deze in de buffer RawSignal * RawSignal.Bits het aantal pulsen*2+startbit*2 \*********************************************************************************************/// Definieer een datablock die gebruikt wordt voor de gegevens die via de ether verzonden moeten worden.struct DataBlockStruct  {  byte Version;  byte SourceUnit;  byte DestinationUnit;  byte Flags;  byte Type;  byte Command;  byte Par1;  unsigned long Par2;  byte Checksum;  };  void Nodo_2_RawSignal(struct NodoEventStruct *Event)  {  struct DataBlockStruct DataBlock;  byte BitCounter           = 1;  RawSignal.Repeats         = 1;                                                // 1 pulsenreeks. Nodo signaal heeft geen herhalingen  RawSignal.Delay           = 0;                                                // Geen repeats, dus delay tussen herhalingen ook niet relevant  RawSignal.Multiply        = 25;  Checksum(Event);  DataBlock.SourceUnit      = Event->SourceUnit | (HOME_NODO<<5);    DataBlock.DestinationUnit = Event->DestinationUnit;  DataBlock.Flags           = Event->Flags;  DataBlock.Type            = Event->Type;  DataBlock.Command         = Event->Command;  DataBlock.Par1            = Event->Par1;  DataBlock.Par2            = Event->Par2;  DataBlock.Checksum        = Event->Checksum;;  DataBlock.Version         = NODO_VERSION_MINOR;    byte *B=(byte*)&DataBlock;  // begin met een lange startbit. Veilige timing gekozen zodat deze niet gemist kan worden  RawSignal.Pulses[BitCounter++]=(NODO_PULSE_1 *2)/RawSignal.Multiply;   RawSignal.Pulses[BitCounter++]=(NODO_SPACE   *2)/RawSignal.Multiply;  for(byte x=0;x<sizeof(struct DataBlockStruct);x++)    {    for(byte Bit=0; Bit<=7; Bit++)      {      if((*(B+x)>>Bit)&1)        RawSignal.Pulses[BitCounter++]=NODO_PULSE_1/RawSignal.Multiply;       else        RawSignal.Pulses[BitCounter++]=NODO_PULSE_0/RawSignal.Multiply;         RawSignal.Pulses[BitCounter++]=NODO_SPACE/RawSignal.Multiply;                   }    }  RawSignal.Pulses[BitCounter-1]=NODO_SPACE/RawSignal.Multiply; // pauze tussen de pulsreeksen  RawSignal.Number=BitCounter;  }// De while() loop waar de statemask wordt getest doorloopt een aantal cycles per milliseconde. Dit is afhankelijk// van de kloksnelheid van de Arduino. Deze routine is in de praktijk geklokt met een processorsnelheid van// 16Mhz. Naast de doorlijktijd van de while() loop en er ook nog overhead die moet worden opgetelt bij de // uiteindelijk gemeten pulstijd. Tijden zijn in de praktijk uitgeklokt met een analyser, echter per arduino// kunnen er kleine verschillen optreden. Timings gemeten aan de IR_RX_DATA en RF_RX_DATA ingangen. Eigenschappen// van de ontvangers kunnen eveneens van invloed zijn op de pulstijden. const unsigned long LoopsPerMilli=345;const unsigned long Overhead=0;  /**********************************************************************************************\ * Haal de pulsen en plaats in buffer.  * bij de TSOP1738 is in rust is de uitgang hoog. StateSignal moet LOW zijn * bij de 433RX is in rust is de uitgang laag. StateSignal moet HIGH zijn *  \*********************************************************************************************/// Omdat deze routine tijdkritisch is halen we de gebruikte variabelen op globaal niveau// zodat ze niet bij iedere functie-call opnieuw geinitialiseerd hoeven te worden. dit scheelt // verwerkingstijd. Als er geen signaal is, neemt deze funktie (incl. de call) int RawCodeLength=0;unsigned long PulseLength=0;unsigned long numloops=0;unsigned long maxloops=0;boolean Ftoggle=false;uint8_t Fbit=0;uint8_t Fport=0;uint8_t FstateMask=0;boolean FetchSignal(byte DataPin, boolean StateSignal)  {  uint8_t Fbit = digitalPinToBitMask(DataPin);  uint8_t Fport = digitalPinToPort(DataPin);  uint8_t FstateMask = (StateSignal ? Fbit : 0);  if((*portInputRegister(Fport) & Fbit) == FstateMask)                          // Als er signaal is    {    // Als het een herhalend signaal is, dan is de kans groot dat we binnen hele korte tijd weer in deze    // routine terugkomen en dan midden in de volgende herhaling terecht komen. Daarom wordt er in dit    // geval gewacht totdat de pulsen voorbij zijn en we met het capturen van data beginnen na een korte     // rust tussen de signalen.Op deze wijze wordt het aantal zinloze captures teruggebracht.        if(RawSignal.Time)                                                          //  Eerst een snelle check, want dit bevindt zich in een tijdkritisch deel...      {          if(RawSignal.Repeats && (RawSignal.Time+SIGNAL_REPEAT_TIME)>millis())     // ...want deze check duurt enkele micro's langer!        {        // digitalWrite(PIN_WIRED_OUT_2,HIGH);                                  // DEBUG: Wired-2 hoog gedurende capturing data        PulseLength=micros()+SIGNAL_TIMEOUT*1000;                               // Wachttijd            while((RawSignal.Time+SIGNAL_REPEAT_TIME)>millis() && PulseLength>micros())          if((*portInputRegister(Fport) & Fbit) == FstateMask)             PulseLength=micros()+SIGNAL_TIMEOUT*1000;                while((RawSignal.Time+SIGNAL_REPEAT_TIME)>millis() &&  (*portInputRegister(Fport) & Fbit) != FstateMask);        // digitalWrite(PIN_WIRED_OUT_2,LOW);                                   // DEBUG: Wired-2 hoog gedurende capturing data        }      }    RawCodeLength=1;                                                            // We starten bij 1, dit om legacy redenen. Vroeger had element 0 een speciaal doel.    Ftoggle=false;    maxloops = SIGNAL_TIMEOUT * LoopsPerMilli;        // digitalWrite(PIN_WIRED_OUT_2,HIGH);                                      // DEBUG:Wired-2 hoog gedurende capturing data      do{                                                                         // lees de pulsen in microseconden en plaats deze in de tijdelijke buffer RawSignal      numloops = 0;      while(((*portInputRegister(Fport) & Fbit) == FstateMask) ^ Ftoggle)       // while() loop *A*        if(numloops++ == maxloops)          break;                                                                // timeout opgetreden        PulseLength=((numloops + Overhead)* 1000) / LoopsPerMilli;                // Bevat nu de pulslengte in microseconden      if(PulseLength<MIN_PULSE_LENGTH)        break;              Ftoggle=!Ftoggle;          RawSignal.Pulses[RawCodeLength++]=PulseLength/(unsigned long)(Settings.RawSignalSample); // sla op in de tabel RawSignal      }    while(RawCodeLength<RAW_BUFFER_SIZE && numloops<=maxloops);                 // Zolang nog ruimte in de buffer, geen timeout en geen stoorpuls      // digitalWrite(PIN_WIRED_OUT_2,LOW);                                       //DEBUG:      if(RawCodeLength>=MIN_RAW_PULSES)      {      ///digitalWrite(PIN_WIRED_OUT_4,HIGH);                                    // DEBUG: Een Spike op Wired-4 bevestigd een capture van een geldig signaal      RawSignal.RepeatChecksum=false;                                           //       RawSignal.Repeats=0;                                                      // Op dit moment weten we nog niet het type signaal, maar de variabele niet ongedefinieerd laten.      RawSignal.Multiply=Settings.RawSignalSample;                              // Ingestelde sample groote.      RawSignal.Number=RawCodeLength-1;                                         // Aantal ontvangen tijden (pulsen *2)      RawSignal.Pulses[RawSignal.Number]=0;                                     // Laatste element bevat de timeout. Niet relevant.      RawSignal.Time=millis();      // digitalWrite(PIN_WIRED_OUT_4,LOW);                                     // DEBUG:      return true;      }    else      RawSignal.Number=0;        }  return false;  }boolean AnalyzeRawSignal(struct NodoEventStruct *E)  {  ClearEvent(E);  boolean Result=false;    if(RawSignal_2_Nodo(E))                                                       // Is het een Nodo signaal?    {    // Als er een Nodo signaal is binnengekomen, dan weten we zeker dat er een Nodo in het landschap is die tijd nodig heeft om    // weer terug te schakelen naar de ontvangstmode. Dit kost (helaas) enige tijd. Zorg er voor dat er gedurende deze tijd    // even geen Nodo event wordt verzonden anders wordt deze vrijwel zeker gemist.    HoldTransmission=millis()+NODO_TX_TO_RX_SWITCH_TIME;    Result=true;    }  if(!Transmission_NodoOnly)    {    if(!Result && PluginCall(PLUGIN_RAWSIGNAL_IN,E,0))                          // Loop de devices langs. Indien een device dit nodig heeft, zal deze het rawsignal gebruiken en omzetten naar een geldig event.      Result=true;          if(!Result && RawSignal_2_32bit(E))                                         // als er geen enkel geldig signaaltype uit de pulsenreeks kon worden gedestilleerd, dan resteert niets anders dan deze weer te geven als een RawSignal.      {      if(Settings.RawSignalReceive==VALUE_ON                                    // Signaal event als de setting RawSignalReceive op On staat      #if NODO_MEGA      || RawSignalExist(E->Par2)                                                // of het een bekend RawSignal is dat is opgeslagen op SDCard.      #endif        )        Result=true;      }    }      return Result;  }/**********************************************************************************************\ * Deze functie genereert uit een willekeurig gevulde RawSignal afkomstig van de meeste  * afstandsbedieningen een (vrijwel) unieke bit code. * Zowel breedte van de pulsen als de afstand tussen de pulsen worden in de berekening * meegenomen zodat deze functie geschikt is voor PWM, PDM en Bi-Pase modulatie. * LET OP: Het betreft een unieke hash-waarde zonder betekenis van waarde. \*********************************************************************************************/boolean RawSignal_2_32bit(struct NodoEventStruct *event)  {  int x;  unsigned int MinPulse=0xffff;  unsigned int MinSpace=0xffff;  unsigned long CodeM=0L;  unsigned long CodeS=0L;  // In enkele gevallen is uitzoeken van het RawSignal zinloos  if(RawSignal.Number < MIN_RAW_PULSES) return false;      // zoek de kortste tijd (PULSE en SPACE). Start niet direct vanaf de eerste puls omdat we anders kans   // lopen een onvolledige startbit te pakken. Ook niet de laatste, want daar zit de niet bestaande  // space van de stopbit in.  for(x=5;x<RawSignal.Number-2;x+=2)    {    if(RawSignal.Pulses[x]  < MinPulse)MinPulse=RawSignal.Pulses[x]; // Zoek naar de kortste pulstijd.    if(RawSignal.Pulses[x+1]< MinSpace)MinSpace=RawSignal.Pulses[x+1]; // Zoek naar de kortste spacetijd.    }  // De kortste pulsen zijn gevonden. Dan een 'opslag' zodat alle korte pulsen er royaal  // onder vallen maar niet de lengte van een lange puls passeren.  MinPulse+=(MinPulse*RAWSIGNAL_TOLERANCE)/100;  MinSpace+=(MinSpace*RAWSIGNAL_TOLERANCE)/100;  // Data kan zowel in de mark als de space zitten. Daarom pakken we beide voor data opbouw.  for(x=3;x<=RawSignal.Number;x+=2)    {    CodeM = (CodeM<<1) | (RawSignal.Pulses[x]   > MinPulse);    CodeS = (CodeS<<1) | (RawSignal.Pulses[x+1] > MinSpace);        }  // Data kan zowel in de mark als de space zitten. We nemen de grootste waarde voor de data.  if(CodeM > CodeS)      event->Par2=CodeM;  else    event->Par2=CodeS;  event->SourceUnit=0;    event->DestinationUnit=0;  event->Type=NODO_TYPE_RAWSIGNAL;  event->Command=EVENT_RAWSIGNAL;  event->Par1=0;  RawSignal.Repeats        = 1;                                                 // het is een herhalend signaal. Bij ontvangst herhalingen onderdukken   #if NODO_MEGA  RawSignal.RepeatChecksum = Settings.RawSignalChecksum==VALUE_ON;              // Moet het signaal moet minimaal tweemaal voorbij komen om een geldig event te zijn?  #else  RawSignal.RepeatChecksum = false;  #endif        return true;  }/*********************************************************************************************\ * Deze routine berekent de uit een RawSignal een NODO code * Geeft een false retour als geen geldig NODO signaal \*********************************************************************************************/boolean RawSignal_2_Nodo(struct NodoEventStruct *Event)  {  byte b,x,y,z;  if(RawSignal.Number!=16*sizeof(struct DataBlockStruct)+2)                     // Per byte twee posities + startbit.    return false;      struct DataBlockStruct DataBlock;  byte *B=(byte*)&DataBlock;                                                    // B wijst naar de eerste byte van de struct  z=3;                                                                          // RawSignal pulse teller: 0=niet gebruiktaantal, 1=startpuls, 2=space na startpuls, 3=1e pulslengte. Dus start loop met drie.  for(x=0;x<sizeof(struct DataBlockStruct);x++)                                 // vul alle bytes van de struct     {    b=0;    for(y=0;y<=7;y++)                                                           // vul alle bits binnen een byte      {      if((RawSignal.Pulses[z]*RawSignal.Multiply)>NODO_PULSE_MID)              b|=1<<y;                                                                // LSB in signaal wordt als eerste verzonden      z+=2;      }    *(B+x)=b;    }  if(DataBlock.SourceUnit>>5!=HOME_NODO)    return false;  RawSignal.Repeats    = 0;                                                     // het is geen herhalend signaal. Bij ontvangst hoeven herhalingen dus niet onderdrukt te worden.  Event->SourceUnit=DataBlock.SourceUnit&0x1F;                                  // Maskeer de bits van het Home adres.  Event->DestinationUnit=DataBlock.DestinationUnit;  Event->Flags=DataBlock.Flags;  Event->Type=DataBlock.Type;  Event->Command=DataBlock.Command;  Event->Par1=DataBlock.Par1;  Event->Par2=DataBlock.Par2;  Event->Version=DataBlock.Version;  Event->Checksum=DataBlock.Checksum;  if(Checksum(Event))    return true;  return false;   }#if NODO_MEGA/*********************************************************************************************\ * Kijk of voor de opgegeven Hex-event (Code) een rawsignal file op de SDCard bestaat. * Als deze bestaat dan return met 'true' \*********************************************************************************************/boolean RawSignalExist(unsigned long Code)  {  boolean exist=false;    SelectSDCard(true);    File dataFile=SD.open(PathFile(ProgmemString(Text_08),int2strhex(Code)+2,"DAT"));  if(dataFile)     {    exist=true;    dataFile.close();    }    SelectSDCard(false);  return exist;  } /*********************************************************************************************\ * Sla de pulsen in de buffer Rawsignal op op de SDCard \*********************************************************************************************/byte RawSignalWrite(unsigned long Key)  {  byte error=false;  char *TempString=(char*)malloc(INPUT_LINE_SIZE);  int x;    SelectSDCard(true);                                                           // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer SDCard chip    strcpy(TempString, PathFile(ProgmemString(Text_08),int2strhex(Key)+2,"DAT")); // Sla Raw-pulsenreeks op in bestand met door gebruiker gekozen nummer als filenaam  SD.remove(TempString);                                                        // eventueel bestaande file wissen, anders wordt de data toegevoegd.      File KeyFile = SD.open(TempString, FILE_WRITE);    RawSignalCleanUp(Settings.RawSignalCleanUp);  if(KeyFile)     {                                                                               sprintf(TempString,"! %s %s\n",cmd2str(EVENT_RAWSIGNAL), int2strhex(Key));        KeyFile.write((uint8_t*)TempString,strlen(TempString));                     // Sla de RawSignal op        sprintf(TempString,"! Pulses %s\n", int2str(RawSignal.Number));        KeyFile.write((uint8_t*)TempString,strlen(TempString));                     // Sla de Pulses op        if(RawSignal.Repeats==0)RawSignal.Repeats=RAWSIGNAL_TX_REPEATS;                 sprintf(TempString,"%s %d\n",cmd2str(CMD_RAWSIGNAL_REPEATS), RawSignal.Repeats);        KeyFile.write((uint8_t*)TempString,strlen(TempString));                     // Sla de repeats op                if(Settings.RawSignalCleanUp!=0)      {      sprintf(TempString,"!%s %d\n",cmd2str(CMD_RAWSIGNAL_CLEANUP), Settings.RawSignalCleanUp);          KeyFile.write((uint8_t*)TempString,strlen(TempString));                   // Sla de CleanUp op      }                      if(RawSignal.Delay==0)RawSignal.Delay=RAWSIGNAL_TX_DELAY;    sprintf(TempString,"%s %d\n",cmd2str(CMD_RAWSIGNAL_DELAY), RawSignal.Delay);        KeyFile.write((uint8_t*)TempString,strlen(TempString));                     // Sla de delay op.    // Sla de multiply/sample op.    if(RawSignal.Multiply==0)RawSignal.Multiply=Settings.RawSignalSample;    sprintf(TempString,"%s %d\n",cmd2str(CMD_RAWSIGNAL_SAMPLE), RawSignal.Multiply);        KeyFile.write((uint8_t*)TempString,strlen(TempString));    // Geef de pulstijden weer    for(x=1;x<=RawSignal.Number;x++)      {      if(x%10==1)        sprintf(TempString,"%s %d:",cmd2str(CMD_RAWSIGNAL_PULSES), x-1);          else        strcat(TempString,",");              strcat(TempString,int2str(RawSignal.Pulses[x]*RawSignal.Multiply));             if(x%10==0 || x==RawSignal.Number)        {        KeyFile.write((uint8_t*)TempString,strlen(TempString));        KeyFile.write('\n');        }            }    KeyFile.close();        PrintString(ProgmemString(Text_22), VALUE_ALL);    FileShow(ProgmemString(Text_08),int2strhex(Key)+2,"DAT", VALUE_ALL);    PrintString(ProgmemString(Text_22), VALUE_ALL);    }  else     error=MESSAGE_UNABLE_OPEN_FILE;  // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer W5100 chip  SelectSDCard(false);  free(TempString);  return error;  }  /*********************************************************************************************\ * Deze funktie vult de struct RawSignal met de opgegeven pulstijden. De string dient het volgende * format te hebben: <Startadres>:<Pulstijd>,<Pulstijd>,<Pulstijd>,<Pulstijd>,... * Startadres en Pulstijd mogen zowel decimaal als hexadecimaal zijn. Zodra er niet wordt * voldaan aan het string format wordt teruggekeerd met een error.    *  * De waarde RawSignal.Multiply moet een geldige waarde bevatten. \*********************************************************************************************/byte RawSignalPulses(char* Line)  {  // Zoek naar eerste getal tot aan ':' teken. Dat is de index waar de rawsignal begint.  byte error=0;  int a,w,x=0,y=0;  char *TmpStr2=(char*)malloc(INPUT_LINE_SIZE);    // Zoek naar eerste getal tot aan ':' teken. Dat is de index waar de rawsignal begint.  y=0;  do    {    w=Line[x];    if(isxdigit(w) && y<(INPUT_LINE_SIZE-1)) // zowel decimaal als hex mogen      TmpStr2[y++]=w;    x++;    }while(w!=0 && w!=':');  if(w==':')    {                  TmpStr2[y]=0;    RawSignal.Number=str2int(TmpStr2);    y=0;    while(w!=0 && !error)      {      w=Line[x++];      if(isxdigit(w) && y<(INPUT_LINE_SIZE-1)) // zowel decimaal als hex mogen.        TmpStr2[y++]=w;      else if(w==' ');      else if(w==',' || w==0)        {        TmpStr2[y]=0;        y=0;        a=str2int(TmpStr2);        if(++RawSignal.Number<RAW_BUFFER_SIZE && a<=(255 * RawSignal.Multiply))          RawSignal.Pulses[RawSignal.Number]=a/RawSignal.Multiply;        else          error=MESSAGE_INVALID_PARAMETER;                    }      else        error=MESSAGE_INVALID_PARAMETER;                                  }    }  else    error=MESSAGE_INVALID_PARAMETER;              free(TmpStr2);  return error;  }boolean RawSignalCleanUp(byte MaxPulseWidthCount)  {  int x,y;    const int MaxCount=5;  unsigned int MaxTime[MaxCount];  unsigned int MinTime[MaxCount];  unsigned int Time=0;  if(MaxPulseWidthCount>MaxCount || MaxPulseWidthCount<2)    return 0;    // Schoon de tabellen.  for(x=0;x<MaxCount;x++)    {    MaxTime[x]=0;    MinTime[x]=0;    }      MinTime[0]=10000;  for(y=0;y<MaxPulseWidthCount;y++)    {    // zoek langste pulstijd  kleiner dan de vorige hoogste pulstijd     for(x=1;x<RawSignal.Number;x++)      {      Time=RawSignal.Pulses[x];      if(Time > MaxTime[y] && Time<MinTime[y])        MaxTime[y]=Time;      }        if(MaxTime[y]==0)      break;          //Serial.print(F("MaxTime = "));Serial.print(MaxTime[y] *RawSignal.Multiply);            // Zoek naar de langste pulstijd die maximaal [afwijking] kleiner is.    MinTime[y]=MaxTime[y];    for(x=1;x<RawSignal.Number;x++)      {      Time=RawSignal.Pulses[x];      if(Time < MinTime[y] && Time>(MaxTime[y]-(MaxTime[y]/5)) && Time<MaxTime[y]) // tot 20% kleinere pulsen horen er nog bij        MinTime[y]=Time;      }        // Serial.print(F(", MinTime = "));Serial.print(MinTime[y] *RawSignal.Multiply);    // Serial.print(F(", Gemiddelde puls  = "));Serial.println(((MinTime[y]+MaxTime[y])/2)*RawSignal.Multiply);    if(y<MaxCount)      {      MinTime[y+1]=MinTime[y]-(MinTime[y]/5);      MaxTime[y+1]=0;      }    }  // Schoon de pulsenreeks voor de gevonden gemiddelde waarde  for(x=1;x<RawSignal.Number;x++)    {    Time=RawSignal.Pulses[x];    if       (Time >  MaxTime[1]                     ) RawSignal.Pulses[x]=MaxTime[0];    else if  (Time >  MaxTime[2] && Time < MinTime[0]) RawSignal.Pulses[x]=MaxTime[1];    else if  (Time >  MaxTime[3] && Time < MinTime[1]) RawSignal.Pulses[x]=MaxTime[2];    else if  (Time >  MaxTime[4] && Time < MinTime[2]) RawSignal.Pulses[x]=MaxTime[3];    else if  (                      Time < MinTime[3]) RawSignal.Pulses[x]=MaxTime[4];    }  return y;  }#endif