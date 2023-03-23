/*
  Файл: upidfsm.cpp
      Это инструмент разработчика, не более того, облегчающий настройку
                 ПИД-регулятора по напряжению.

    Параметры ПИД-регулятора по напряжению сохраняются в разделах энергонезависимой
  памяти (NVS) под именами "profil1" ... "profil3", ключи "kpV", "kiV" и "kdV".
  Порог регулирования, он же SetPoint или SP под именем "pidtest", ключ "spV".

                    ==========  КАК ЭТО РАБОТАЕТ ==========
    
    Подбор коэффициентов PID-регулятора (что само по себе чистое шаманство) следует 
  производить с нагрузкой соответствующей мощности и контролем за процессом с помощью 
  браузера по беспроводной сети.
    Вход из главного меню по кнопке "B" подтверждается белым свечением светодиода. 
  Длинным нажатием "C" из любого состояния производится завершение режима без 
  сохранения результатов, которые на тот момент не были сохранены. 
    По нажатию "C" происходит переход к состоянию выбора профиля (свечение светодиода  
  голубым), в котором содержится подобранная ранее комбинация коэффициентов KP, PI, KD
  или, в случае отсутствия таковой - их заводская комбинация. Нажатием "C" коэффициенты 
  загружаются в драйвер. Красное свечение светодиода означает, что параметры загружены
  и произведен переход в состояние ввода порога регулирования по напряжению с ожиданием
  физического пуска процесса.
    Для подачи напряжения на клеммы необходимо нажатие на "C", свечение светодиода 
  будет зеленое, но это только в том случае, если не зафиксированы аварийные ситуации, 
  как то перегрузки, перегрев и т.п. В таком случае всё зависит от того, как они 
  обрабатываются вашим прибором. Пороговое напряжение можно менять "на лету" 
  кнопками UP (+) и DN (-) шагами по 0,1 или 1,0 вольту коротким или длинным нажатием 
  соответственно. Повторное нажатие на "C" выключает подачу напряжения (светодиод 
  красный). Не отключая напряжения на клеммах нажатием на "B" переходим к подбору 
  коэффициентов.
    Выбор конкретного коэффициента производится кольцевым перебором в прямом и обратном 
  направлении кнопками "B" и "P". Длинным нажатием этих же кнопок инициируется выход на 
  сохранение подобранных параметров в энергонезависимой памяти или возврат к 
  состоянию ввода порогового напряжения соответственно. Именно поcле возврата к 
  состоянию ввода порогового напряжения можно окончательно убедиться в качестве подбора 
  параметров, наблюдая отсутствие колебательного процесса или существенного выброса при 
  изменении напряжения, имитируя насколько это возможно, включая и отключая источник 
  силового питания.  
    Подобранные настройки могут быть сохранены под любым из возможных трёх профилей 
  и в дальнейшем применены при реализации целевых алгоритмов прибора. Не лишним 
  будет подстраховаться и зафиксировать у себя подобранные параметры традиционным 
  методом.
    И напоследок об одной недокументированной возможности. В процессе разработки 
  может возникнуть такая ситуация, когда потребуется переименовать ключи, под которыми 
  записываются параметры. Старые ключи необходимо удалить, а если этого не сделать, 
  то новые запишутся, а старые будут напрасно занимать память. Чтобы запустить 
  эту процедуру, надо после подтверждения входа (при белом свечении светодиода) семь 
  раз нажать на ту же кнопку "B" и при голубом свечении выбрать и удалить все ключи 
  в выбранной группе настроек (профилей). Следует иметь ввиду, что будут удалены 
  все 6 коэффициентов, как по напряжению, так и по току.  
  
  Версия от  15.03.2023 - алгоритм несколько изменен
  */

#include "modes/upidfsm.h"
#include "mtools.h"
#include "board/mboard.h"
#include "measure/mkeyboard.h"
#include "display/mdisplay.h"
#include <Arduino.h>

namespace MUPid
{
    // Переменные, используемые более чем в одном состоянии
  float sp, kp, ki, kd;
  short prof = 1;
  float tmp = 0.0f;

  //========== MStart, инициализация ========================================
  MStart::MStart(MTools * Tools) : MState(Tools)
  {
    Tools->txPowerStop();                               // Команда перейти в безопасный режим (0x21)
    Display->showMode((char*)"       U-PID       ");    // Произведен вход в U-PID
    Display->showHelp((char*)"  C-GO    C*-EXIT  ");    // Подсказка
        
        Tools->showAmp(Tools->getRealCurrent(), 3, 5);

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



  //========== MClearPidKeys, очистка всех ключей режима U-PID ==============
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
  Serial.print("mult=0x");  Serial.println(Tools->pMult, HEX);
  Serial.print("max=0x");   Serial.println(Tools->pMax, HEX);



    Display->showHelp((char*)"  +/-  C-GO  C*-EX ");
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
            kp = Tools->readNvsFloat("profil1", "kpV", MConst::fixedKpV);
            ki = Tools->readNvsFloat("profil1", "kiV", MConst::fixedKiV);
            kd = Tools->readNvsFloat("profil1", "kdV", MConst::fixedKdV);
            break;
          case 2:
            kp = Tools->readNvsFloat("profil2", "kpV", MConst::fixedKpV);
            ki = Tools->readNvsFloat("profil2", "kiV", MConst::fixedKiV);
            kd = Tools->readNvsFloat("profil2", "kdV", MConst::fixedKdV);
            break;
          case 3:
            kp = Tools->readNvsFloat("profil3", "kpV", MConst::fixedKpV);
            ki = Tools->readNvsFloat("profil3", "kiV", MConst::fixedKiV);
            kd = Tools->readNvsFloat("profil3", "kdV", MConst::fixedKdV);
            break;
          default:;
        }
        // Команда задать режим разряда и коэффициенты PID-регулятора
        // и перейти к заданию порога PID-регулятора напряжения
  Serial.print("kp=");  Serial.println(kp);
        Tools->txSetPidCoeffV(kp, ki, kd);              return new MLoadSp(Tools);

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
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    return this;
  };



  //========== MLoadSp, ввод порога PID-регулятора напряжения =============== 
  MLoadSp::MLoadSp(MTools * Tools) : MState(Tools)
  {
    sp = Tools->readNvsFloat("pidtest", "spV", MConst::fixedSpV);   //не находит??))
  
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
        Tools->writeNvsFloat("pidtest", "spV", sp);     return new MLoadKp(Tools);
        // Перейти к сохранению профиля под любым номером
      case MKeyboard::P_CLICK: Board->buzzerOn();       return new MSaveProf(Tools);
        // Далее UP/DOWN сетпойнта по напряжению 
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, 0.1f);
        Tools->txPowerMode(sp, 2.0, MODE_V);
        break;
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, 1.0f);
        Tools->txPowerMode(sp, 2.0, MODE_V);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, -0.1f);
        Tools->txPowerMode(sp, 2.0, MODE_V);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        sp = Tools->updnFloat(sp, dn, up, -1.0f);
        Tools->txPowerMode(sp, 2.0, MODE_V);
        break;

        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        // Serial.print("statusV=0x");   Serial.println(Tools->getStatusPidVoltage(), HEX);
        // Serial.print("state=0x");   Serial.println( Tools->getState(), HEX);
        (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                       Tools->txPowerStop() : Tools->txPowerMode(sp, 2.0, MODE_V); // Ток задать любой
        break;
      default:;
    }
      // Индикация, которая могла измениться при исполнении.
    Display->showMode((char*)"  U-SP = ", sp);
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
  //Serial.print("state=0x"); Serial.println(Tools->getState(), HEX);
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKp, ввод параметра KP PID-регулятора напряжения ========= 
  MLoadKp::MLoadKp(MTools * Tools) : MState(Tools)
  {
      // Индикация 
    Display->showHelp((char*)"  P*-SP  B*-SAVE  ");
  }
  MState * MLoadKp::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
      case MKeyboard::P_LONG_CLICK: Board->buzzerOn();  return new MLoadSp(Tools);
      case MKeyboard::P_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kpV", kp);     return new MLoadSp(Tools);
      case MKeyboard::B_CLICK: Board->buzzerOn();       
        Tools->writeNvsFloat("pidtest", "kpV", kp);     return new MLoadKi(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, 0.01f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, 0.10f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, -0.01f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        kp = Tools->updnFloat(kp, dn, up, -0.10f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                       Tools->txPowerStop() : Tools->txPowerMode(sp, 2.0, MODE_V); // Ток задать любой
        break;
      default:;
    }
    Display->showVolt(Tools->getRealVoltage(), 3);
    Display->showPidV(kp, 2);
    Display->showMode((char*)"        KP         ");
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKi, ввод параметра KI PID-регулятора напряжения ========= 
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
        Tools->writeNvsFloat("pidtest", "kiV", ki);     return new MLoadKp(Tools);
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kiV", ki);     return new MLoadKd(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, 0.01f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, 0.10f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, -0.01f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        ki = Tools->updnFloat(ki, dn, up, -0.10f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                       Tools->txPowerStop() : Tools->txPowerMode(sp, 2.0, MODE_V); // Ток задать любой

  //     tmp = ki * Tools->pMult;
  // Serial.print("parKI=0x");  Serial.println((short) tmp, HEX);

        break;
      default:;
    }
    Display->showVolt(Tools->getRealVoltage(), 3);
    Display->showPidV(ki, 2);
    Display->showMode((char*)"        KI         ");
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MLoadKd, ввод параметра KD PID-регулятора напряжения ========= 
  MLoadKd::MLoadKd(MTools * Tools) : MState(Tools)
  {
      // Индикация 
    Display->showHelp((char*)"  P*-SP  B*-SAVE  ");
  }
  MState * MLoadKd::fsm()
  {
    Tools->chargeCalculations();                        // Подсчет отданных ампер-часов.
    switch (Keyboard->getKey())
    {
      case MKeyboard::C_LONG_CLICK: Board->buzzerOn();  return new MStop(Tools);
      case MKeyboard::P_LONG_CLICK: Board->buzzerOn();  return new MLoadSp(Tools);
      case MKeyboard::P_CLICK: Board->buzzerOn();       
        Tools->writeNvsFloat("pidtest", "kdV", kd);     return new MLoadKi(Tools);      
      case MKeyboard::B_CLICK: Board->buzzerOn();
        Tools->writeNvsFloat("pidtest", "kdV", kd);     return new MLoadSp(Tools);
      case MKeyboard::B_LONG_CLICK: Board->buzzerOn();  return new MSaveProf(Tools);
      case MKeyboard::UP_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, 0.01f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;     
      case MKeyboard::UP_LONG_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, 0.10f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
      case MKeyboard::DN_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, -0.01f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
      case MKeyboard::DN_LONG_CLICK: Board->buzzerOn();
        kd = Tools->updnFloat(kd, dn, up, -0.10f);
        Tools->txSetPidCoeffV(kp, ki, kd);
        break;
        // Включить (0x22 )или отключить (0x21) подачу напряжения на клеммы
      case MKeyboard::C_CLICK: Board->buzzerOn();
        (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                       Tools->txPowerStop() : Tools->txPowerMode(sp, 2.0, MODE_V); // Ток задать любой
        break;
      default:;
    }
    Display->showVolt(Tools->getRealVoltage(), 3);
    Display->showPidV(kd, 2);
    Display->showMode((char*)"        KD         ");
    (Tools->getState() == Tools->getStatusPidVoltage()) ? 
                                     Board->ledsGreen() : Board->ledsRed();
    Display->showDuration(Tools->getChargeTimeCounter(), MDisplay::SEC);
    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MSaveProf, сохранение профиля под выбранным номером ========== 
  MSaveProf::MSaveProf(MTools * Tools) : MState(Tools)
  {
    Display->showHelp((char*)"  P*-SP  B*-SAVE  ");
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
            Tools->writeNvsFloat("profil1", "kpV", MConst::fixedKpV);
            Tools->writeNvsFloat("profil1", "kiV", MConst::fixedKiV);
            Tools->writeNvsFloat("profil1", "kdV", MConst::fixedKdV);
            break;
          case 2:
            Tools->writeNvsFloat("profil2", "kpV", MConst::fixedKpV);
            Tools->writeNvsFloat("profil2", "kiV", MConst::fixedKiV);
            Tools->writeNvsFloat("profil2", "kdV", MConst::fixedKdV);
            break;
          case 3:
            Tools->writeNvsFloat("profil3", "kpV", MConst::fixedKpV);
            Tools->writeNvsFloat("profil3", "kiV", MConst::fixedKiV);
            Tools->writeNvsFloat("profil3", "kdV", MConst::fixedKdV);
            break;
          default:;
        }
      default:;
    }
    Display->showMode((char*)"  PROF = ", (float)prof);

    Display->showAh(Tools->getAhCharge());
    return this;
  };



  //========== MStop, завершение режима U-PID ================================= 
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

    
    //Display->showAmp (Tools->getRealCurrent(), 3);
    Tools->showAmp(Tools->getRealCurrent(), 3, 2);
    //Tools->showAmp(Tools->getRealCurrent(), 3, 3);
    return this;
  };



  //========== MExit, выход из режима U-PID =================================== 
  MExit::MExit(MTools * Tools) : MState(Tools)
  {
    Display->showMode((char*)"     U-PID OFF    ");
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
      Display->showMode((char*)"       U-PID:      ");
      Display->showHelp((char*)"  SP KP KI KD PRF  ");  return nullptr;
    default:;
    }
    return this;
  };

};
 