#pragma once
#include <iostream>

class Processor;
class Mainboard;
class PwrSupUnit;
class PhosphorusDisp;
class Kboard;
//this for the type of operation
enum opcodesForAEC
{
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_CMP, OP_NONE
};

class SignalVector
{
public:
    bool IS_VALID, USES_AEC, IS_MEMORY_READ, IS_MEMORY_WRITE, IS_BRANCH, BRANCH_ON_ZERO, BRANCH_ON_NOT_ZERO, WRITES_TO_REGISTER, IS_4BIT_IMMEDIATE, IS_16BIT_IMMEDIATE;
    opcodesForAEC AEC_OPERATION;
    SignalVector();
};

class StrBank
{
private:
    unsigned short registArr[8];
    unsigned short pC;
    unsigned short iR;
    unsigned char flags;
    const int counterPositionZ, counterPositionN, counterPositionP, counterPositionHalt;

public:
    StrBank();
    unsigned short readRegIdx(int idx);
    void writeRegIdx(int idx, unsigned short result);
    unsigned short getterForPC();
    unsigned short getterForIR();
    unsigned char getterForFlags();
    void setterForPC(unsigned short adr);
    void setterForIR(unsigned short inst);
    void setterForFlags(unsigned char flag);
    void flagUpdateOnComparison(unsigned short registerOne, unsigned short registerTwo);
    void thermalShutNow();
};
class ArithmeticLogic
{
public:
    unsigned short execute(opcodesForAEC operation, unsigned short operandOne, unsigned short operandTwo, StrBank& bank);
};
class MemoryModule
{
private:
    unsigned char memoryArr[3840];
public:
    MemoryModule();
    unsigned char read(unsigned short adr);
    void write(unsigned short adr, unsigned char data);
    void loadRawBinary(unsigned short adr, unsigned short word);
};
class Kboard
{
private:
    char* buf;
    int size;
    int cap;
    int readIdx;
    int writeIdx;
    int currentCount;
    void enterChar(char c);
    void block();
public:
    Kboard(int size = 128);
    ~Kboard();
    char extract();
    unsigned char readCh();
    unsigned char readingInteger();
};
class PhosphorusDisp
{
private:
    char screen[3000];
    int bufCount;
public:
    PhosphorusDisp();
    int getBufCount();
    void displayToChar(unsigned char data);
    void displayToInt(unsigned char pLOAD);
    void display();
};
class Mainboard
{//these members are basically aggregated into the main board
private:
    Processor* centralPU;
    MemoryModule* randomAM; //ram
    PhosphorusDisp* graphicsPU; //graphics processor unit
    Kboard* kboard;
    PwrSupUnit* powerSupunit;
    bool systPwr;
public:
    unsigned short adrStream;
    unsigned char dataStream;
    bool isReading;
    bool isWriting;
    Mainboard();
    bool plugInTheCPU(Processor* p);
    bool plugInTheRAM(MemoryModule* m);
    bool plugInTheGPU(PhosphorusDisp* g);
    bool plugInTheKeyboard(Kboard* k);
    bool plugInThePSU(PwrSupUnit* p);
    void killPower();
    void dataProcessing(unsigned short adr, bool writeEnable);
    void pulseClock();
    bool getSystemPower();
};

class PwrSupUnit
{
private:
    double maxCapacity;
    Mainboard* mb;
public:
    PwrSupUnit(double capacity, Mainboard* mainboardAddress);
    void checkPower(double currentDraw);
};
class Processor
{
private:
    StrBank bank;
    ArithmeticLogic alu;
    Mainboard* mainboard;
    SignalVector box[256];
    unsigned char cache[16];
    unsigned short BA;//the starting address/the base address
    bool isValid;
    unsigned short nextsixteen;
    unsigned char readFromCache(unsigned short address);
public:
    Processor();
    double cTemp;
    char choice;
    void connectToMainboard(Mainboard* mb);
    void oneIter();
    bool isHalted();
    void displayDmp();
};