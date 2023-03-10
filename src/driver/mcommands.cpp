/*
 * Работа с драйвером силовой платы
 * read  (get) - чтение через драйвер
 * write (set) - запись через драйвер
 * 01.02.2023, некоторые не используемые команды не поддерживаются
 */

#include "driver/mcommands.h"
#include "nvs.h"
#include "board/mboard.h"
#include "mtools.h"
#include "mcmd.h"
#include "mwake.h"
#include "stdint.h"
#include <Arduino.h>


MCommands::MCommands(MTools * tools) : Tools(tools), Board(tools->Board) 
{ 
  Wake = new MWake(); 
  Wake->wakeInit( 0x00, 50 );  // Адрес в сети и время ожидания ответа (ms)
}

MCommands::~MCommands()
{
  delete Wake;
}

uint8_t cmd = MCmd::cmd_nop;

void MCommands::doCommand()
{
  cmd = Tools->getBuffCmd();

  #ifdef WO_VIS
    Tools->setBuffCmd(MCmd::cmd_nop);
  #else
    Tools->getTuningAdc() ? Tools->setBuffCmd(MCmd::cmd_adc_read_probes) : Tools->setBuffCmd(MCmd::cmd_get_uis);
  #endif

  if( cmd != MCmd::cmd_nop)
  {
    #ifdef DEBUG_COMMANDS
      Serial.print(" command -> 0x"); Serial.println(cmd, HEX);
    #endif

    switch( cmd )
    {
      //Команды чтения результатов измерений
      case MCmd::cmd_get_uis:                 doGetUIS();                 break;  // 0x10
      case MCmd::cmd_get_u:                   doGetU();                   break;  // 0x11 Чтение напряжения (мВ)
      case MCmd::cmd_get_i:                   doGetI();                   break;  // 0x12 Чтение тока (мА)
      case MCmd::cmd_get_ui:                  doGetUI();                  break;  // 0x13 Чтение напряжения (мВ) и тока (мА)
      case MCmd::cmd_get_state:               doGetState();               break;  // 0x14 Чтение состояния
      //case MCmd::cmd_get_celsius:             doGetCelsius();             break;  // 0x15 Чтение температуры радиатора
      case MCmd::cmd_ready:             doReady();             break;  // 0x15 Параметры согласованы

        // Команды управления
      case MCmd::cmd_power_auto:              doPowerAuto();              break;  // 0x20
      case MCmd::cmd_power_stop:              doPowerStop();              break;  // 0x21
      case MCmd::cmd_power_mode:              doPowerMode();              break;  // 0x22
      case MCmd::cmd_discharge_go:            doDischargeGo();            break;  // 0x24
      // case MCmd::cmd_power_go_mode:           doPowerGoMode();            break;  // 0x25

        // Команды работы с измерителем напряжения 
      case MCmd::cmd_read_factor_u:           doGetFactorU();             break;  // 0x30
      case MCmd::cmd_write_factor_u:          doSetFactorU();             break;  // 0x31
      case MCmd::cmd_write_factor_default_u:  doSetFactorDefaultU();      break;  // 0x32
      case MCmd::cmd_read_smooth_u:           doGetSmoothU();             break;  // 0x33
      case MCmd::cmd_write_smooth_u:          doSetSmoothU();             break;  // 0x34
      case MCmd::cmd_read_offset_u:           doGetOffsetU();             break;  // 0x35
      case MCmd::cmd_write_offset_u:          doSetOffsetU();             break;  // 0x36
      
        // Команды работы с измерителем тока
      case MCmd::cmd_read_factor_i:             doGetFactorI();           break;  // 0x38
      case MCmd::cmd_write_factor_i:            doSetFactorI();           break;  // 0x39
      case MCmd::cmd_write_factor_default_i:    doSetFactorDefaultI();    break;  // 0x3A
      case MCmd::cmd_read_smooth_i:             doGetSmoothI();           break;  // 0x3B
      case MCmd::cmd_write_smooth_i:            doSetSmoothI();           break;  // 0x3C
      case MCmd::cmd_read_offset_i:             doGetOffsetI();           break;  // 0x3D
      case MCmd::cmd_write_offset_i:            doSetOffsetI();           break;  // 0x3E

        // Команды работы с ПИД-регулятором
      case MCmd::cmd_pid_configure:             doPidConfigure();         break;  // 0x40
      case MCmd::cmd_pid_write_coefficients:    doPidSetCoefficients();   break;  // 0x41
      case MCmd::cmd_pid_output_range:          doPidOutputRange();       break;  // 0x42
      case MCmd::cmd_pid_reconfigure:           doPidReconfigure();       break;  // 0x43
      case MCmd::cmd_pid_clear:                 doPidClear();             break;  // 0x44
      case MCmd::cmd_pid_test:                  doPidTest();              break;  // 0x46
      case MCmd::cmd_pid_read_treaty:           doPidGetTreaty();         break;  // 0x47
      case MCmd::cmd_pid_read_configure:        doPidGetConfigure();      break;  // 0x48
      // case MCmd::cmd_pid_write_max_sum:        doPidSetMaxSum();         break;  // 0x49  ?
      //case MCmd::cmd_pid_write_treaty:          doPidSetTreaty();         break;  // 0x4A (резерв)
      case MCmd::cmd_pid_write_frequency:       doPidSetFrequency();      break;  // 0x4A Задание частоты pid-регулятора

        // Команды работы с АЦП
      case MCmd::cmd_adc_read_probes:           doReadProbes();           break;  // 0x50
      case MCmd::cmd_adc_read_offset:           doAdcGetOffset();         break;  // 0x51
      case MCmd::cmd_adc_write_offset:          doAdcSetOffset();         break;  // 0x52
      case MCmd::cmd_adc_auto_offset:           doAdcAutoOffset();        break;  // 0x53 na


        // Команды управления тестовые
      case MCmd::cmd_write_switch_pin:          doSwPin();                break;  // 0x54 na
      case MCmd::cmd_write_power:               doSetPower();             break;  // 0x56 na
      case MCmd::cmd_write_discharge:           doSetDischg();            break;  // 0x57 na
      case MCmd::cmd_write_voltage:             doSetVoltage();           break;  // 0x58 na
      case MCmd::cmd_write_current:             doSetCurrent();           break;  // 0x59 na
      case MCmd::cmd_write_discurrent:          doSetDiscurrent();        break;  // 0x5A na
  //   case MCmd::cmd_write_surge_compensation:  doSurgeCompensation();     break;  // 0x5B   nu
      case MCmd::cmd_write_idle_load:           doIdleLoad();             break;  // 0x5C na   

        // Команды задания порогов отключения
      case MCmd::cmd_get_lt_v:                  doGetLtV();               break;  // 0x60
      case MCmd::cmd_set_lt_v:                  doSetLtV();               break;  // 0x61
      case MCmd::cmd_set_lt_default_v:          doSetLtDefaultV();        break;  // 0x62
      case MCmd::cmd_get_up_v:                  doGetUpV();               break;  // 0x63
      case MCmd::cmd_set_up_v:                  doSetUpV();               break;  // 0x64
      case MCmd::cmd_set_up_default_v:          doSetUpDefaultV();        break;  // 0x65

      case MCmd::cmd_get_lt_i:                  doGetLtI();               break;  // 0x68
      case MCmd::cmd_set_lt_i:                  doSetLtI();               break;  // 0x69
      case MCmd::cmd_set_lt_default_i:          doSetLtDefaultI();        break;  // 0x6A
      case MCmd::cmd_get_up_i:                  doGetUpI();               break;  // 0x6B
      case MCmd::cmd_set_up_i:                  doSetUpI();               break;  // 0x6C
      case MCmd::cmd_set_up_default_i:          doSetUpDefaultI();        break;  // 0x6D

        // Команды универсальные
      case MCmd::cmd_nop:                       doNop();                  break;  // 0x00
      case MCmd::cmd_info:                      doInfo();                 break;  // 0x03

      default: 
      break ;
    }
    cmd = MCmd::cmd_nop;                                                          // не обязательно
  }

}

  // Обработка принятого пакета
short MCommands::dataProcessing()
{
  Wake->wakeRead();
  int cmd = Wake->getCommand();                         // Код команды в ответе
//  Serial.print("cmd=0x");   Serial.println( cmd, HEX );

  switch(cmd)
  {
    // Ответ на команду чтения результатов измерения напряжения (мВ), тока (мА) и
    // двух байт состояния драйвера (всего 7 байт, включая байт ошибки)
    case MCmd::cmd_get_uis:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 7) )
      {
        
        //Tools->setVoltageVolt(Wake->get16(1));
        Tools->setMilliVolt(Wake->get16(1));

        //Tools->setCurrentAmper(Wake->get16(3));
        Tools->setMilliAmper(Wake->get16(3));

        Tools->setState(Wake->get16(5));
        return 0; //Tools->setProtErr(0);
      }
      else  return 1; //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

    // Ответ на команду чтения результата измерения напряжения (мВ)
    // всего 3 байта, включая байт ошибки
    case MCmd::cmd_get_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->setVoltageVolt(Wake->get16(1));
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

    // Ответ на команду чтения результатов измерения тока (мА)
    // всего 3 байта, включая байт ошибки
    case MCmd::cmd_get_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->setCurrentAmper(Wake->get16(1));
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

    // Ответ на команду чтения результатов измерения напряжения (мВ), тока (мА)
    // всего 5 байт, включая байт ошибки
    case MCmd::cmd_get_ui:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 5) )
      {
        Tools->setVoltageVolt(Wake->get16(1));
        Tools->setCurrentAmper(Wake->get16(3));
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

    // Ответ на команду чтения двух байт состояния драйвера (всего 3 байта, включая байт ошибки)
    case MCmd::cmd_get_state:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->setState(Wake->get16(1));
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

    // // Ответ на команду чтения результата преобразования данных датчика температуры
    // // всего 3 байта, включая байт ошибки
    // case MCmd::cmd_get_celsius:
    //   if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
    //   {
    //     Tools->setCelsius(Wake->get16(1));
    //     return 0;  //Tools->setProtErr(0);
    //   }
    //   else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    // break;

      // Ответ на команду старта преобразователя с заданными максимальными V и I
      // (всего 5 байт, включая байт ошибки)
    case MCmd::cmd_power_auto:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 5) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола - или нет подтверждения исполнения команды 
    break;

      // Ответ на команду отключения преобразователя и цепи разряда
      // (всего 1 байт - байт ошибки)
    case MCmd::cmd_power_stop:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола - или нет подтверждения исполнения команды 
    break;


      // Ответ на команду старта преобразователя с заданными максимальными V и I
      // (всего 5 байт, включая байт ошибки)
    case MCmd::cmd_power_mode:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 5) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола - или нет подтверждения исполнения команды 
    break;


      // ========= Обработка ответов на команды работы с измерителем напряжения =========
      // Чтение множителя преобразования в милливольты           0x30
    case MCmd::cmd_read_factor_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->factorV = Wake->get16(1);
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Запись множителя преобразования в милливольты           0x31
    case MCmd::cmd_write_factor_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Возврат к заводскому множителю                         0x32
    case MCmd::cmd_write_factor_default_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Чтение параметра сглаживания                           0x33
    case MCmd::cmd_read_smooth_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 2) )
      {
        Tools->smoothV = Wake->get08(1);
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Запись параметра сглаживания                           0x34
    case MCmd::cmd_write_smooth_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      //  Чтение приборного смещения                            0x35
    case MCmd::cmd_read_offset_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->shiftV = (float)Wake->get16(1) / 1000;
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      //  Запись приборного смещения                            0x36
    case MCmd::cmd_write_offset_u:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // ========= Обработка ответов на команды работы с измерителем тока =========
      // Чтение множителя преобразования в миллиамперы           0x38
    case MCmd::cmd_read_factor_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->factorI = Wake->get16(1);
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Запись множителя преобразования в миллиамперы           0x39
    case MCmd::cmd_write_factor_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Возврат к заводскому множителю                         0x3A
    case MCmd::cmd_write_factor_default_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Чтение параметра сглаживания                           0x3B
    case MCmd::cmd_read_smooth_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 2) )
      {
        Tools->smoothI = Wake->get08(1);
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Запись параметра сглаживания                           0x3C
    case MCmd::cmd_write_smooth_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      //  Чтение приборного смещение                            0x3D
    case MCmd::cmd_read_offset_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->shiftI = Wake->get16(1);
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      //  Запись приборного смещения                            0x3E
    case MCmd::cmd_write_offset_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // ================ Команды работы с ПИД-регулятором =================
      // Параметры не возвращаются, только подтверждение исполнения команды
    case MCmd::cmd_pid_configure:               // 0x40   + 0B->01
    case MCmd::cmd_pid_write_coefficients:      // 0x41   + 07->01
    case MCmd::cmd_pid_output_range:            // 0x42   + 05->01
    case MCmd::cmd_pid_reconfigure:             // 0x43   + 0B->01
    case MCmd::cmd_pid_clear:                   // 0x44   + 01->01
    case MCmd::cmd_pid_test:                    // 0x46   + 03->01
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Чтение параметров обмена с ПИД-регулятором
      // и расчет множителя и максимума параметров
    case MCmd::cmd_pid_read_treaty:                              // 0x47
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 7) )
      {
        unsigned short shift = Wake->get16(1);
        unsigned short bits  = Wake->get16(3);
        unsigned short hz    = Wake->get16(5);
          // вычисления и запись
        Tools->pMult = Tools->calkPMult(shift, bits);
        Tools->pMax  = Tools->calkPMax(shift, bits);
        //Tools->pHz   = Tools->calkPHz(hz);
        Tools->pidHz   = Tools->calkPHz(hz);
  #ifdef DEBUG_TREATY
    Serial.print("shift: 0x");  Serial.println(shift, HEX);
    Serial.print("bits: 0x");   Serial.println(bits, HEX);
    Serial.print("hz: 0x");     Serial.println(hz, HEX);
    Serial.print("pMult = 0x"); Serial.println(Tools->pMult, HEX);
    Serial.print("pMax = 0x");  Serial.println(Tools->pMax, HEX);
    Serial.print("pHz = 0x");   Serial.println(Tools->pidHz, HEX);
  #endif
        return 0;                     // Подтверждение
      }
      else  return 1;                 // ошибка протокола  
    break;

      // Чтение настроек обмена ПИД-регулятора                  // 0x48 (не используется, резерв)
    case MCmd::cmd_pid_read_configure:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 12) )
      {
        Tools->pidMode  = Wake->get08(1);
        Tools->kp       = Wake->get16(2);
        Tools->ki       = Wake->get16(4);
        Tools->kd       = Wake->get16(6);
        Tools->minOut   = Wake->get16(8);
        Tools->maxOut   = Wake->get16(10);
        return 0;  //Tools->setProtErr(0);                      // Подтверждение
      }
      else  return 1;  //Tools->setProtErr(1);                  // ошибка
    break;

      // case cmd_pid_write_max_sum:         doPidSetMaxSum();           break;  // 0x49   + 0?->0?


  //constexpr uint8_t cmd_pid_write_frequency       = 0x4A; //Запись частоты pid-регулятора
    case MCmd::cmd_pid_write_frequency:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;
      }
      else  return 1;  // ошибка протокола или нет подтверждения исполнения команды 
    break;


      // ================ Команды работы с АЦП =================
      // Чтение АЦП                                        0x50   + 00->07
    case MCmd::cmd_adc_read_probes:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 7) )
      {
        Tools->setAdcV(Wake->get16(1));
        Tools->setAdcI(Wake->get16(3));
        // состояние
        // состояние
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Чтение смещения АЦП                               0x51   + 00->03
    case MCmd::cmd_adc_read_offset:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->txSetAdcOffset(Wake->get16(1));
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;

      // Запись смещения АЦП                                0x52   + 02->01
    case MCmd::cmd_adc_write_offset:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 1) )
      {
        return 0;  //Tools->setProtErr(0);
      }
      else  return 1;  //Tools->setProtErr(1);  // ошибка протокола или нет подтверждения исполнения команды 
    break;





        // Команды чтения и записи порогов отключения по напряжению
        // Нижний порог отключения, чтение                  0x60
    case MCmd::cmd_get_lt_v:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->txSetLtV(Wake->get16(1));
        return 0;
      }
      else return 1;
      break;

        // Нижний порог отключения, запись                  0x61
    case MCmd::cmd_set_lt_v:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

        // Нижний порог отключения, запись дефолтного       0x62    
    case MCmd::cmd_set_lt_default_v:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

        // Верхний порог отключения, чтение                 0x63
    case MCmd::cmd_get_up_v:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->txSetUpV(Wake->get16(1));
        return 0;
      }
      else return 1;
      break;

        // Верхний порог отключения, запись                 0x64
    case MCmd::cmd_set_up_v:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

        // Верхний порог отключения, запись дефолтного      0x65    
    case MCmd::cmd_set_up_default_v:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

        // Команды чтения и записи порогов отключения по току
        // Нижний порог отключения, чтение                  0x68
    case MCmd::cmd_get_lt_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->txSetLtI(Wake->get16(1));
        return 0;
      }
      else return 1;
      break;

        // Нижний порог отключения, запись                  0x69
    case MCmd::cmd_set_lt_i:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

        // Нижний порог отключения, запись дефолтного       0x6A    
    case MCmd::cmd_set_lt_default_i:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

        // Верхний порог отключения, чтение                 0x6B
    case MCmd::cmd_get_up_i:
      if( (Wake->get08(0) == 0) && (Wake->getNbt() == 3) )
      {
        Tools->txSetUpV(Wake->get16(1));
        return 0;
      }
      else return 1;
      break;

        // Верхний порог отключения, запись                 0x6C
    case MCmd::cmd_set_up_i:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

        // Нижний порог отключения, запись дефолтного       0x6D    
    case MCmd::cmd_set_up_default_i:
      if((Wake->get08(0) == 0) && (Wake->getNbt() == 1))  return 0;
      else  return 1;
      break;

    default:
    return 2;   // Нет такой команды
    break;
  }
}

// Запись байта в буфер передатчика по индексу 
void MCommands::txU08(uint8_t id,  uint8_t value)
{
  Wake->setU8( id, value );
}

// Запись двухбайтового числа в буфер передатчика по индексу 
void MCommands::txU16(uint8_t id, uint16_t value)
{
  Wake->setU8( id,   uint8_t(( value >>  8 ) & 0xff ));
  Wake->setU8( id+1, uint8_t(  value         & 0xff ));
}

// Запись четырехбайтового числа в буфер передатчика по индексу 
void MCommands::txU32(uint8_t id, uint32_t value)
{
  Wake->setU8( id,   uint8_t(( value >> 24 ) & 0xff ));
  Wake->setU8( id+1, uint8_t(( value >> 16 ) & 0xff ));
  Wake->setU8( id+2, uint8_t(( value >>  8 ) & 0xff ));
  Wake->setU8( id+3, uint8_t(  value         & 0xff ));
}

//================= Команды управления процессами =================

// Команда запроса данных измерений драйвером напряжения и тока
// Ожидаемый ответ: целочисленные знаковые в милливольтах и миллиамперах и два байта состояний.
void MCommands::doGetUIS()
{        
  Wake->configAsk( 0, MCmd::cmd_get_uis);
}

// 0x11 Чтение напряжения (мВ)
// Ожидаемый ответ: целочисленное знаковое в милливольтах.
void MCommands::doGetU()
{
  Wake->configAsk( 0, MCmd::cmd_get_u);
}

// 0x12 Чтение тока (мА)
// Ожидаемый ответ: целочисленное знаковое в миллиамперах.
void MCommands::doGetI()
{
  Wake->configAsk( 0, MCmd::cmd_get_i);
}

// 0x13 Чтение напряжения (мВ) и тока (мА)
// Ожидаемый ответ: целочисленные знаковые в милливольтах и миллиамперах.
void MCommands::doGetUI()
{
  Wake->configAsk( 0, MCmd::cmd_get_ui);
}

// 0x14 Чтение состояния
// Ожидаемый ответ: два байта состояний.
void MCommands::doGetState()
{
  Wake->configAsk( 0, MCmd::cmd_get_state);
}

// Команда параметры согласованы 0x15
void MCommands::doReady()
{
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_ready);
}

// Команда управления PID регулятором 0x20
void MCommands::doPowerAuto()
{
  int id = 0;
  id = Wake->replyU16( id, Tools->setpointU );
  id = Wake->replyU16( id, Tools->setpointI );
//  id = Wake->replyU08( id, Tools->pidMode );
  Wake->configAsk( id, MCmd::cmd_power_auto);
}

// Команда отключения регулятора  0x21 
void MCommands::doPowerStop()     {Wake->configAsk( 0, MCmd::cmd_power_stop);}

// Команда управления PID регулятором 0x22
void MCommands::doPowerMode()
{
  int id = 0;
  id = Wake->replyU16( id, Tools->setpointU );
  id = Wake->replyU16( id, Tools->setpointI );
  id = Wake->replyU08( id, Tools->pidMode );
  Wake->configAsk( id, MCmd::cmd_power_mode);
}

// Команда управления PID-регулятором разряда 0x24
void MCommands::doDischargeGo()
{
  int id = 0;
  id = Wake->replyU16( id, Tools->setpointI );
  Wake->configAsk( id, MCmd::cmd_discharge_go);
}

//  // 0x25
// void MCommands::doPowerGoMode()
// {
//   int id = 0;
//   id = Wake->replyU16( id, Tools->setpointU );
//   id = Wake->replyU16( id, Tools->setpointI );
//   id = Wake->replyU08( id, Tools->pidMode );
//   Wake->configAsk( id, MCmd::cmd_power_go_mode);
// }

// =============== Команды работы с измерителем напряжения ================
// Команда чтения множителя по напряжению 0x30 
void MCommands::doGetFactorU() 
{
  Wake->configAsk( 0, MCmd::cmd_read_factor_u);
  // ...
}

// Команда записи множителя по напряжению 0x31 (0x0123)
void MCommands::doSetFactorU() 
{
  int id = 0;
  id = Wake->replyU16( id, Tools->factorV );
  Wake->configAsk( id, MCmd::cmd_write_factor_u);
}

// Команда замены множителя по напряжению на заводской 0x32
void MCommands::doSetFactorDefaultU() 
{
  Wake->configAsk( 0, MCmd::cmd_write_factor_default_u);
}


// Команда чтения коэффициента фильтрации 0x33
void MCommands::doGetSmoothU() 
{
  Wake->configAsk( 0, MCmd::cmd_read_smooth_u);
  // ...
}

// Команда записи коэффициента фильтрации 0x34
void MCommands::doSetSmoothU() 
{
  int id = 0;
  id = Wake->replyU08( id, Tools->smoothV );
  Wake->configAsk( id, MCmd::cmd_write_smooth_u);
}

// Команда чтения смещения по напряжению 0x35
void MCommands::doGetOffsetU() 
{
  Wake->configAsk( 0, MCmd::cmd_read_offset_u);
  // ...
}

// Команда записи смещения по напряжению 0x36 (0x0289)
void MCommands::doSetOffsetU() 
{
  int id = 0;
  id = Wake->replyU16( id, Tools->shiftV );
  Wake->configAsk( id, MCmd::cmd_write_offset_u);
}

// =============== Команды работы с измерителем тока ================
// Команда чтения множителя по току 0x38 
void MCommands::doGetFactorI() 
{
  Wake->configAsk( 0, MCmd::cmd_read_factor_i);
  // ...
}

// Команда записи множителя по току 0x39 (0xabcd)
void MCommands::doSetFactorI() 
{
  int id = 0;
  id = Wake->replyU16( id, Tools->factorI );
  Wake->configAsk( id, MCmd::cmd_write_factor_i);
}

// Команда замены множителя по току на заводской 0x3A
void MCommands::doSetFactorDefaultI() 
{
  Wake->configAsk( 0, MCmd::cmd_write_factor_default_i);
}

// Команда чтения коэффициента фильтрации 0x3B
void MCommands::doGetSmoothI() 
{
  Wake->configAsk( 0, MCmd::cmd_read_smooth_i);
  // ...
}

// Команда записи коэффициента фильтрации 0x3C (0x02)
void MCommands::doSetSmoothI() 
{
  int id = 0;
  id = Wake->replyU08( id, Tools->smoothI );
  Wake->configAsk( id, MCmd::cmd_write_smooth_i);
}

// Команда чтения смещения по току 0x3D
void MCommands::doGetOffsetI() 
{
  Wake->configAsk( 0, MCmd::cmd_read_offset_i);
  // ...
}

// Команда записи смещения по току 0x3E
void MCommands::doSetOffsetI() 
{
  int id = 0;
  id = Wake->replyU16( id, Tools->shiftI );
  Wake->configAsk( id, MCmd::cmd_write_offset_i);
}

//================= Команды работы с регуляторами ================= 
// Конфигурирование пид-регулятора с очисткой регистров   0x40
// set mode, kp, ki, kd, min, max
void MCommands::doPidConfigure()
{
  int id = 0;
  id = Wake->replyU08( id, Tools->pidMode );
  id = Wake->replyU16( id, Tools->kp );
  id = Wake->replyU16( id, Tools->ki );
  id = Wake->replyU16( id, Tools->kd );
  id = Wake->replyU16( id, Tools->minOut );
  id = Wake->replyU16( id, Tools->maxOut );
  Wake->configAsk( id, MCmd::cmd_pid_configure);
}

// ввод коэффициентов kp, ki, kd для заданного режима     0x41 
// set mode, kp, ki, kd  
void MCommands::doPidSetCoefficients() 
{
  int id = 0;
  id = Wake->replyU08( id, Tools->pidMode );
  id = Wake->replyU16( id, Tools->kp );
  id = Wake->replyU16( id, Tools->ki );
  id = Wake->replyU16( id, Tools->kd );
  Wake->configAsk( id, MCmd::cmd_pid_write_coefficients);
}

// ввод диапазона вывода для заданного режима     0x42 
// set mode, min, max
void MCommands::doPidOutputRange() 
{
  int id = 0;
  id = Wake->replyU08( id, Tools->pidMode );
  id = Wake->replyU16( id, Tools->minOut );
  id = Wake->replyU16( id, Tools->maxOut );
  Wake->configAsk( id, MCmd::cmd_pid_output_range);
} 

// Конфигурирование пид-регулятора без очистки регистров     0x43 
// set kp, ki, kd,min, max w/o clear
void MCommands::doPidReconfigure() 
{
  int id = 0;
  id = Wake->replyU08( id, Tools->pidMode );
  id = Wake->replyU16( id, Tools->kp );
  id = Wake->replyU16( id, Tools->ki );
  id = Wake->replyU16( id, Tools->kd );
  id = Wake->replyU16( id, Tools->minOut );
  id = Wake->replyU16( id, Tools->maxOut );
  Wake->configAsk( id, MCmd::cmd_pid_reconfigure);
}

// Очистка регистров регулятора     0x44 
// clear mode
void MCommands::doPidClear() 
{
  int id = 0;
  id = Wake->replyU08( id, Tools->pidMode );
  Wake->configAsk( id, MCmd::cmd_pid_clear);
}  

// Тестовая. Тест пид-регулятора     0x46 
// Задает ПИД-регулятору режим регулирования U,I или D и задает уровень.
// В режиме OFF ПИД-регулятор отключен, но схема скоммутирована как для регулирования 
// по напряжению. Уровень предназначен для подачи непосредственно на PWM с осторожностью. 
// mode, setpoint, sw
void MCommands::doPidTest() 
{
  int id = 0;
  id = Wake->replyU08( id, Tools->pidMode );
  id = Wake->replyU16( id, Tools->setpoint );
  Wake->configAsk( id, MCmd::cmd_pid_test);
}

// Чтение согласованных параметров обмена shift, bytes, hz     0x47 
void MCommands::doPidGetTreaty() 
{
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_pid_read_treaty);
}  

// Возвращает параметры текущего режима регулирования     0x48 
void MCommands::doPidGetConfigure() 
{
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_pid_read_configure);
  // ...
}  

// // Задает максимальный интеграл при вычислении шага регулирования
// // Задает максимальный интеграл при вычислении шага рег     0x49
// void MCommands::doPidSetMaxSum() 
// {
//   int id = 0;

//   Wake->configAsk( id, cmd_pid_write_max_sum);
// }    

// // Ввод параметров PID-регулятора для синхронизации            0x4A
//   // !!!! Не исполняется на стороне драйвера. Это резерв !!!!
// void MCommands::doPidSetTreaty() 
// {
//   int id = 0;
//   // id = Wake->replyU16( id, Tools->paramShift );
//   // id = Wake->replyU16( id, Tools->pidHz );
//   Wake->configAsk( id, MCmd::cmd_pid_write_treaty);
// }

// Ввод частоты PID-регулятора                                    0x4A
void MCommands::doPidSetFrequency() 
{
  int id = 0;
  id = Wake->replyU16( id, Tools->pidHz );
  //id = Wake->replyU16( id, 0x96 );                  // test
  Wake->configAsk( id, MCmd::cmd_pid_write_frequency);
}

//================= Команды работы с АЦП =================

// Команда запроса результатов проеобразования АЦП 0x50
void MCommands::doReadProbes()
{        
  Wake->configAsk( 0, MCmd::cmd_adc_read_probes);
  // ...
}

// Команда чтения смещения АЦП  0x51
void MCommands::doAdcGetOffset()
{
  Wake->configAsk( 0, MCmd::cmd_adc_read_offset);
  // ...
}

// Команда записи смещения АЦП  0x52
void MCommands::doAdcSetOffset()
{
  int id = 0;
  //  id = Wake->replyU16( id, Board->readAdcOffset() );
  id = Wake->replyU16( id, Tools->getAdcOffset());
  Wake->configAsk( id, MCmd::cmd_adc_write_offset);
}  

// Команда автоматической компенсации смещения АЦП 0x53
void MCommands::doAdcAutoOffset()
{
  Wake->configAsk( 0, MCmd::cmd_adc_auto_offset);
}  

// ================= Команды тестирования =================
// Команда управления ключами подключения нагрузки     0x54
void MCommands::doSwPin()
{
  int id = 0;
  id = Wake->replyU08( id, Tools->swOnOff );  // 0x00;
  Wake->configAsk( id, MCmd::cmd_write_switch_pin);
}

// Команда проверки пределов регулирования преобразователя снизу. 0x56
void MCommands::doSetPower()
{
  int id = 0;
  id = Wake->replyU16( id, Board->getPwmVal() );
  id = Wake->replyU16( id, Board->getDacVal() );
  Wake->configAsk( id, MCmd::cmd_write_power);
}

// Команда проверка управления цепью разряда.      0x57
void MCommands::doSetDischg()
{
  int id = 0;
  id = Wake->replyU08( id, Board->getPerc() );
  Wake->configAsk( id, MCmd::cmd_write_discharge);
}

// Команда включения и поддержание заданного напряжения в мВ   0x58
void MCommands::doSetVoltage()
{
  int id = 0;
  id = Wake->replyU08( id, Tools->pidMode );  // 0x01;
  id = Wake->replyU16( id, Tools->setpointU );
  Wake->configAsk( id, MCmd::cmd_write_voltage);
}

// Команда задать ток в мА и включить 0x59
void MCommands::doSetCurrent()
{
  int id = 0;
  id = Wake->replyU08( id, Tools->swOnOff );
  id = Wake->replyU16( id, Tools->setpointI );
  id = Wake->replyU16( id, Tools->factorI );
  Wake->configAsk( id, MCmd::cmd_write_current);
}

// Команда задать код DAC или ток разряда в мА и включить    0x5A
void MCommands::doSetDiscurrent()
{
  int id = 0;
  id = Wake->replyU08(id, Tools->pidMode);
  id = Wake->replyU16(id, Tools->setpointD);
  Wake->configAsk(id, MCmd::cmd_write_discurrent);
}

// Команда задать параметры компенсации перенапряжения - отменено     0x5B
void MCommands::doSurgeCompensation()
{
  int id = 0;
//   id = Wake->replyU08( id, Board->get() );  // 0x00;
//   id = Wake->replyU16( id, Board->get() );
  Wake->configAsk( id, MCmd::cmd_write_surge_compensation);
}

// Команда задать параметры доп. нагрузки на ХХ       0x5C
void MCommands::doIdleLoad()
{
  int id = 0;
  id = Wake->replyU16( id, Board->getIdleI() );
  id = Wake->replyU16( id, Board->getIdleDac() );
  Wake->configAsk( id, MCmd::cmd_write_idle_load);
}

// ================ Команды управления порогами отключения ================

// Команда чтения нижнего порога отключения по напряжению  0x60;
void MCommands::doGetLtV()              
{
  int id = 0;
  // ...
  Wake->configAsk( id, MCmd::cmd_get_lt_v );
}

// Команда записи нижнего порога отключения по напряжению  0x61;
void MCommands::doSetLtV()
{
  int id = 0;
  id = Wake->replyU16( id, Tools->getLtV() ); // 0xFF38
  Wake->configAsk( id, MCmd::cmd_set_lt_v );
}

// Команда восстановления заводского нижнего порога отключения по напряжению  0x62;
void MCommands::doSetLtDefaultV() 
{
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_set_lt_default_v );
}

// Команда чтения верхнего порога отключения по напряжению  0x63;
void MCommands::doGetUpV()              
{
  int id = 0;
  // ...
  Wake->configAsk( id, MCmd::cmd_get_up_v );
}

// Команда записи верхнего порога отключения по напряжению  0x64;
void MCommands::doSetUpV()
{
  int id = 0;
  id = Wake->replyU16( id, Tools->getUpV() ); // 0xFF38
  Wake->configAsk( id, MCmd::cmd_set_up_v );
}

// Команда восстановления заводского верхнего порога отключения по напряжению  0x65;
void MCommands::doSetUpDefaultV() 
{
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_set_up_default_v );
}

// Команда чтения нижнего порога отключения по току  0x68;
void MCommands::doGetLtI()              
{
  int id = 0;
  // ...
  Wake->configAsk( id, MCmd::cmd_get_lt_i );
}

// Команда записи нижнего порога отключения по току  0x69;
void MCommands::doSetLtI()
{
  int id = 0;
  id = Wake->replyU16( id, Tools->getLtI() ); // 0x
  Wake->configAsk( id, MCmd::cmd_set_lt_i );
}

// Команда восстановления заводского нижнего порога отключения по току  0x6A;
void MCommands::doSetLtDefaultI() 
{
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_set_lt_default_i );
}

// Команда чтения верхнего порога отключения по току  0x6B;
void MCommands::doGetUpI()              
{
  int id = 0;
  // ...
  Wake->configAsk( id, MCmd::cmd_get_up_i );
}

// Команда записи верхнего порога отключения по току  0x6C;
void MCommands::doSetUpI()
{
  int id = 0;
  id = Wake->replyU16( id, Tools->getUpI() ); // 0x
  Wake->configAsk( id, MCmd::cmd_set_up_i );
}

// Команда восстановления заводского верхнего порога отключения по току  0x6D;
void MCommands::doSetUpDefaultI() 
{
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_set_up_default_i );
}

// ================ Команды универсальные ================

// нет операции   0x00
void MCommands::doNop()
{  
  Wake->configAsk( 0x00, MCmd::cmd_nop );

}

// Считать информацию о драйвере
void MCommands::doInfo()
{  
  int id = 0;
  Wake->configAsk( id, MCmd::cmd_info );
}
