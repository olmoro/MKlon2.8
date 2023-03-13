#ifndef _CCCVFSM_H_
#define _CCCVFSM_H_

#include "state/mstate.h"

namespace MCccv
{
  struct MConst
  {                               // 20230311     20230204
    static constexpr float fixedKpV =  1.0;       //0.100f;
    static constexpr float fixedKiV =  1.8;       //0.240f;
    static constexpr float fixedKdV =  0.1;       //0.000f;
    static constexpr float fixedKpI =  1.0;       //0.100f;
    static constexpr float fixedKiI =  1.8;       //0.240f;
    static constexpr float fixedKdI =  0.1;       //0.000f;
  };

    // Режимы работы PID-регулятора
  enum mode {MODE_OFF = 0, MODE_U, MODE_I, MODE_D, MODE_AUTO};

  class MStart : public MState
  {       
    public:
      MStart(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr float nominal_v_fixed  = 12.0f;
      static constexpr float capacity_fixed   = 55.0f;
      static constexpr float voltageMaxFactor =  1.234f;    // 12v  * 1.234 = 14.8v
      static constexpr float voltageMinFactor =  0.890f;    // 12v  * 0.89  = 10.7v
      static constexpr float currentMaxFactor =  0.100f;    // 55ah * 0.1   = 5,5A 
      static constexpr float currentMinFactor =  0.050f;    // 55ah * 0.05  = 2.75A
      short cnt;
      float voltageNom = 12.0;
      float capacity   = 55.0;
  };

  class MClearCccvKeys : public MState
  {
    public:  
      MClearCccvKeys(MTools * Tools);
      MState * fsm() override;
    private:
      short cnt;
      bool done;
  };

  class MSetCurrentMax : public MState
  {
    public:   
      MSetCurrentMax(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования max тока
      static constexpr float up = 3.0f;
      static constexpr float dn = 0.2f;
  };
  
  class MSetVoltageMax : public MState
  {
    public:   
      MSetVoltageMax(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования max напряжения
      static constexpr float up = 14.4f;
      static constexpr float dn = 10.0f;
  };

  class MSetCurrentMin : public MState
  {
    public:     
      MSetCurrentMin(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования min тока
      static constexpr float up = 2.0f;
      static constexpr float dn = 0.2f;
  };

  class MSetVoltageMin : public MState
  {
    public:     
      MSetVoltageMin(MTools * Tools);
      MState * fsm() override;
    private:
        // Пределы регулирования min напряжения
      static constexpr float up = 13.2f;
      static constexpr float dn = 10.0f;
  };

  class MPostpone : public MState
  {
    public:   
      MPostpone(MTools * Tools);
      MState * fsm() override;
    private:
      float kp, ki, kd;
  };

  class MImportPID : public MState
  {
    public:   
      MImportPID(MTools * Tools);
      MState * fsm() override;
    private:
      short k = 0;
      float kpV, kiV, kdV, kpI, kiI, kdI;
  };

  class MUpCurrent : public MState
  {
    public:   
      MUpCurrent(MTools * Tools);
      MState * fsm() override;
  };

  class MKeepVmax : public MState
  {
    public: 
      MKeepVmax(MTools * Tools);
      MState * fsm() override;
  };

  class MKeepVmin : public MState
  {
    public:   
      MKeepVmin(MTools * Tools);
      MState * fsm() override;
    private:
      short timeOut;    // Максимальное время заряда
  };

  class MStop : public MState
  {
    public:  
      MStop(MTools * Tools);
      MState * fsm() override;
  };

  class MExit : public MState
  {
    public:
      MExit(MTools * Tools);
      MState * fsm() override;
  };

};

#endif  // !_CCCVFSM_H_
