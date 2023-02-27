#ifndef _MMEASURE_H_
#define _MMEASURE_H_

#include "state/mstate.h"

class MTools;
class MBoard;
class MState;

class MMeasure
{
  public:
    MMeasure(MTools * tools);

    void run();
    //void delegateWork();

  private:
    MTools * Tools = nullptr;
    MBoard * Board = nullptr;

    MState * State = nullptr;
};

namespace MMeasureStates
{
    // class MAdcVI : public MState
    // {
    //     public:     
    //         MAdcVI(MTools * Tools) : MState(Tools) {}   
    //         virtual MState * fsm() override;
    //     private:
    //         int collectV    = 0;
    //         int collectI    = 0;
    //         int collectI3   = 0;
    //         int averageV    = 0;
    //         int averageI    = 0;
    //         int averageI3   = 0;
    //         int cntVI       = 0;
    //         float volt      = 0.0f;
    // };
       
    class MAdcCelsius : public MState
    {
        public:   
            MAdcCelsius(MTools * Tools) : MState(Tools) {}     
            MState * fsm() override;
    };
    
    class MAdcPowerGood : public MState
    {
        public:   
            MAdcPowerGood(MTools * Tools) : MState(Tools) {}     
            MState * fsm() override;
    };




    class MAdcKeyboard : public MState
    {
        public:   
            MAdcKeyboard(MTools * Tools) : MState(Tools) {}     
            MState * fsm() override;
    };
    
};

#endif  // _MMEASURE_H_
