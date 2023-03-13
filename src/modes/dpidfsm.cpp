/*
  Файл: dpidfsm.cpp
  
    Параметры передаются в команде как положительные  20230301

  Версия от  07.03.2023
  */

#include "modes/dpidfsm.h"
#include "mtools.h"
#include "board/mboard.h"
#include "measure/mkeyboard.h"
#include "display/mdisplay.h"
#include <Arduino.h>

namespace MDPid
{
    // Переменные, используемые более чем в одном состоянии
  float sp, kp, ki, kd;
  short prof = 1;
  float tmp = 0.0f;     // test

  //========== MStart, инициализация ========================================
  MStart::MStart(MTools * Tools) : MState(Tools)
  {
    Tools->txPowerStop();                               // Команда перейти в безопасный режим (0x21)
    Display->showMode((char*)"       D-PID       ");    // Произведен вход в D-PID
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



  //========== MClearPidKeys, очистка всех ключей режима D-PID ==============
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
          case 4:  done = Tools->clearAllKeys("pidtest");   break;
          case 3:  done = Tools->clearAllKeys("profil1");   break;
          case 2:  done = Tools->clearAllKeys("profil2");   break;
          case 1:  done = Tools->clearAllKeys("profil3");   break;
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



  //========== MLoad, выбор и загрузка профиля ==============================
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
            kp = Tools->readNvsFloat("profil1", "kpD", MConst::fixedKpD);
            ki = Tools->readNvsFloat("profil1", "kiD", MConst::fixedKiD);
            kd = Tools->readNvsFloat("profil1", "kdD", MConst::fixedKdD);
            break;
          case 2:
            kp = Tools->readNvsFloat("profil2", "kpD", MConst::fixedKpD);
            ki = Tools->readNvsFloat("profil2", "kiD", MConst::fixedKiD);
            kd = Tools->readNvsFloat("profil2", "kdD", MConst::fixedKdD);
            break;
          case 3:
            kp = Tools->readNvsFloat("profil3", "kpD", MConst::fixedKpD);
            ki = Tools->readNvsFloat("profil3", "kiD", MConst::fixedKiD);
            kd = Tools->readNvsFloat("profil3", "kdD", MConst::fixedKdD);
            break;
          default:;
        }
        // Команда задать режим по разряду и коэффициенты PID-регулятора
        // и перейти к заданию порога PID-регулятора разряда
  Serial.print("kp=");  Serial.println(kp);
        Tools->txSetPidCoeffD(kp, ki, kd);              return new MLoadSp(Tools);

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



  //========== MLoadSp, ввод порога PID-регулятора разряда ================== 
  MLoadSp::MLoadSp(MTools * Tools) : MState(Tools)
  {
    sp = Tools->readNvsFloat("pidtest", "spD", MConst::fixedSpD);
   
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
        Tools->writeNvsFloat("pidtest", "spD", sp);     return new MLoadKp(Tools);
        // Перейти к сохранению профиля под любым номером
      case MKeyboard::P_CLICK: Board->buzzerOn();       return new MSaveProf(Tools);
        // Далее UP/DOWN сетпойнта по току 
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, 0.1f);
        Tools->txDischargeGo(sp);
        break;
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, 1.0f);
        Tools->txDischargeGo(sp);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, -0.1f);
        Tools->txDischargeGo(sp);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, -1.0f);
        Tools->txDischargeGo(sp);
        break;

        // Включить (0x24)или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
// Serial.print("statusD=0x");   Serial.println(Tools->getStatusPidDiscurrent(), HEX);
// Serial.print("state=0x");   Serial.println( Tools->getState(), HEX);
        (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
          Tools->txPowerStop() : Tools->txDischargeGo(sp); // Напряжение задать любое
        break;
      default:;
    }
      // Индикация, которая могла измениться при исполнении.
    //Display->showMode((char*)"  D-SP = ", sp);
    Display->showMode((char*)"  D-SP = ", -sp);
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
  //Serial.print("state=0x"); Serial.println(Tools->getState(), HEX);
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKp, ввод параметра KP PID-регулятора разряда ==================
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
        Tools->writeNvsFloat("pidtest", "kpD", kp);     return new MLoadSp(Tools);
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kpD", kp);     return new MLoadKi(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, 0.1f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, 1.0f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, -0.1f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, -1.0f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
        // Включить (0x24)или отключить (0x21) разряд
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                          Tools->txPowerStop() : Tools->txDischargeGo(sp);
        break;
      default:;
    }
    Display->showPidI(kp, 1);
    Display->showAmp(Tools->getRealCurrent(), 3);
    Display->showMode((char*)"        KP         ");
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKi, ввод параметра KI PID-регулятора разряда ================== 
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
        Tools->writeNvsFloat("pidtest", "kiD", ki);     return new MLoadKp(Tools);
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kiD", ki);     return new MLoadKd(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, 0.1f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, 1.0f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, -0.1f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, -1.0f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
        // Включить (0x24)или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                          Tools->txPowerStop() : Tools->txDischargeGo(sp);
        break;
      default:;
    }
    Display->showPidI(ki, 1);
    Display->showAmp(Tools->getRealCurrent(), 3);
    Display->showMode((char*)"        KI         ");
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                        Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKd, ввод параметра KD PID-регулятора разряда ==================
  MLoadKd::MLoadKd(MTools * Tools) : MState(Tools)
  {
    Display->showHelp((char*)"  P*-SP B*-SAVE   ");     // Индикация 
  }
  MState * MLoadKd::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
      case MKeyboard::P_LONG_CLICK: Board->buzzerOn();  return new MLoadSp(Tools);
      case MKeyboard::P_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kdD", kd);     return new MLoadKi(Tools);      
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kdD", kd);     return new MLoadSp(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, 0.1f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, 1.0f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, -0.1f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, -1.0f);
        Tools->txSetPidCoeffD(kp, ki, kd);
        break;
        // Включить (0x24)или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
                                          Tools->txPowerStop() : Tools->txDischargeGo(sp);
        break;
      default:;
    }
    Display->showPidI(kd, 1);
    Display->showAmp(Tools->getRealCurrent(), 3);
    Display->showMode((char*)"        KD         ");
    (Tools->getState() == Tools->getStatusPidDiscurrent()) ? 
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
            Tools->writeNvsFloat("profil1", "kpD", MConst::fixedKpD);
            Tools->writeNvsFloat("profil1", "kiD", MConst::fixedKiD);
            Tools->writeNvsFloat("profil1", "kdD", MConst::fixedKdD);
            break;
          case 2:
            Tools->writeNvsFloat("profil2", "kpD", MConst::fixedKpD);
            Tools->writeNvsFloat("profil2", "kiD", MConst::fixedKiD);
            Tools->writeNvsFloat("profil2", "kdD", MConst::fixedKdD);
            break;
          case 3:
            Tools->writeNvsFloat("profil3", "kpD", MConst::fixedKpD);
            Tools->writeNvsFloat("profil3", "kiD", MConst::fixedKiD);
            Tools->writeNvsFloat("profil3", "kdD", MConst::fixedKdD);
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



  //========== MStop, завершение режима D-PID ================================= 
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
    Display->showVolt(Tools->getRealVoltage(), 3);
    Display->showAmp (Tools->getRealCurrent(), 3);
    return this;
  };



  //========== MExit, выход из режима D-PID =================================== 
  MExit::MExit(MTools * Tools) : MState(Tools)
  {
    Display->showMode((char*)"     D-PID OFF    ");
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
      Display->showMode((char*)"       D-PID:      ");
      Display->showHelp((char*)"  SP KP KI KD PRF  ");  return nullptr;
    default:;
    }
    return this;
  };

};
