#ifndef _IPIDFSM_H_
#define _IPIDFSM_H_

#include "state/mstate.h"

namespace MIPid
{
    // Режимы работы PID-регулятора
  enum mode {MODE_OFF = 0, MODE_V, MODE_I, MODE_D, MODE_AUTO};

  struct MConst
  {
    static constexpr float fixedSpV   = 14.0;
    static constexpr float fixedSpI   =  0.5;
    static constexpr short fixedMode  = MODE_I;
      // 
    static constexpr float fixedKpI =  0.20f;
    static constexpr float fixedKiI =  0.20f;
    static constexpr float fixedKdI =  0.10f;
  };

  //========== MStart, инициализация ========================================
  class MStart : public MState
  {       
    public:
      MStart(MTools * Tools);
      MState * fsm() override;
    private:
      short cnt;
  };

  //========== MClearPidKeys, очистка всех ключей U-PID и I-PID =============
  class MClearPidKeys : public MState
  {
    public:  
      MClearPidKeys(MTools * Tools);
      MState * fsm() override;
    private:
      bool done;
      short settings = 0;
        // min/max для задания имени настроек 
      static constexpr float up = 3;
      static constexpr float dn = 0; 
  };

  //========== MLoadProf, выбор и загрузка профиля ==========================
  class MLoadProf : public MState
  {
    public:  
      MLoadProf(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания номера профиля
      static constexpr float up = 3;
      static constexpr float dn = 1; 
  };

  //========== MLoadSp, ввод порога PID-регулятора тока ===================== 
  class MLoadSp : public MState
  {
    public:  
      MLoadSp(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания тока
      static constexpr float up = 5.50f;
      static constexpr float dn = 0.05f; 
  };

  //========== MLoadKp, ввод параметра KP PID-регулятора тока =============== 
  class MLoadKp : public MState
  {
    public:  
      MLoadKp(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 0.24f;
      static constexpr float dn = 0.01f; 
  };

  //========== MLoadKi, ввод параметра KI PID-регулятора тока =============== 
  class MLoadKi : public MState
  {
    public:  
      MLoadKi(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 0.24f;
      static constexpr float dn = 0.00f;
  };

  //========== MLoadKd, ввод параметра KD PID-регулятора тока =============== 
  class MLoadKd : public MState
  {
    public:  
      MLoadKd(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 0.24f;
      static constexpr float dn = 0.00f;
  };

  //========== MSaveProf, сохранение профиля под выбранным номером ========== 
  class MSaveProf : public MState
  {
    public:  
      MSaveProf(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания номера профиля
      static constexpr float up = 3;
      static constexpr float dn = 1; 
  };

  //========== MStop, завершение режима I-PID ================================= 
  class MStop : public MState
  {
    public:  
      MStop(MTools * Tools);
      MState * fsm() override;
  };

  //========== MExit, выход из режима I-PID =================================== 
  class MExit : public MState
  {
    public:
      MExit(MTools * Tools);
      MState * fsm() override;
  };
};

#endif  // !_IPIDFSM_H_
