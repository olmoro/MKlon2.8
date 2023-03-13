#ifndef _BOOTFSM_H_
#define _BOOTFSM_H_

#include "state/mstate.h"

namespace MBoot
{
  // struct MConst
  // {                               // 20230214
  //   static constexpr unsigned short   factor_default_v = 0x5326                       - OK
  //     smooth_default_v =      4                       - OK
  //     shift_default_v  =      0                       - OK
  //     factor_default_i = 0x7918                       - OK
  //     smooth_default_i =      2                       - OK
  //     shift_default_i  =      0                       - OK


  //   static constexpr float fixedKpV = 0.050f;     //0.06f;    //  15
  //   static constexpr float fixedKiV = 0.050f;     //0.40f;    // 
  //   static constexpr float fixedKdV = 0.000f;     //0.00f;    //   0 0x0000
  //   static constexpr float fixedKpI = 0.050f;     //0.08f;    //  20
  //   static constexpr float fixedKiI = 0.050f;     //0.40f;    //  
  //   static constexpr float fixedKdI = 0.000f;     //0.00f;    //   0 0x0000  
  // };





  class MStart : public MState
  {       
    public:
      MStart(MTools * Tools);
      MState * fsm() override;
  };

  class MTxPowerStop : public MState
  {
    public:   
      MTxPowerStop(MTools * Tools);
      MState * fsm() override;
  };

  class MTxSetFrequency : public MState
  {
    public:   
      MTxSetFrequency(MTools * Tools);
      MState * fsm() override;
    private:
      short freq[6]{ 10, 20, 50, 100, 200, 250 };
      short i;
      static constexpr short fixed = 3;
  };
  
  class MTxGetTreaty : public MState
  {
    public:   
      MTxGetTreaty(MTools * Tools);
      MState * fsm() override;
  };

  class MTxsetFactorV : public MState
  {
    public:   
      MTxsetFactorV(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr unsigned short fixed = 0x5326;
  };

  class MTxSmoothV : public MState
  {
    public:   
      MTxSmoothV(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr unsigned short fixed = 0x0004;
  };

  class MTxShiftV : public MState
  {
    public:   
      MTxShiftV(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr unsigned short fixed = 0x0000;
  };

  class MTxFactorI : public MState
  {
    public:   
      MTxFactorI(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr unsigned short fixed = 0x7918;
  };

  class MTxSmoothI : public MState
  {
    public:   
      MTxSmoothI(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr unsigned short fixed = 4;    //0x0002;
  };
  
  class MTxShiftI : public MState
  {
    public:   
      MTxShiftI(MTools * Tools);
      MState * fsm() override;
    private:
      static constexpr unsigned short fixed = 0x0000;
  };

  class MExit : public MState
  {
    public:
      MExit(MTools * Tools);
      MState * fsm() override;
  };

};

#endif  //_BOOTFSM_H_
