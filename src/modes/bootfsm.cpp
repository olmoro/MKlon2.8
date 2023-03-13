/*
  bootfsm.cpp
  Конечный автомат синхронизации данных между контроллерами.
            Как это работает.
    Процесс синхронизации (BOOT) запускается диспетчером при инициализации прибора. 
  Пользовательские параметры (OPTIONS) и параметры разработчика (DEVICE) восстанавливаются из
  энергонезависимой памяти ESP32 и одновременнно передаются на драйвер SAMD21, где заменяют 
  соответствующие дефолтные значения. Во время синхронизации на дисплей выводится информация 
  о ходе синхронизации. По окончании процесса синхронизации прибор готов к работе в выбранном режиме.
  14.02.2023 
  20230310 Это только заготовка - нет проверок и при необходимости повторных обращений
*/

#include "modes/bootfsm.h"
#include "mtools.h"
#include "board/mboard.h"
#include "display/mdisplay.h"
#include <Arduino.h>
#include <string>

namespace MBoot
{
  // Старт и инициализация выбранного режима работы.
  MStart::MStart(MTools * Tools) : MState(Tools) 
  {
    Board->ledsBlue();            // Подтверждение входа синим свечением светодиода как и любой загрузки
  }
  MState * MStart::fsm()
  {
    Tools->setBlocking(true);                                     // Блокировать обмен
    vTaskDelay(1000/portTICK_PERIOD_MS);                          // Не беспокоим драйвер 3 секунды после рестарта 
    Tools->setBlocking(false);                                    // Разблокировать обмен
    return new MTxPowerStop(Tools);                               // Перейти к отключению
  };

  MTxPowerStop::MTxPowerStop(MTools * Tools) : MState(Tools) {}
  MState * MTxPowerStop::fsm()
  {
    Tools->txPowerStop();                                                     // 0x21  Команда драйверу
    return new MTxSetFrequency(Tools);
  };

  MTxSetFrequency::MTxSetFrequency(MTools * Tools) : MState(Tools) {}
  MState * MTxSetFrequency::fsm()
  {
    i = Tools->readNvsShort("device", "freq", fixed);                 // Взять сохраненное из nvs.
    if(i <= 0) i = 0;      if(i >= 5) i = 5;
    Tools->txSetPidFrequency(freq[i]);                                // 0x4A  Команда драйверу
    #ifdef PRINT_BOOT
      Serial.print("4A*SetFrequency=0x"); Serial.println(freq[i], HEX);
    #endif    
    return new MTxGetTreaty(Tools);
  };
  
  // Получить согласованные данные для обмена с драйвером.
  MTxGetTreaty::MTxGetTreaty(MTools * Tools) : MState(Tools) {}
  MState * MTxGetTreaty::fsm()
  {
    Tools->txGetPidTreaty();                                                  // 0x47  Команда драйверу
    #ifdef PRINT_BOOT
      Serial.println("47*GetTreaty: done");
    #endif
    return new MTxsetFactorV(Tools);
  };

  // Восстановление пользовательского (или заводского) коэфициента преобразования в милливольты.
  MTxsetFactorV::MTxsetFactorV(MTools * Tools) : MState(Tools) {}
  MState * MTxsetFactorV::fsm()
  {
    Tools->factorV = Tools->readNvsShort("device", "factorV", fixed);         // Взять сохраненное из nvs.
    Tools->txSetFactorU(Tools->factorV);                                      // 0x31  Команда драйверу
    #ifdef PRINT_BOOT
      Serial.print("31*SetFactorU=0x");    Serial.println(Tools->factorV, HEX);
    #endif
    return new MTxSmoothV(Tools);                                             // Перейти к следующему параметру
  };

  // Восстановление пользовательского (или заводского) коэффициента фильтрации по напряжению.
  MTxSmoothV::MTxSmoothV(MTools * Tools) : MState(Tools) {}
  MState * MTxSmoothV::fsm()
  {
    Tools->smoothV = Tools->readNvsShort("device", "smoothV", fixed);         // Взять сохраненное из nvs.
    Tools->txSetSmoothU(Tools->smoothV);                                      // 0x34  Команда драйверу
    #ifdef PRINT_BOOT
      Serial.print("34*SetSmoothU=0x");   Serial.println(Tools->smoothV, HEX);
    #endif
    return new MTxShiftV(Tools);                                              // Перейти к следующему параметру
  };

  // Восстановление пользовательской (или заводской) настройки сдвига по напряжению.
  MTxShiftV::MTxShiftV(MTools * Tools) : MState(Tools) {}
  MState * MTxShiftV::fsm()
  {
    Tools->shiftV = Tools->readNvsShort("device", "offsetV", fixed);          // Взять сохраненное из nvs.
    Tools->txSetShiftU(Tools->shiftV);                                        // 0x36  Команда драйверу
    #ifdef PRINT_BOOT    
      Serial.print("36*SetShiftU=0x");    Serial.println(Tools->shiftV, HEX);
    #endif
    return new MTxFactorI(Tools);                                             // Перейти к следующему параметру
  };

  // Восстановление пользовательского (или заводского) коэфициента преобразования в миллиамперы.
  MTxFactorI::MTxFactorI(MTools * Tools) : MState(Tools) {}
  MState * MTxFactorI::fsm()
  {
    Tools->factorI = Tools->readNvsShort("device", "factorI", fixed);         // Взять сохраненное из nvs.
    Tools->txSetFactorI(Tools->factorI);                                      // 0x39  Команда драйверу
    #ifdef PRINT_BOOT    
      Serial.print("39*SetFactorI=0x");    Serial.println(Tools->factorI, HEX);
    #endif
    return new MTxSmoothI(Tools);                                             // Перейти к следующему параметру
  };

  // Восстановление пользовательского (или заводского) коэффициента фильтрации по току.
  MTxSmoothI::MTxSmoothI(MTools * Tools) : MState(Tools) {}
  MState * MTxSmoothI::fsm()
  {
    Tools->smoothI = Tools->readNvsShort("device", "smoothI", fixed);         // Взять сохраненное из nvs.
    Tools->txSetSmoothI(Tools->smoothI);                                      // 0x3C  Команда драйверу
    #ifdef PRINT_BOOT
      Serial.print("3C*SetSmoothI=0x");    Serial.println(Tools->smoothI, HEX);
    #endif
    return new MTxShiftI(Tools);                                              // Перейти к следующему параметру
  };

  // Восстановление пользовательской (или заводской) настройки сдвига по току.
  MTxShiftI::MTxShiftI(MTools * Tools) : MState(Tools) {}
  MState * MTxShiftI::fsm()
  {
    Tools->shiftI = Tools->readNvsShort("device", "offsetI", fixed);          // Взять сохраненное из nvs.
    Tools->txSetShiftI(Tools->shiftI);                                        // 0x3E  Команда драйверу
    #ifdef PRINT_BOOT    
      Serial.print("3C*SetShiftI=0x");    Serial.println(Tools->shiftI, HEX);
    #endif
    return new MExit(Tools);      //return new MTxPidConfigure(Tools); // Перейти к следующему параметру (Временно закончить)
  };

  // Процесс выхода из режима "BOOT".
  // Состояние: "Индикация итогов и выход из режима заряда в меню диспетчера" 
  MExit::MExit(MTools * Tools) : MState(Tools)
  {Display->showHelp((char*) "   ...READY...    " );}    
  MState * MExit::fsm()
  {

    Tools->txReady();                                                         // 0x15  Команда драйверу
    #ifdef PRINT_BOOT    
      Serial.println("15*Ready: done");
    #endif

    Board->ledsOff();
    Display->showVolt(Tools->getRealVoltage(), 3);
    Display->showAmp (Tools->getRealCurrent(), 3);
    Board->setReady(true);
    //Serial.println(")");
    return nullptr;                                                           // Возврат к выбору режима
  }
};  //MExit

// !Конечный автомат синхронизации данных между контроллерами.
