#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

class MTools;
class MBoard;
class MDisplay;
class MState;
class MSupervisor;

class MDispatcher
{
  public:
    enum MODES
    {
      BOOT = 0,            // режим синхронизации
      OPTIONS,             // режим ввода настроек
      UPID,                // режим настройки регулятора по напряжению
      IPID,                // режим настройки регулятора по току
      DPID,                // режим настройки регулятора по току разряда
      TEMPLATE,            // шаблон режима 
      CCCV,                // режим заряда "постоянный ток / постоянное напряжение"
      CCCVT,               // режим заряда CC/CV "технологический"
      //DISCHARGE,           // режим разряда
      DEVICE,              // режим заводских регулировок
    };

  public:
    MDispatcher(MTools * tools);

    void run();
    void delegateWork();
    void textMode(short modeSelection);

  private:
    MTools    * Tools;
    MBoard    * Board;
    MDisplay  * Display;
    MState    * State = nullptr;
    MSupervisor * Supervisor;

    bool latrus;                      // Зарезервировано
    bool sync = true;                 // Начать с синхронизации параметров
    short modeSelection;              // Выбранный режим работы
};

#endif //_DISPATCHER_H_
