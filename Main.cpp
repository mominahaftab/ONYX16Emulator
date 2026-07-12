#include <iostream>
#include <iomanip>
#include "Header.h"
#include "Interpreter.h"
using namespace std;

SignalVector::SignalVector()
{
	IS_VALID = false;
	USES_AEC = false;
	AEC_OPERATION = OP_NONE;
	IS_MEMORY_READ = false;
	IS_MEMORY_WRITE = false;
	IS_BRANCH = false;
	BRANCH_ON_ZERO = false;
	BRANCH_ON_NOT_ZERO = false;
	WRITES_TO_REGISTER = false;
	IS_4BIT_IMMEDIATE = false;
	IS_16BIT_IMMEDIATE = false;
}
MemoryModule::MemoryModule()
{
	for (int i = 0; i < 3840; i++)
	{
		*(memoryArr + i) = 0;
	}
}
//Adding data for the Storage Bank. 
//first off, created constructor that sets the array of registers to 0 just so garbage isn't stored at each index
//the loop iterates through the array
//member initialiser list to initialise the member variables that are const
StrBank::StrBank() : counterPositionZ(0), counterPositionN(1), counterPositionP(2), counterPositionHalt(7)
{
	//assigning
	int sizeR = 8;
	for (int i = 0; i < sizeR; i++)
	{	//each block in the array represents one register from R0 to R7 
		//these registers right next to ALU (the Arithmetic logic unit) inside the CPU
		//the array is unsigned short because it holds 16 bit value
		//the data inside RAM is very slow to process let alone be undergoing operations
		//therefor, the RAM places data onto the registers, performs the operation and the result goes back to the RAM
		*(registArr + i) = 0x000;
	}
	//represent the program counter, instruction register, and flags.
	//all initialised
	pC = 0x0000;
	iR = 0x0000;
	flags = 0x00;
}
unsigned short StrBank::readRegIdx(int idx)
{
	//checking for bounds
	//according to the assignment it's said that if out of range, return 0xFFFF
	//this to extract the value residing inside the register at index "idx" on the array of registers
	if (idx < 0 || idx > 7)
	{
		cout << "[CPU FAULT] Invalid Register Read!" << endl;
		return 0xFFFF;
	}

	return *(registArr + idx);
}
//to place the result after applying the operation back in the register index
void StrBank::writeRegIdx(int idx, unsigned short result)
{
	if (idx < 0 || idx > 7) //checking for bounds, if outside, display error
	{
		cout << "[CPU FAULT] Invalid Register Write!" << endl;
	}
	else if (idx >= 0 && idx <= 7)
	{
		*(registArr + idx) = result; //if within the range, then insert the result at the register index
	}
}
//having getters and setters for the private members
//getters
unsigned short StrBank::getterForPC()
{
	return pC;
}
unsigned short StrBank::getterForIR()
{
	return iR;
}
unsigned char StrBank::getterForFlags()
{
	return flags;
}
//setters
//they are void return type since only assigning takes place of the private members of the class itself
void StrBank::setterForPC(unsigned short adr)
{
	pC = adr; //assigning pC the address of the byte of next intruction that is to be caught
}
void StrBank::setterForIR(unsigned short inst)
{
	iR = inst; //assigning iR the current instruction 
}
void StrBank::setterForFlags(unsigned char flag)
{
	this->flags = flag; //assigning pC the address of the byte of next intruction that is to be caught
}
//this for the comparison or the CMP function
//this function accounts for the clearing of the other two bits that are not involved directly in the comparison
void StrBank::flagUpdateOnComparison(unsigned short registerOne, unsigned short registerTwo)
{
	//clearing up the bits other than the one being referenced to
	if (registerOne == registerTwo)
	{
		flags = flags | (1 << counterPositionZ);
		flags = flags & ~(1 << counterPositionN);
		flags = flags & ~(1 << counterPositionP);
	}
	else if (registerTwo > registerOne)
	{
		flags = flags | (1 << counterPositionN);
		flags = flags & ~(1 << counterPositionZ);
		flags = flags & ~(1 << counterPositionP);
	}
	else if (registerOne > registerTwo)
	{
		flags = flags | (1 << counterPositionP);
		flags = flags & ~(1 << counterPositionZ);
		flags = flags & ~(1 << counterPositionN);
	}
}
//this function accounts for the thermal shut down
//when the shut down occurs, the bit is set ie becomes 1 and never changed after that
void StrBank::thermalShutNow()
{
	cout << "\n[CRITICAL ERROR] Thermal threshold exceeded\n";
	flags = flags | (1 << counterPositionHalt);
}
unsigned short ArithmeticLogic::execute(opcodesForAEC operation, unsigned short operandOne, unsigned short operandTwo, StrBank& bank)
{	//evaluate the operation first and then perform that operation onto the operands
	unsigned short answer = 0;
	//now the zero flag being updated logic
	//if the answer is 0, the flag is set to ONE else cleared to ZERO
	(operation == OP_CMP) ? (bank.flagUpdateOnComparison(operandOne, operandTwo), 0) :
		(answer = (operation == OP_ADD) ? (operandOne + operandTwo) : ((operation == OP_SUB) ? (operandOne - operandTwo) : ((operation == OP_MUL) ? (operandOne * operandTwo) : ((operation == OP_DIV) ? ((operandTwo == 0) ? 0 : (operandOne / operandTwo)) : 0))),
			(answer == 0) ? (bank.setterForFlags(bank.getterForFlags() | 0x01)) : (bank.setterForFlags(bank.getterForFlags() & ~0x01)), 0);
	return answer;
}
unsigned char MemoryModule::read(unsigned short adr)
{	//the address lies within the range, then plug in the address into the memory
	if (adr > 0x0EFF)
	{
		return 0xFF;
	}
	return*(memoryArr + adr);
}
void MemoryModule::write(unsigned short adr, unsigned char data)
{
	//the address lies within the range, then plug in the data into the memory at address location
	if (adr >= 0x0000 && adr <= 0x0EFF)
	{
		*(memoryArr + adr) = data;
	}
}
//the word has 16 bits and is needed to be broken into two 8 bit pieces
void MemoryModule::loadRawBinary(unsigned short adr, unsigned short word)
{
	//the first check to ensure that address + 1 is also inside the ram capacity
	if (adr + 1 <= 0x0EFF) //the valid range check 
	{
		unsigned char upper = 0;//here bits from 8 to 15
		unsigned char lower = 0;//here bits from 0 to 7
		//loop for storing the lower byte (0 to 7 byte basically)
		for (int i = 0; i <= 7; i++)
		{	//& it with 1 to check if the resultant is a 1. if yes, store one in the variable
			//zeros however will be exactly where they were originally at
			if ((word >> i) & 1)
			{
				lower = lower | (1 << i);
			}
		}
		//same thing for the upper 
		for (int i = 0; i <= 7; i++)
		{

			//& it with 1 to check if the top bit is a 1
			if ((word >> (i + 8)) & 1)
			{

				//store the result in the new variable
				upper = upper | (1 << i);
			}
		}
		//now putting in the upper and lower in the memory
		*(memoryArr + adr) = upper;
		*(memoryArr + adr + 1) = lower;
	}
}
//constructor for the key board class that sets the capacity, creates buffer array of that capacitty
Kboard::Kboard(int size) : size(size), cap(size), readIdx(0), writeIdx(0), currentCount(0)
{
	buf = new char[cap];
}
//the distructor manages the cleanup for the dynamic memory
Kboard::~Kboard()
{
	delete[] buf;
}
//this function handles the blockage on the screen that is triggered when the buffer is empty
void Kboard::block()
{	//printing the message on the screen to ask user for input to fill in the buffer
	cout << "\n[Hardware Interrupt] Awaiting Keyboard Input: ";

	char input[300];
	cin.getline(input, 300);
	//additional safety check that ensures if the string is still null
	if (*(input) == '\0')
	{
		cin.getline(input, 300);
	}
	//for as long as the end of the input buffer is NOT null
	bool isNotNull = true;
	int i = 0;
	while (isNotNull && i < 300)
	{
		isNotNull = (*(input + i) != '\0') ? true : false;
		if (isNotNull)
		{
			enterChar(*(input + i));
			i++;
		}
	}
	enterChar(' ');
}
void Kboard::enterChar(char c)
{	//for as long as the capacity isnot reached, characters are drawn into the buffer
	if (currentCount < cap)
	{
		*(buf + writeIdx) = c;
		writeIdx++;
		currentCount++;
		//when the capactiy exceeds, bring it back to col number 0
		if (writeIdx == cap)
		{
			writeIdx = 0;
		}
	}
}
char Kboard::extract()
{	//if no characters to extract
	if (currentCount == 0)
	{
		block();
	}
	char c = *(buf + readIdx);
	readIdx++;
	currentCount--;
	//when column capacity hits, reset to 0
	if (readIdx == cap)
	{
		readIdx = 0;
	}
	return c;
}
unsigned char Kboard::readCh()
{
	return extract();
}
unsigned char Kboard::readingInteger()
{
	char temp[20]; //static array of set size
	int idx = 0; //tracker of the index
	bool spaceNotHit = true;
	//the loop runs for as long as no space is hit
	while (spaceNotHit)
	{	//first the character is extracted
		char charExtracted = extract();
		//these are the irrelevant characters which are needed to be skipped.
		//so if not these, and then within the range, then inside the temp array the chars are inserted
		if (charExtracted != ' ' && charExtracted != '\r' && charExtracted != '\n' && charExtracted != '\t')
		{
			if (idx < 16)
			{
				*(temp + idx) = charExtracted;
				idx++;
			}
		}
		else
		{
			if (idx > 0)
			{
				spaceNotHit = false;
			}
		}
	}
	//this loop helps convert the decimal number into character 
	int sum = 0;
	for (int i = 0; i < idx; i++)
	{
		int multiple = 1;
		for (int j = 0; j < idx - i - 1; j++)
		{
			multiple = multiple * 10;
		}
		//accumulation in sum variable
		sum += (*(temp + i) - '0') * multiple;
	}
	return (unsigned char)(sum % 256);
}
//ensuring nothing is printed on the screen first
PhosphorusDisp::PhosphorusDisp() : bufCount(0)
{
	for (int i = 0; i < 3000; i++)
	{
		*(screen + i) = '\0';
	}
}
void PhosphorusDisp::displayToChar(unsigned char data)
{	//storing rhe characters one by one into the screen
	bool hasCapacity = (bufCount < 2999);
	if (hasCapacity == true)
	{
		*(screen + bufCount) = (char)data;
		*(screen + bufCount + 1) = '\0';
		bufCount++;
	}
}
void PhosphorusDisp::display()
{
	cout << "\n\033[92m+--------------------------------+\n";
	cout << "| PHOSPHOR CRT DISPLAY RENDER    |\n";
	cout << "+--------------------------------+\n";
	cout << "|";
	int i = 0;
	int charsPerL = 0;
	int rowsPrinted = 0;
	while (*(screen + i) != '\0' && rowsPrinted < 16)
	{
		char c = *(screen + i);
		if (c == '\r')
		{
			i++;
			continue;
		}
		if (c == '\n')
		{
			int spacesToEnter = 32 - charsPerL;
			int s = 0;
			while (s < spacesToEnter)
			{
				cout << " ";
				s++;
			}
			cout << "|\n";
			rowsPrinted++;
			//resetting the chars per line
			charsPerL = 0;
			if (rowsPrinted < 16)
			{
				cout << "|";
			}
		}
		//when the end isn't reach so the printin proceeeds
		else
		{
			cout << c;
			charsPerL++;
			if (charsPerL == 32)
			{
				cout << "|\n";
				rowsPrinted++;
				charsPerL = 0;
				if (rowsPrinted < 16)
				{
					cout << "|";
				}
			}
		}
		i++;
	}
	if (rowsPrinted < 16)
	{
		int p = charsPerL;
		while (p < 32)
		{
			cout << " ";
			p++;
		}
		cout << "|\n";
	}
	int r = 0;
	while (r < 5)
	{
		cout << "|                                |\n";
		r++;
	}
	cout << "+--------------------------------+\033[0m\n";
}
void Processor::displayDmp()
{
	cout << "\n----------------------------------------------------------------------\n\n";
	cout << "=== PROCESSOR STATE DUMP ===\n";
	cout << "Temperature: " << cTemp << " C (Max: 90 C)\n\n";
	cout << "--- INTERNAL REGISTERS (16-bit) ---\n";

	for (int i = 0; i < 8; i++)
	{
		unsigned short val = bank.readRegIdx(i);
		cout << "R" << i << ": 0x";
		int digitCount = 0;
		unsigned short temp = val;
		if (temp == 0)
		{
			digitCount = 1;
		}
		while (temp > 0)
		{
			temp = temp >> 4;
			digitCount++;
		}
		int diff = 4 - digitCount;
		int p = 0;
		while (p < diff)
		{
			cout << "0";
			p++;
		}
		cout << hex << val << dec << endl;
	}
	unsigned short pcountVal = bank.getterForPC();
	cout << "PC  : 0x";
	int digitCount = 0;
	unsigned short temp = pcountVal;
	if (temp == 0)
	{
		digitCount = 1;
	}
	while (temp > 0)
	{
		temp = temp >> 4;
		digitCount++;
	}
	int diff = 4 - digitCount;
	int p = 0;
	while (p < diff)
	{
		cout << "0";
		p++;
	}
	cout << hex << pcountVal << dec << endl;

	unsigned short regVal = bank.getterForIR();
	cout << "IR  : 0x";
	digitCount = 0;
	temp = regVal;
	if (temp == 0)
	{
		digitCount = 1;
	}
	while (temp > 0)
	{
		temp = temp >> 4;
		digitCount++;
	}
	diff = 4 - digitCount;
	p = 0;
	while (p < diff)
	{
		cout << "0";
		p++;
	}
	cout << hex << regVal << dec << endl;

	unsigned char flgVal = bank.getterForFlags();
	cout << "FLAGS: 0x";
	digitCount = 0;
	temp = flgVal;
	if (temp == 0)
	{
		digitCount = 1;
	}
	while (temp > 0)
	{
		temp = temp >> 4;
		digitCount++;
	}
	diff = 2 - digitCount;
	p = 0;
	while (p < diff)
	{
		cout << "0";
		p++;
	}
	cout << hex << (int)flgVal << dec << endl;
	cout << "====================================\n";
}
void PhosphorusDisp::displayToInt(unsigned char pLOAD)
{
	if (pLOAD == 0)
	{
		displayToChar('0');
		return;
	}

	char tempArray[5];
	int idxTemp = 0;
	while (pLOAD > 0)
	{
		int lastDigit = pLOAD % 10;
		char digitChar = lastDigit + '0';
		*(tempArray + idxTemp) = digitChar;
		idxTemp++;
		pLOAD = pLOAD / 10;
	}
	//now ensuring that the function recieves the character from the end 
	for (int i = idxTemp - 1; i >= 0; i--)
	{
		displayToChar(*(tempArray + i));
	}
}
//void PhosphorusDisp::displayToChar(unsigned char data)
//{ 
//	cout<<(char)data; 
//}
//constructor for the power supply unit
PwrSupUnit::PwrSupUnit(double capacity, Mainboard* mainboardAddress)
{	//the PSU is aggregated into the mother board. it has the reference to the motherboard
	//it exists even BEFORE the main board /mother board
	maxCapacity = capacity;
	mb = mainboardAddress;
}
void PwrSupUnit::checkPower(double currentCapacity)
{	//this checks that if the current capacity has exceeded the max limit then power killed func on the main board is called
	if (currentCapacity > maxCapacity)
	{
		mb->killPower();
	}
}
//constructor that uses member initialiser list
Mainboard::Mainboard() : centralPU(nullptr), randomAM(nullptr), graphicsPU(nullptr), kboard(nullptr), powerSupunit(nullptr), systPwr(true), adrStream(0), dataStream(0), isReading(false), isWriting(false)
{
}
bool Mainboard::plugInTheCPU(Processor* p)
{
	if (!centralPU)
	{
		centralPU = p;
		return true;
	}
	return false;
}
//when the plugInTheInTheging is needed to take place, first check ensures that it if ram isn't plugInTheged in already
bool Mainboard::plugInTheRAM(MemoryModule* r)
{
	if (!randomAM) //not connected already
	{
		randomAM = r;
		return true;
	}
	return false;
}
bool Mainboard::plugInTheGPU(PhosphorusDisp* g)
{
	if (!graphicsPU) //not connected already
	{
		graphicsPU = g;
		return true;
	}
	return false;
}
bool Mainboard::plugInTheKeyboard(Kboard* k)
{
	if (!kboard) //not connected already
	{
		kboard = k;
		return true;
	}
	return false;
}
bool Mainboard::plugInThePSU(PwrSupUnit* p)
{
	if (!powerSupunit) //not connected already
	{
		powerSupunit = p;
		return true;
	}
	return false;
}
void Mainboard::killPower()
{
	systPwr = false;
	std::cout << "\n[CRITICAL WARNING] Power Draw exceeded PSU Capacity\n";
}
void Mainboard::dataProcessing(unsigned short adr, bool writeEnable)
{	//first checking for the segmentation fault range first
	//if so, print the message
	if (adr >= 0x0F00 && adr <= 0x0FEF)
	{
		if (writeEnable == false) //when nothing to write
		{
			cout << "[HARDWARE FAULT] Segmentation Fault: Read out of bounds" << endl;
			dataStream = 0xFF; //then the data stream becomes 0000 0000
		}
		else
		{
			cout << "[HARDWARE FAULT] Segmentation Fault: Write out of bounds" << endl;
		}
		return;
	}
	//if these are code or data segments that are to be sent to memeory module:
	else if (adr <= 0x0EFF)
	{	//if the data is simply empty ie no ram
		if (randomAM == nullptr)
		{
			return;
		}
		dataStream = (writeEnable == true) ? ((*randomAM).write(adr, dataStream), 0) : ((*randomAM).read(adr));
	}
	//reading at specific addresses
	else if (adr == 0x0FF0)
	{	//for as long as the keybord is NOT null ptr/is active by the user and writing is enabled then;
		if (kboard != nullptr && !writeEnable)
		{
			dataStream = (*kboard).readCh();
		}
	}
	else if (adr == 0x0FF3)
	{
		//for as long as the gpu is NOT null ptr/is active by the user and writing is enabled then;
		if (kboard != nullptr && !writeEnable)
		{
			dataStream = (*kboard).readingInteger();
		}
	}
	else if (adr == 0x0FF1)
	{
		//for as long as the gpu is NOT null ptr/is active by the user and writing is enabled then;
		if (graphicsPU != nullptr && writeEnable)
		{
			(*graphicsPU).displayToChar(dataStream);
		}
	}
	else if (adr == 0x0FF2)
	{
		//for as long as the keybord is NOT null ptr/is active by the user and writing is enabled then;
		if (graphicsPU != nullptr && writeEnable)
		{
			(*graphicsPU).displayToInt(dataStream);
		}
	}
	else
	{
		if (writeEnable == false)
		{
			cout << "[MOTHERBOARD FAULT] Invalid Read Address" << endl;
			dataStream = 0x00;
		}
		else
		{
			cout << "[MOTHERBOARD FAULT] Invalid Write Address" << endl;
		}
	}
}
void Mainboard::pulseClock()
{
	if (systPwr == 0)
	{
		return; //the system is already shut
	}
	if (isReading)
	{
		dataProcessing(adrStream, false);
	}
	else if (isWriting)
	{
		dataProcessing(adrStream, true);
	}
	double powerCurrentlyUsed = 10.0f;
	if (adrStream >= 0x0000 && adrStream <= 0x0EFF)
	{
		//range is representing the ram
		if (isReading || isWriting)
		{
			powerCurrentlyUsed += 0.5;
		}
		else
		{
			powerCurrentlyUsed += 0.1;
		}
	}
	//this is the range for display
	else if (adrStream >= 0x0FF1 && adrStream <= 0x0FF2)
	{
		powerCurrentlyUsed += 15.0;
	}
	if (powerSupunit != nullptr)
	{
		powerSupunit->checkPower(powerCurrentlyUsed);
	}
	isReading = false;
	isWriting = false;
}
bool Mainboard::getSystemPower()
{
	return systPwr;
}

Processor::Processor() : mainboard(nullptr), nextsixteen(0), cTemp(25.0), BA(0), isValid(false)
{
	for (int i = 0; i < 16; i++)
	{
		*(cache + i) = 0;
	}
	//filling the decode matrix based on appendix A
	//all the members are set for each index in the matrix
	box[0x00].IS_VALID = true;
	//this one JAMA or add basically
	box[0x01].IS_VALID = true;
	box[0x01].USES_AEC = true;
	box[0x01].AEC_OPERATION = OP_ADD;
	box[0x01].WRITES_TO_REGISTER = true;
	//this for subtraction/TAFREEK
	box[0x02].IS_VALID = true;
	box[0x02].USES_AEC = true;
	box[0x02].AEC_OPERATION = OP_SUB;
	box[0x02].WRITES_TO_REGISTER = true;
	//for multiplication
	box[0x03].IS_VALID = true;
	box[0x03].USES_AEC = true;
	box[0x03].AEC_OPERATION = OP_MUL;
	box[0x03].WRITES_TO_REGISTER = true;
	//for division
	box[0x04].IS_VALID = true;
	box[0x04].USES_AEC = true;
	box[0x04].AEC_OPERATION = OP_DIV;
	box[0x04].WRITES_TO_REGISTER = true;
	//for comparison
	box[0x0A].IS_VALID = true;
	box[0x0A].USES_AEC = true;
	box[0x0A].AEC_OPERATION = OP_CMP;
	box[0x0A].WRITES_TO_REGISTER = false;
	//caters JMP
	box[0x10].IS_VALID = true;
	box[0x10].IS_BRANCH = true;
	//for JUMP that is due to ZERO bit only 
	box[0x11].IS_VALID = true;
	box[0x11].IS_BRANCH = true;
	box[0x11].BRANCH_ON_ZERO = true;
	//for JUMP that is due to ONE bit only 
	box[0x12].IS_VALID = true;
	box[0x12].IS_BRANCH = true;
	box[0x12].BRANCH_ON_NOT_ZERO = true;
	//when immeduat is 4 bit value
	box[0x1A].IS_VALID = true;
	box[0x1A].IS_4BIT_IMMEDIATE = true;
	box[0x1A].WRITES_TO_REGISTER = true;
	//when immediat eis 16 bit value
	box[0x1B].IS_VALID = true;
	box[0x1B].IS_16BIT_IMMEDIATE = true;
	box[0x1B].WRITES_TO_REGISTER = true;
	//loading from source to destination
	box[0x20].IS_VALID = true;
	box[0x20].IS_MEMORY_READ = true;
	box[0x20].WRITES_TO_REGISTER = true;
	//value written in R[d] to the adr holding in R[s]
	box[0x21].IS_VALID = true;
	box[0x21].IS_MEMORY_WRITE = true;

}
bool Processor::isHalted()
{
	if (mainboard != nullptr && !(*mainboard).getSystemPower())
	{
		return true;
	}
	return (bank.getterForFlags() & 0x80) != 0;
}
void Processor::connectToMainboard(Mainboard* mb)
{
	mainboard = mb;
}

unsigned char Processor::readFromCache(unsigned short rA)
{
	bool isAddress = false;
	int targetIndex = 0;
	unsigned char extractedData = 0x00;
	if (rA >= BA && rA < BA + 16 && isValid)
	{
		for (int i = 0; i < 16; i++)
		{
			if ((BA + i) == rA)
			{
				isAddress = true;
				targetIndex = i;
				break;
			}
		}
		if (isAddress == true)
		{
			extractedData = *(cache + targetIndex);
		}
	}
	else
	{
		BA = (rA / 16) * 16;
		isValid = true;
		for (int i = 0; i < 16; i++)
		{
			(*mainboard).adrStream = BA + i;
			(*mainboard).isReading = true;
			(*mainboard).pulseClock();
			*(cache + i) = (*mainboard).dataStream;
		}
		int idx = rA - BA;
		extractedData = *(cache + idx);
	}
	return extractedData;
}
int PhosphorusDisp::getBufCount()
{
	return bufCount;
}
void Processor::oneIter()
{
	if (isHalted())
	{
		return;
	}
	//fetching the byte address of the next instruction
	unsigned short currentPCounter = bank.getterForPC();
	//extracting the high and the low bytes
	unsigned short highByte = readFromCache(currentPCounter);
	unsigned short lowByte = readFromCache(currentPCounter + 1);
	//now reading off the operation or the command that is needed to be performed
	unsigned short command = (highByte << 8) | lowByte;
	//writing it down in the register
	bank.setterForIR(command);
	if (command == 0x0000)
	{
		cout << "\n[PROCESSOR] End-Of-File (0x0000) reached at PC 0x" << hex << currentPCounter << dec << ". Halting.\n";
		bank.setterForFlags(bank.getterForFlags() | 0x80);
		return;
	}
	//advancing 2 units
	bank.setterForPC(bank.getterForPC() + 2);
	//the nibble division
	unsigned char opC = 0;//holds 8 - 15 bits
	unsigned char d = 0;//holds 4 - 7 bits
	unsigned char s = 0;//holds 0-3 bits
	//loop for opcode (8 to 15 byte basically)
	for (int i = 0; i <= 7; i++)
	{
		//& it with 1 to check if the top bit is a 1
		if ((command >> (i + 8)) & 1)
		{
			//store the result in the new variable
			opC = opC | (1 << i);
		}
	}
	//loop for getting the d and s
	for (int i = 0; i <= 3; i++)
	{
		if ((command >> (i + 4)) & 1)
		{
			d = d | (1 << i);
		}
		if ((command >> i) & 1)
		{
			s = s | (1 << i);
		}
	}
	if ((*(box + opC)).IS_VALID == false)
	{
		cout << "[HARDWARE FAULT] Invalid Instruction" << endl;
		return;
	}
	//performing the same action if the immediate value is 16 bit
	if ((*(box + opC)).IS_16BIT_IMMEDIATE == true)
	{
		unsigned short payloadPC = bank.getterForPC();
		unsigned short pHigh = readFromCache(payloadPC);
		unsigned short pLow = readFromCache(payloadPC + 1);
		//the instruction is stored in this variable for this after the combining has taken place
		nextsixteen = (pHigh << 8) | pLow;
		bank.setterForPC(bank.getterForPC() + 2);

		if (choice == 'y' || choice == 'Y')
		{
			cout << "\033[94m  -> Decode: Format D (2-Word). Fetched Immediate: 0x";
			//for the padding. ensuring that the total number of digits on display remain 4 
			int numOfDig = 0;
			//storing initially in binary form
			unsigned short temp = nextsixteen;
			if (temp == 0)
			{
				numOfDig = 1;
			}
			//the loop that counts how many digits are there
			while (temp > 0)
			{
				temp = temp >> 4;
				numOfDig++;
			}
			int diff = 4 - numOfDig;
			int i = 0;
			while (i < diff)
			{
				cout << "0";
				i++;
			}
			cout << hex << nextsixteen << dec << "\033[0m\n";
		}
		else if (choice == 'n' || choice == 'N')
		{
			cout << "\033[94mDecode: Format D (2-Word). Fetched Immediate: 0x" << hex << nextsixteen << dec << "\033[0m\n";
		}
	}
	unsigned short resultFinal = 0;
	double heatEveryFrame = 0.05;
	//now translating the opcodes from the box to carry out operations
	if ((*(box + opC)).IS_BRANCH == true)
	{	//having bools that check for the jump type
		bool jumpZero = false;
		bool jumpOne = false;
		bool jump = false;
		//storing the last bit
		int lastbit = bank.getterForFlags() & 0x01;
		if (lastbit == 1)
		{
			jumpZero = true;
		}
		else if (lastbit == 0)
		{
			jumpOne = true;
		}
		if ((*(box + opC)).BRANCH_ON_ZERO)
		{
			if (jumpZero == true)
			{
				jump = true;
			}
		}
		else if ((*(box + opC)).BRANCH_ON_NOT_ZERO)
		{  // instruction == jnz
			if (jumpOne == true)
			{
				jump = true;
			}
		}
		else
		{   //jump regardless of the bit
			jump = true;
		}
		if (jump == true)
		{
			bank.setterForPC(bank.readRegIdx(s));
		}
	}
	//if there needs to be an operation which is to be performed
	//if yes, then grab the operands and the operation and obtain the result
	if ((*(box + opC)).USES_AEC)
	{
		resultFinal = alu.execute((*(box + opC)).AEC_OPERATION, bank.readRegIdx(d), bank.readRegIdx(s), bank);
		//heatEveryFrame += 0.15;//my custom logic for heat
	}
	if ((*(box + opC)).IS_4BIT_IMMEDIATE)
	{
		resultFinal = s;
	}
	if ((*(box + opC)).IS_16BIT_IMMEDIATE)
	{
		resultFinal = nextsixteen;
	}
	if ((*(box + opC)).IS_MEMORY_READ)
	{
		unsigned short readAdd = bank.readRegIdx(s);
		(*mainboard).adrStream = readAdd;
		(*mainboard).isReading = true;
		(*mainboard).pulseClock();
		resultFinal = (*mainboard).dataStream;
		//pints the value and the address that the data was read from
		if (choice == 'y' || choice == 'Y')
		{
			cout << "\033[94m  -> Exec: Read 0x" << hex << (int)resultFinal << " from Mem Address 0x" << readAdd << dec << "\033[0m\n";
		}
		else if (choice == 'n' || choice == 'N')
		{
			cout << "\033[94mExec: Read 0x" << hex << (int)resultFinal << " from Mem Address 0x" << readAdd << dec << "\033[0m\n";
		}
	}
	if ((*(box + opC)).IS_MEMORY_WRITE)
	{
		unsigned short writeAdd = bank.readRegIdx(s);
		(*mainboard).adrStream = writeAdd;
		(*mainboard).dataStream = bank.readRegIdx(d);
		(*mainboard).isWriting = true;
		if (isValid && writeAdd >= BA && writeAdd < BA + 16)
		{
			isValid = false;
		}
		(*mainboard).pulseClock();
		//displays what was written into the memory + the address it was sent to 
		if (choice == 'y' || choice == 'Y')
		{
			cout << "\033[94m  -> Exec: Store 0x" << hex << bank.readRegIdx(d) << " to Mem Address 0x" << writeAdd << dec << "\033[0m\n";
		}
		else if (choice == 'n' || choice == 'N')
		{
			cout << "\033[94mExec: Store 0x" << hex << bank.readRegIdx(d) << " to Mem Address 0x" << writeAdd << dec << "\033[0m\n";
		}
	}
	if ((*(box + opC)).WRITES_TO_REGISTER)
	{
		bank.writeRegIdx(d, resultFinal);
		//this displayes the final result after operation into the register and which register asw
		if (choice == 'y' || choice == 'Y')
		{
			cout << "\033[94m  -> Writeback: R" << (int)d << " = " << dec << resultFinal << " (0x" << hex << resultFinal << dec << ")\033[0m\n";
		}
		else if (choice == 'n' || choice == 'N')
		{
			cout << "\033[94mWriteback: R" << (int)d << " = " << dec << resultFinal << " (0x" << hex << resultFinal << dec << ")\033[0m\n";
		}
	}
	cTemp += heatEveryFrame;
	if (cTemp > 90.0)
	{
		bank.thermalShutNow();
	}
}
void copyString(char* d, const char* s)
{
	if (d == nullptr || s == nullptr)
	{
		return;
	}
	while (*s != '\0')
	{
		*d = *s;
		d++;
		s++;
	}
	*d = '\0'; //ending array with a null terminator
}
int main()
{
	//creating the instances here
	Mainboard mboard;
	Processor centralp;
	MemoryModule randAM;
	PhosphorusDisp graphicPU;
	Kboard keyboard(256);
	PwrSupUnit powerSupply(500.0, &mboard);
	//bools that help check if the slot is already occupied or not
	bool isCpuSlotAvailable = mboard.plugInTheCPU(&centralp);
	bool isRamSlotAvailable = mboard.plugInTheRAM(&randAM);
	bool isGpuSlotAvailable = mboard.plugInTheGPU(&graphicPU);
	bool isKbSlotAvailable = mboard.plugInTheKeyboard(&keyboard);
	bool isPsuSlotAvailable = mboard.plugInThePSU(&powerSupply);
	int switchTrigger = 0;
	if (!isCpuSlotAvailable)
	{
		cout << "CPU slot occupied!" << endl;;
	}
	else if (!isRamSlotAvailable)
	{
		cout << "RAM slot occupied!" << endl;
	}
	else if (!isGpuSlotAvailable)
	{
		cout << "GPU slot occupied!" << endl;
	}
	else if (!isKbSlotAvailable)
	{
		cout << "I/O slot occupied!" << endl;
	}
	else if (!isPsuSlotAvailable)
	{
		cout << "ATX slot occupied!" << endl;;
	}
	//establishing the connection of these components with the main board
	centralp.connectToMainboard(&mboard);
	//the display content
	cout << "\n\n\n\033[97m      L a z a r u s   M a c h i n e\033[0m" << endl;
	cout << "\033[96m              O N Y X - 1 6\033[0m\n";
	cout << "\033[90m     16-BIT Virtual Turing Architecture\033[0m\n\n";
	cout << "     --- BOOTING SILICON PROTOCOL ---\n";
	cout << "[BIOS] Initializing Custom OS Interpreter...\n";
	cout << "      ====================================\n";
	cout << "\033[97m          O N Y X   B O O T   M E N U\033[0m" << endl;
	cout << "      ====================================\n\n";
	cout << "\033[96m[1]\033[0m Turing Complete Human Urdu Calculator\n";
	cout << "\033[96m[2]\033[0m 'HELLO WORLD' Urdu Printer\n";
	cout << "\033[96m[3]\033[0m Hardware Authentication Firewall\n\n";
	cout << "\033[93mSelection (1-3): \033[0m";
	char selectingArr[5];
	cin >> std::setw(5) >> selectingArr;
	cout << "Enable Cycle-by-Cycle Verbose Debug Logging? (Y/N): ";
	char v[3];
	cin >> std::setw(3) >> v;
	cin.ignore(1000, '\n');
	centralp.choice = v[0];
	char filename[100] = "hello.txt";
	if (*(selectingArr) == '1')
	{
		copyString(filename, "calculator.txt");
	}
	else if (*(selectingArr) == '2')
	{
		copyString(filename, "hello.txt");
	}
	else if (*(selectingArr) == '3')
	{
		copyString(filename, "auth.txt");
	}

	Interpreter loader;
	loader.loadProgramAndFlash(filename, randAM);

	while (!centralp.isHalted())
	{
		centralp.oneIter();
	}

	centralp.displayDmp();
	graphicPU.display();
	return 0;
}