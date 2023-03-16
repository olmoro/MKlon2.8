#ifndef _UPIDFSM_H_
#define _UPIDFSM_H_

#include "state/mstate.h"

namespace MUPid
{
    // Режимы работы PID-регулятора
  enum mode {MODE_OFF = 0, MODE_V, MODE_I, MODE_D, MODE_AUTO};

  struct MConst
  {
    static constexpr float fixedSpV   = 14.0;
    static constexpr float fixedSpI   =  2.0;
    static constexpr short fixedMode  = MODE_V;
      //
    static constexpr float fixedKpV =  0.20f;
    static constexpr float fixedKiV =  0.03f;
    static constexpr float fixedKdV =  0.22f;
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

  //========== MClearPidKeys, очистка ключей U-PID и I-PID ==================
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

  //========== MLoadSp, ввод порога PID-регулятора напряжения =============== 
  class MLoadSp : public MState
  {
    public:  
      MLoadSp(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания напряжения
    static constexpr float up = 16.5f;
    static constexpr float dn =  2.0f; 
  };

  //========== MLoadKp, ввод параметра KP PID-регулятора напряжения ========= 
  class MLoadKp : public MState
  {
    public:  
      MLoadKp(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 0.24f;
      static constexpr float dn = 0.01f; 
  };

  //========== MLoadKi, ввод параметра KI PID-регулятора напряжения ========= 
  class MLoadKi : public MState
  {
    public:  
      MLoadKi(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 0.24f;
      static constexpr float dn = 0.00f;
  };

  //========== MLoadKd, ввод параметра KD PID-регулятора напряжения ========= 
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

  //========== MStop, завершение режима U-PID ================================= 
  class MStop : public MState
  {
    public:  
      MStop(MTools * Tools);
      MState * fsm() override;
  };

  //========== MExit, выход из режима U-PID =================================== 
  class MExit : public MState
  {
    public:
      MExit(MTools * Tools);
      MState * fsm() override;
  };
};

#endif  // !_UPIDFSM_H_
