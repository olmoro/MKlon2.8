/*
  Файл: ipidfsm.cpp
  Как это работает смотри в upidfsm.cpp

















































  Версия от  15.03.2023
  */

#include "modes/ipidfsm.h"
#include "mtools.h"
#include "board/mboard.h"
#include "measure/mkeyboard.h"
#include "display/mdisplay.h"
#include <Arduino.h>

namespace MIPid
{
    // Переменные, используемые более чем в одном состоянии
  float sp, kp, ki, kd;
  short prof = 1;
  float tmp = 0.0f;     // test

  //========== MStart, инициализация ========================================
  MStart::MStart(MTools * Tools) : MState(Tools)
  {
    Tools->txPowerStop();                               // Команда перейти в безопасный режим (0x21)
    Display->showMode((char*)"       I-PID       ");    // Произведен вход в I-PID
    Display->showHelp((char*)"  C-GO    C*-EXIT  ");    // Подсказка
    Board->ledsOn();                                    // Подтверждение входа белым свечением
    cnt = 7;                                            // Счетчик нажатий для очистки
  
  Serial.print("ParamMult=0x");   Serial.println(Tools->getParamMult(), HEX);
  
  }
  MState * MStart::fsm()
  {
    switch (Keyboard->getKey())    //Здесь так можно
    {
        // Выход из режима длинным нажатием "C"
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
        // Начать с выбора и загрузки профиля (kp, KI, KD)
      case MKeyboard::C_CLICK: Board->buzzerOn();       
        // Обнуляются счетчики времени и отданного заряда
        Tools->clrTimeCounter();
        Tools->clrAhCharge();                           return new MLoadProf(Tools);
        // Недокументированная возможность очистки параметров режима (ключей)
      case MKeyboard::B_CLICK: Board->buzzerOn();
        if(--cnt <= 0)                                  return new MClearPidKeys(Tools);
      default:;
    }
      return this;
  };



  //========== MClearPidKeys, очистка всех ключей режима I-PID ==============
  MClearPidKeys::MClearPidKeys(MTools * Tools) : MState(Tools)
{
    Display->showHelp((char*)"  P-EXIT   C-YES  ");   // Подсказка
    Board->ledsBlue();
  }
  MState * MClearPidKeys::fsm()
  {
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();    return new MStop(Tools);
        // Выход из очистки только в состояние MStart
      case MKeyboard::P_CLICK: Board->buzzerOn();         return new MStart(Tools);

        // Выбрать объект очистки
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        settings = Tools->updnInt(settings, dn, up, 1);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        settings = Tools->updnInt(settings, dn, up, -1);
        break;

        // Произвести очистку выбранных настроек
      case MKeyboard::C_CLICK: Board->buzzerOn();
        switch(settings)
        {
          case 0:  done = Tools->clearAllKeys("pidtest");   break;
          case 1:  done = Tools->clearAllKeys("profil 1");   break;
          case 2:  done = Tools->clearAllKeys("profil 2");   break;
          case 3:  done = Tools->clearAllKeys("profil 3");   break;
          default:;
        }

        break;
      default:;
    }

    switch(settings)
    {
      case 0: Display->showMode((char*)"      \"PIDTEST\"  ");  break;
      case 1: Display->showMode((char*)"      \"PROFIL1\"  ");  break;
      case 2: Display->showMode((char*)"      \"PROFIL2\"  ");  break;
      case 3: Display->showMode((char*)"      \"PROFIL3\"  ");  break;
      default:;
    }
    return this;
  };



  //========== MLoadProf, выбор и загрузка профиля ==========================
  MLoadProf::MLoadProf(MTools * Tools) : MState(Tools)
  {

      // Tools->txSetPidParameters(8,10); // убрать, недоделано
      Tools->txGetPidTreaty(); // 0x47 test 20230201

    Display->showHelp((char*)"  +/- PROF   C-GO  ");
    Board->ledsCyan();
  }
  MState * MLoadProf::fsm()
  {
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
        // Загрузить выбранный профиль
      case MKeyboard::C_CLICK: Board->buzzerOn();
        switch (prof)
         {
          case 1:
            kp = Tools->readNvsFloat("profil1", "kpI", MConst::fixedKpI);
            ki = Tools->readNvsFloat("profil1", "kiI", MConst::fixedKiI);
            kd = Tools->readNvsFloat("profil1", "kdI", MConst::fixedKdI);
            break;
          case 2:
            kp = Tools->readNvsFloat("profil2", "kpI", MConst::fixedKpI);
            ki = Tools->readNvsFloat("profil2", "kiI", MConst::fixedKiI);
            kd = Tools->readNvsFloat("profil2", "kdI", MConst::fixedKdI);
            break;
          case 3:
            kp = Tools->readNvsFloat("profil3", "kpI", MConst::fixedKpI);
            ki = Tools->readNvsFloat("profil3", "kiI", MConst::fixedKiI);
            kd = Tools->readNvsFloat("profil3", "kdI", MConst::fixedKdI);
            break;
          default:;
        }
        // Команда задать режим по току и коэффициенты PID-регулятора
        // и перейти к заданию порога PID-регулятора тока
  Serial.print("kp=");  Serial.println(kp);
        Tools->txSetPidCoeffI(kp, ki, kd);              return new MLoadSp(Tools);

        // Выбор следующего профиля
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        prof = Tools->updnInt(prof, dn, up, 1);
        break;
        // Выбор предыдущего профиля
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        prof = Tools->updnInt(prof, dn, up, -1);
        break;    
      default:;
    }
      // Выбор float вызван тем, что в этой строке возможен лишь float-to-string
    Display->showMode((char*)"  PROF = ", (float)prof); 
    return this;
  };



  //========== MLoadSp, ввод порога PID-регулятора тока ===================== 
  MLoadSp::MLoadSp(MTools * Tools) : MState(Tools)
  {
    sp = Tools->readNvsFloat("pidtest", "spI", MConst::fixedSpI);
 
      // Индикация при инициализации состояния
    Display->showHelp((char*)"  B-SAVE    C-GO  ");
    Board->ledsOff();
  }
  MState * MLoadSp::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);

        // Ввод sp закончен, сохранить и перейти к вводу KP
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "spI", sp);     return new MLoadKp(Tools);
        // Перейти к сохранению профиля под любым номером
      case MKeyboard::P_CLICK: Board->buzzerOn();       return new MSaveProf(Tools);
        // Далее UP/DOWN сетпойнта по току 
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, 0.05f);
        Tools->txPowerMode(10.0, sp, MODE_I);
        break;
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, 1.0f);
        Tools->txPowerMode(10.0, sp, MODE_I);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, -0.05f);
        Tools->txPowerMode(10.0, sp, MODE_I);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, -1.0f);
        Tools->txPowerMode(10.0, sp, MODE_I);
        break;

        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        // Serial.print("statusI=0x");   Serial.println(Tools->getStatusPidCurrent(), HEX);
        // Serial.print("state=0x");   Serial.println( Tools->getState(), HEX);
        (Tools->getState() == Tools->getStatusPidCurrent()) ? 
          Tools->txPowerStop() : Tools->txPowerMode(10.0, sp, MODE_I); // Напряжение задать любое
        break;
      default:;
    }
      // Индикация, которая могла измениться при исполнении.
    Tools->showAmp(Tools->getRealCurrent(), 3, 4);

    Display->showMode((char*)"  I-SP = ", sp);
    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
                                    Board->ledsGreen() : Board->ledsRed();
  //Serial.print("state=0x"); Serial.println(Tools->getState(), HEX);
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKp, ввод параметра KP PID-регулятора тока =============== 
  MLoadKp::MLoadKp(MTools * Tools) : MState(Tools)
  {
      // Индикация 
    Display->showHelp((char*)"  P*-SP B*-SAVE   ");
  }
  MState * MLoadKp::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
      case MKeyboard::P_LONG_CLICK: Board->buzzerOn();  return new MLoadSp(Tools);
      case MKeyboard::P_CLICK: Board->buzzerOn();       
        Tools->writeNvsFloat("pidtest", "kpI", kp);     return new MLoadSp(Tools);
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kpI", kp);     return new MLoadKi(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, 0.01f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, 0.10f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, -0.01f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, -0.10f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidCurrent()) ? 
          Tools->txPowerStop() : Tools->txPowerMode(10.0, sp, MODE_I); // Ток задать любой
        break;
      default:;
    }
    Display->showPidI(kp, 2);
    //Display->showAmp(Tools->getRealCurrent(), 3);
    Tools->showAmp(Tools->getRealCurrent(), 3, 4);
    Display->showMode((char*)"        KP         ");
    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
      Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKi, ввод параметра KI PID-регулятора тока =============== 
  MLoadKi::MLoadKi(MTools * Tools) : MState(Tools)
  {
      // Индикация 
    Display->showHelp((char*)"  P*-SP B*-SAVE   ");
  }
  MState * MLoadKi::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
      case MKeyboard::P_LONG_CLICK: Board->buzzerOn();  return new MLoadSp(Tools);
      case MKeyboard::P_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kiI", ki);     return new MLoadKp(Tools);
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kiI", ki);     return new MLoadKd(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, 0.01f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, 0.10f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, -0.01f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, -0.10f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidCurrent()) ? 
          Tools->txPowerStop() : Tools->txPowerMode(10.0, sp, MODE_I); // Ток задать любой




        break;
      default:;
    }
    Display->showPidI(ki, 2);
    //Display->showAmp(Tools->getRealCurrent(), 3);
    Tools->showAmp(Tools->getRealCurrent(), 3, 4);
    Display->showMode((char*)"        KI         ");
    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
      Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKd, ввод параметра KD PID-регулятора тока =============== 
  MLoadKd::MLoadKd(MTools * Tools) : MState(Tools)
  {
      // Индикация 
    Display->showHelp((char*)"  P*-SP B*-SAVE   ");
  }
  MState * MLoadKd::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
      case MKeyboard::P_LONG_CLICK: Board->buzzerOn();  return new MLoadSp(Tools);
      case MKeyboard::P_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kdI", kd);     return new MLoadKi(Tools);      
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kdI", kd);     return new MLoadSp(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, 0.01f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, 0.10f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, -0.01f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, -0.10f);
        Tools->txSetPidCoeffI(kp, ki, kd);
        break;
        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidCurrent()) ? 
          Tools->txPowerStop() : Tools->txPowerMode(10.0, sp, MODE_I); // Ток задать любой
        break;
      default:;
    }
    Display->showPidI(kd, 2);
    //Display->showAmp(Tools->getRealCurrent(), 3);
    Tools->showAmp(Tools->getRealCurrent(), 3, 4);
    Display->showMode((char*)"        KD         ");
    (Tools->getState() == Tools->getStatusPidCurrent()) ? 
      Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MSaveProf, сохранение профиля под выбранным номером ========== 
  MSaveProf::MSaveProf(MTools * Tools) : MState(Tools)
  {
    Display->showHelp((char*)"  P*-SP B*-SAVE   ");
  }
  MState * MSaveProf::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
      case MKeyboard::P_LONG_CLICK: Board->buzzerOn();  return new MLoadSp(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        prof = Tools->updnInt(prof, dn, up, 1);
        break;     
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        prof = Tools->updnInt(prof, dn, up, -1);
        break;
        // Загрузить профиль
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();
        switch (prof)
        {
          case 1:
            Tools->writeNvsFloat("profil1", "kpI", MConst::fixedKpI);
            Tools->writeNvsFloat("profil1", "kiI", MConst::fixedKiI);
            Tools->writeNvsFloat("profil1", "kdI", MConst::fixedKdI);
            break;
          case 2:
            Tools->writeNvsFloat("profil2", "kpI", MConst::fixedKpI);
            Tools->writeNvsFloat("profil2", "kiI", MConst::fixedKiI);
            Tools->writeNvsFloat("profil2", "kdI", MConst::fixedKdI);
            break;
          case 3:
            Tools->writeNvsFloat("profil3", "kpI", MConst::fixedKpI);
            Tools->writeNvsFloat("profil3", "kiI", MConst::fixedKiI);
            Tools->writeNvsFloat("profil3", "kdI", MConst::fixedKdI);
            break;
          default:;
        }
      default:;
    }
    Display->showMode((char*)"  PROF = ", (float)prof);
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MStop, завершение режима I-PID ================================= 
  MStop::MStop(MTools * Tools) : MState(Tools)
  {
    Tools->txPowerStop();                               // 0x21  Команда драйверу перейти в безопасный режим
    Display->showMode((char*)"       STOP        ");
    Display->showHelp((char*)"      C-EXIT       ");
    Board->ledsRed();
  }    
  MState * MStop::fsm()
  {
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_CLICK:  Board->buzzerOn();        return new MExit(Tools);
    default:;
    }
    //Display->showVolt(Tools->getRealVoltage(), 3);
    //Display->showAmp (Tools->getRealCurrent(), 3);
    Tools->showVolt(Tools->getRealVoltage(), 3, 2);
    Tools->showAmp (Tools->getRealCurrent(), 3, 2);
    return this;
  };



  //========== MExit, выход из режима I-PID =================================== 
  MExit::MExit(MTools * Tools) : MState(Tools)
  {
    Display->showMode((char*)"     I-PID OFF    ");
    Display->showHelp((char*)"  P-AGAIN  C-EXIT ");
    Board->ledsOff();
  }    
  MState * MExit::fsm()
  {
    switch (Keyboard->getKey())
    {
      // Стартовать снова без выхода в главное меню
    case MKeyboard::P_CLICK:  Board->buzzerOn();        return new MStart(Tools);
      // Выйти в главное меню
    case MKeyboard::C_CLICK:  Board->buzzerOn(); 
      Display->showMode((char*)"       I-PID:      ");
      Display->showHelp((char*)"  SP KP KI KD PRF  ");  return nullptr;
    default:;
    }
    return this;
  };

};
