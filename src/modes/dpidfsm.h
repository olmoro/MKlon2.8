#ifndef _DPIDFSM_H_
#define _DPIDFSM_H_

#include "state/mstate.h"

namespace MDPid
{
    // Режимы работы PID-регулятора
  enum mode {MODE_OFF = 0, MODE_V, MODE_I, MODE_D, MODE_AUTO};

  struct MConst
  {
    /* Параметры передаются в команде как положительные  20230301*/
    static constexpr float fixedSpD   =  1.0;
    static constexpr short fixedMode  = MODE_D;
      // 
    static constexpr float fixedKpD =  1.0f;
    static constexpr float fixedKiD =  1.8f;
    static constexpr float fixedKdD =  0.1f;
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

  //========== MClearPidKeys, очистка всех ключей U-, I- и D-PID ============
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

  //========== MLoadSp, ввод порога PID-регулятора ========================= 
  class MLoadSp : public MState
  {
    public:  
      MLoadSp(MTools * Tools);
      MState * fsm() override;
    private:
      // min/max для задания тока
      static constexpr float up = 2.0f;
      static constexpr float dn = 0.2f; 
  };

  //========== MLoadKp, ввод параметра KP PID-регулятора =================== 
  class MLoadKp : public MState
  {
    public:  
      MLoadKp(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 10.0f;
      static constexpr float dn =  0.1f; 
  };

  //========== MLoadKi, ввод параметра KI PID-регулятора =================== 
  class MLoadKi : public MState
  {
    public:  
      MLoadKi(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 10.0f;
      static constexpr float dn =  0.0f; 
  };

  //========== MLoadKd, ввод параметра KD PID-регулятора =================== 
  class MLoadKd : public MState
  {
    public:  
      MLoadKd(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float up = 10.0f;
      static constexpr float dn =  0.0f; 
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

  //========== MStop, завершение режима D-PID ================================= 
  class MStop : public MState
  {
    public:  
      MStop(MTools * Tools);
      MState * fsm() override;
  };

  //========== MExit, выход из режима D-PID =================================== 
  class MExit : public MState
  {
    public:
      MExit(MTools * Tools);
      MState * fsm() override;
  };
};

#endif  // !_DPIDFSM_H_
