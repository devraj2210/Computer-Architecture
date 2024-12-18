#include <bits/stdc++.h>
using namespace std;

#define stoi stoll    // to handle strings containing large numbers

// structure to hold encoding data for instructions
typedef struct fields
{
    int format;
    string opcode;
    string funct3;
    string funct7;
} fields;
// maps to implement efficient access of encoding data
map<string, uint64_t> register_values;
unordered_map<string, string> registers;
unordered_map<string, string> aliases;
unordered_map<string, fields> encoding;
vector<pair<string, int>> callStack;
vector<vector<string>> lines(50);              // stores the input file with each index as a line (vector of strings)
stack<string> last_label;                      // top stores the name of the function at the top of the call stack
set<int> breakpoints;
uint8_t memory[0x50001];
int text_addr = 0;                            // starting address of text section
int data_addr = 0x10000;                      // starting address of data section
int text_start=0;                             // starting line number of the text part of the code
int pc=0;                                     // program counter
int num_lines = 0;                            // number of lines in the input file

// to convert a decimal number into a binary string with number of bits specified
string decTobin(long long int n, int bits)
{
    string binary = "";
    int digits = bits;
    bool neg = 0;
    if (n < 0)
    {
        neg = 1;
        n *= -1;
    }
    while (n > 0)
    {
        binary = to_string(n % 2) + binary;
        digits--;
        n = n / 2;
    }
    while (digits > 0) // to extend MSB
    {
        binary = '0' + binary;
        digits--;
    }
    if (neg) // 2's complement
    {
        for (int i = 0; i < bits; i++)
        {
            if (binary[i] == '1')
                binary[i] = '0';
            else
                binary[i] = '1';
        }
        for (int i = bits - 1; i >= 0; i--)
        {
            if (binary[i] == '1')
                binary[i] = '0';
            else
            {
                binary[i] = '1';
                break;
            }
        }
    }
    return binary;
}

unsigned long long int hexTodec(string hex)
{
    unsigned long long int num = 0;
    int fac = 1;
    for (int i = hex.length() - 1; i >= 0; i--)
    {
        int a;
        if (hex[i] - 'a' >= 0)
            a = hex[i] - 'a' + 10;
        else
            a = hex[i] - '0';
        num = num + a * fac;
        fac *= 16;
    }
    return num;
}

// to convert a 32 bit binary string into hexadecimal
string binTohex(string s, int digits)
{
    string ans = "";
    for (int i = digits - 1; i >= 0; i--) // converting 4 binary digits into 1 hexadecimal digit
    {
        int num = stoi(to_string(s[4 * i + 3] - 48)) + stoi(to_string(s[4 * i + 2] - 48)) * 2 + stoi(to_string(s[4 * i + 1] - 48)) * 4 + stoi(to_string(s[4 * i] - 48)) * 8;
        if (num < 0 || num > 15)
            return "error";
        if (num < 10)
            ans = to_string(num) + ans;
        else
            ans = char((int)'a' + (num - 10)) + ans;
    }
    return ans;
}

// to convert a hexadecimal immediate into binary
string hexTobin(string s, int bits)
{
    long long int num = hexTodec(s);
    return decTobin(num, bits); // then to binary
}

string decTohex(long long int num, int digits)
{
    string binary = decTobin(num, digits * 4);
    return binTohex(binary, digits);
}

// convert a hex or dec number as a string into a specified number of bytes in the form of a vector
vector<uint8_t> numTobytes(string s, int bytes, bool hex)
{
    vector<uint8_t> ans;
    unsigned long long int num;
    if (hex)
    {
        if (s.length() != 2 * bytes)
        {
            num = hexTodec(s);
            s = binTohex(decTobin(num, bytes * 2 * 4), bytes * 2);
        }
    }
    else
    {
        num = stoi(s);
        s = binTohex(decTobin(num, bytes * 2 * 4), bytes * 2);
    }
    for (int i = s.length() - 1; bytes > 0; bytes--)
    {
        uint8_t a, b;
        if (s[i - 1] - 'a' >= 0)
        {
            a = 10 + s[i - 1] - 'a';
        }
        else
        {
            a = s[i - 1] - '0';
        }
        if (s[i] - 'a' >= 0)
        {
            b = 10 + s[i] - 'a';
        }
        else
        {
            b = s[i] - '0';
        }
        i -= 2;
        ans.push_back(a * 16 + b);
    }
    return ans;
}

// initialize register values to 0, sp is initialized to stack-top value: 0x50000
void initRegisterValues(void)
{
    for (int i = 0; i < 32; i++)
    {
        register_values["x" + to_string(i)] = 0;
    }
    // register_values[aliases["sp"]] = 0x50000;
}

// to set the 5bit register values for the register names and aliases
void setRegisters(void)
{
    for (int i = 0; i < 32; i++)
    {
        registers["x" + to_string(i)] = decTobin(i, 5);
    }
    for (int i = 0; i < 32; i++)
    {
        aliases["x" + to_string(i)] = "x" + to_string(i);
        if (i >= 5 && i <= 7)
            aliases["t" + to_string(i - 5)] = "x" + to_string(i);
        if (i >= 10 && i <= 17)
            aliases["a" + to_string(i - 10)] = "x" + to_string(i);
        if (i >= 18 && i <= 27)
            aliases["s" + to_string(i - 16)] = "x" + to_string(i);
        if (i >= 28 && i <= 31)
            aliases["t" + to_string(i - 25)] = "x" + to_string(i);
    }
    aliases["zero"] = "x0";
    aliases["ra"] = "x1";
    aliases["sp"] = "x2";
    aliases["gp"] = "x3";
    aliases["tp"] = "x4";
    aliases["fp"] = "x8";
    aliases["s0"] = "x8";
    aliases["s1"] = "x9";
}

// setting encoding binary values
void setEncoding(void)
{
    encoding["add"] = {1, "0110011", "000", "0000000"}; // R
    encoding["sub"] = {1, "0110011", "000", "0100000"};
    encoding["and"] = {1, "0110011", "111", "0000000"};
    encoding["or"] = {1, "0110011", "110", "0000000"};
    encoding["xor"] = {1, "0110011", "100", "0000000"};
    encoding["sll"] = {1, "0110011", "001", "0000000"};
    encoding["srl"] = {1, "0110011", "101", "0000000"};
    encoding["sra"] = {1, "0110011", "101", "0100000"};

    encoding["addi"] = {2, "0010011", "000", "-1"}; //  I
    encoding["andi"] = {2, "0010011", "111", "-1"};
    encoding["ori"] = {2, "0010011", "110", "-1"};
    encoding["xori"] = {2, "0010011", "100", "-1"};
    encoding["slli"] = {2, "0010011", "001", "000000"};
    encoding["srli"] = {2, "0010011", "101", "000000"};
    encoding["srai"] = {2, "0010011", "101", "010000"};

    encoding["ld"] = {3, "0000011", "101", "-1"}; // I
    encoding["lw"] = {3, "0000011", "010", "-1"};
    encoding["lh"] = {3, "0000011", "001", "-1"};
    encoding["lb"] = {3, "0000011", "000", "-1"};
    encoding["lwu"] = {3, "0000011", "110", "-1"};
    encoding["lhu"] = {3, "0000011", "101", "-1"};
    encoding["lbu"] = {3, "0000011", "100", "-1"};

    encoding["sd"] = {4, "0100011", "101", "-1"}; // S
    encoding["sw"] = {4, "0100011", "100", "-1"};
    encoding["sh"] = {4, "0100011", "001", "-1"};
    encoding["sb"] = {4, "0100011", "000", "-1"};

    encoding["beq"] = {5, "1100011", "000", "-1"}; // B
    encoding["bne"] = {5, "1100011", "001", "-1"};
    encoding["blt"] = {5, "1100011", "100", "-1"};
    encoding["bge"] = {5, "1100011", "101", "-1"};
    encoding["bltu"] = {5, "1100011", "110", "-1"};
    encoding["bgeu"] = {5, "1100011", "111", "-1"};

    encoding["jal"] = {7, "1101111", "-1", "-1"};   // J
    encoding["jalr"] = {3, "1100111", "000", "-1"}; // I

    encoding["lui"] = {6, "0110111", "000", "-1"}; // U
}

// to find the line number of a label, also to check if there are zero or multuple labels with the same name
int find_label(string label)
{
    int count = 0;
    int ind = -1;
    for (int i = 0; i < num_lines; i++)
    {
        if (lines[i][0] == label + ":")
        {
            count++;
            ind = i;
            if (count == 2) // multiple labels
                return -2;
        }
    }
    if (count == 0) // label not found
        return -1;
    return ind;
}

// functions to perform encoding for each format
string encodeR(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;
    string funct7 = codes.funct7;

    int words = operation.size();
    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return "error";
    }
    string rd = operation[1];
    if (aliases.find(rd) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return "error";
    }
    string rs1 = operation[2];
    if (aliases.find(rs1) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return "error";
    }
    string rs2 = operation[3];
    if (aliases.find(rs2) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs2 not valid, must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return "error";
    }
    rd = registers[aliases[rd]];
    rs1 = registers[aliases[rs1]];
    rs2 = registers[aliases[rs2]];

    string ans = binTohex(funct7 + rs2 + rs1 + funct3 + rd + opcode, 8);
    if (ans == "error")
    {
        cout << "Error on line " << line_no + 1 << endl;
        return "error";
    }
    return ans;
}

string encodeI1(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;
    string funct7 = codes.funct7;

    int words = operation.size();
    string imm, rs1, rd;
    rd = operation[1];

    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, rs1, imm" << endl;
        return "error";
    }
    int num = stoi(operation[3]);
    if (funct3 == "001" || funct3 == "101")
    {
        if (num < 0 || num > 32)
        {
            cout << "Error on line " << line_no + 1 << ": immediate value out of range : must be between 0 and 32!";
            return "error";
        }
        imm = funct7 + decTobin(num, 6);
    }
    else
    {
        if (num < -2048 || num >= 2048)
        {
            cout << "Error on line " << line_no + 1 << ": immediate value cannot fit in 12 bits!";
            return "error";
        }
        imm = decTobin(num, 12);
    }
    rs1 = operation[2];
    if (aliases.find(rd) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, rs1, imm" << endl;
        return "error";
    }
    if (aliases.find(rs1) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rd, rs1, imm" << endl;
        return "error";
    }
    rd = registers[aliases[rd]];
    rs1 = registers[aliases[rs1]];

    string ans = binTohex(imm + rs1 + funct3 + rd + opcode, 8);
    if (ans == "error")
    {
        cout << "Error on line " << line_no + 1 << endl;
        return "error";
    }
    return ans;
}

string encodeI2(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;
    string funct7 = codes.funct7;

    int words = operation.size();
    string imm, rs1, rd;
    rd = operation[1];
    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, imm(rs1)" << endl;
        return "error";
    }
    int num = stoi(operation[2]);
    if (num < -2048 || num >= 2048)
    {
        cout << "Error on line " << line_no + 1 << ": immediate value cannot fit in 12 bits!";
        return "error";
    }
    imm = decTobin(num, 12);
    rs1 = operation[3];
    if (aliases.find(rd) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, imm(rs1)" << endl;
        return "error";
    }
    if (aliases.find(rs1) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rd, imm(rs1)" << endl;
        return "error";
    }
    rd = registers[aliases[rd]];
    rs1 = registers[aliases[rs1]];

    string ans = binTohex(imm + rs1 + funct3 + rd + opcode, 8);
    if (ans == "error")
    {
        cout << "Error on line " << line_no + 1 << endl;
        return "error";
    }
    return ans;
}

string encodeS(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rs2, imm(rs1)" << endl;
        return "error";
    }
    int num = stoi(operation[2]);
    if (num < -2048 || num >= 2048)
    {
        cout << "Error on line " << line_no + 1 << ": immediate value cannot fit in 12 bits!";
        return "error";
    }
    string imm = decTobin(num, 12);
    string imm7 = imm.substr(0, 7);
    string imm5 = imm.substr(7, 5);
    string rs1 = operation[3];
    if (aliases.find(rs1) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rs2, imm(rs1)" << endl;
        return "error";
    }
    string rs2 = operation[1];
    if (aliases.find(rs2) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs2 not valid, must have syntax --> " << operation[0] << " rs2, imm(rs1)" << endl;
        return "error";
    }
    rs1 = registers[aliases[rs1]];
    rs2 = registers[aliases[rs2]];

    string ans = binTohex(imm7 + rs2 + rs1 + funct3 + imm5 + opcode, 8);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return "error";
    }
    return ans;
}

string encodeB(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rs1, rs2, label" << endl;
        return "error";
    }
    int found = find_label(operation[operation.size() - 1]);
    if (found == -1)
    {
        cout << "Error: Cannot find label '" << operation[operation.size() - 1] << "' on line " << line_no + 1 << endl;
        return "error";
    }
    if (found == -2)
    {
        cout << "Error: Multiple labels named '" << operation[operation.size() - 1] << "' on line " << line_no + 1 << endl;
        return "error";
    }
    string imm = decTobin((found - line_no) * 4, 13);
    string imm_5to10 = imm.substr(2, 6);
    string imm_1to4 = imm.substr(8, 4);
    string rs1 = operation[1];
    if (aliases.find(rs1) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rs1, rs2, label" << endl;
        return "error";
    }
    string rs2 = operation[2];
    if (aliases.find(rs2) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs2 not valid, must have syntax --> " << operation[0] << " rs1, rs2, label" << endl;
        return "error";
    }
    rs1 = registers[aliases[rs1]];
    rs2 = registers[aliases[rs2]];

    string ans = binTohex(imm[0] + imm_5to10 + rs2 + rs1 + funct3 + imm_1to4 + imm[1] + opcode, 8);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return "error";
    }
    return ans;
}

string encodeU(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 3)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, imm" << endl;
        return "error";
    }
    string imm;
    if (operation[2][1] == 'x')
    {
        operation[2].erase(0, 2);
        imm = hexTobin(operation[2], 20);
    }
    else
    {
        int num = stoi(operation[2]);
        if (num < 0 || num >= 1048576)
        {
            cout << "Error on line " << line_no + 1 << ": immediate value out of range!";
            return "error";
        }
        imm = decTobin(num, 20);
    }
    string rd = operation[1];
    if (aliases.find(rd) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, imm" << endl;
        return "error";
    }
    rd = registers[aliases[rd]];

    string ans = binTohex(imm + rd + opcode, 8);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return "error";
    }
    return ans;
}

string encodeJ(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 3)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, label" << endl;
        return "error";
    }
    int found = find_label(operation[2]);
    if (found == -1)
    {
        cout << "Error: Cannot find label '" << operation[2] << "' on line " << line_no + 1 << endl;
        return "error";
    }
    if (found == -2)
    {
        cout << "Error: Multiple labels named '" << operation[2] << "' on line " << line_no + 1 << endl;
        return "error";
    }
    string imm = decTobin((found - line_no) * 4, 21);
    string imm_12to19 = imm.substr(1, 8);
    string imm_1to10 = imm.substr(10, 10);
    string rd = operation[1];
    if (aliases.find(rd) == aliases.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, label" << endl;
        return "error";
    }
    rd = registers[aliases[rd]];

    string ans = binTohex(imm[0] + imm_1to10 + imm[9] + imm_12to19 + rd + opcode, 8);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return "error";
    }
    return ans;
}

// function to check the encoding format and call the appropriate encoding function
bool encode(void)
{
    text_start=0;
    text_addr=0;
    data_addr=0x10000;
    if (lines[0][0] == ".data")
    {
        text_start++;
        int l=1;
        while (1)
        {
            if (lines[l][0] == ".text")
            {
                text_start++;
                break;
            }
            int num_bytes;
            if (lines[l][0] == ".dword")
                num_bytes = 8;
            else if (lines[l][0] == ".word")
                num_bytes = 4;
            else if (lines[l][0] == ".half")
                num_bytes = 2;
            else if (lines[l][0] == ".byte")
                num_bytes = 1;
            else
            {
                cout << "Invalid Data Type: " << lines[l][0] << " !" << endl;
                return 0;
            }
            for (int i = 1; i < lines[l].size(); i++)
            {
                bool hex = false;
                if (lines[l][i][0] == '0' && lines[l][i][1] == 'x')
                {
                    hex = true;
                    lines[l][i].erase(0, 2);
                }
                vector<uint8_t> bytes = numTobytes(lines[l][i], num_bytes, hex);
                for (int j = 0; j < num_bytes; j++)
                {
                    memory[data_addr++] = bytes[j];
                }
            }
            text_start++;
            l++;
        }
    }
    else if (lines[0][0] == ".text")
        text_start++;
    for (int i = text_start; i < num_lines; i++)
    {
        vector<string> operation = lines[i];
        if (encoding.find(operation[0]) == encoding.end()) // if first word is a label
            operation.erase(operation.begin());
        if (encoding.find(operation[0]) == encoding.end()) // second word cannot be a label
        {
            cout << "Error: cannot find instruction '" << operation[0] << "' on line " << i + 1 << endl;
            return 0;
        }
        fields var = encoding[operation[0]];
        int fmt = var.format;

        string ans;
        switch (fmt) // call function according to format, if in any iteration error is returned, exit
        {
        case 1:
            if ((ans=encodeR(operation, var, i)) == "error")
                return 0;
            break;

        case 2:
            if ((ans=encodeI1(operation, var, i)) == "error")
                return 0;
            break;

        case 3:
            if ((ans=encodeI2(operation, var, i)) == "error")
                return 0;
            break;

        case 4:
            if ((ans=encodeS(operation, var, i)) == "error")
                return 0;
            break;

        case 5:
            if ((ans=encodeB(operation, var, i)) == "error")
                return 0;
            break;

        case 6:
            if ((ans=encodeU(operation, var, i)) == "error")
                return 0;
            break;

        case 7:
            if ((ans=encodeJ(operation, var, i)) == "error")
                return 0;
            break;

        default:
            cout << "Error on line " << i + 1 << " : Invalid Format" << endl;
            return 0;
            break;
        }
        vector<uint8_t> bytes = numTobytes(ans, 4, true);
        for (auto j : bytes)
        {
            memory[text_addr++] = j;
        }
    }
    return 1;
}

// functions to execute the instructions using the registers and the memory
void executeR(vector<string> operation)
{
    if (operation[0] == "add")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] + register_values[aliases[operation[3]]];
    }
    else if (operation[0] == "sub")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] - register_values[aliases[operation[3]]];
    }
    else if (operation[0] == "xor")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] ^ register_values[aliases[operation[3]]];
    }
    else if (operation[0] == "or")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] | register_values[aliases[operation[3]]];
    }
    else if (operation[0] == "and")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] & register_values[aliases[operation[3]]];
    }
    else if (operation[0] == "sll")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] << register_values[aliases[operation[3]]];
    }
    else if (operation[0] == "srl")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] >> register_values[aliases[operation[3]]];
    }
    else if (operation[0] == "sra")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] * int(pow(2, register_values[aliases[operation[3]]]));
    }
    cout << "executed " + operation[0] + " " + operation[1] + ", " + operation[2] + ", " + operation[3] << "; pc=0x" << decTohex(pc * 4, 8) << endl;
    pc++;
}

void executeI1(vector<string> operation)
{
    if (operation[0] == "addi")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] + stoi(operation[3]);
    }
    else if (operation[0] == "xori")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] ^ stoi(operation[3]);
    }
    else if (operation[0] == "ori")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] | stoi(operation[3]);
    }
    else if (operation[0] == "andi")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] & stoi(operation[3]);
    }
    else if (operation[0] == "slli")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] << stoi(operation[3]);
    }
    else if (operation[0] == "srli")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] >> stoi(operation[3]);
    }
    else if (operation[0] == "srai")
    {
        register_values[aliases[operation[1]]] = register_values[aliases[operation[2]]] * int(pow(2, stoi(operation[3])));
    }
    cout << "executed " + operation[0] + " " + operation[1] + ", " + operation[2] + ", " + operation[3] << "; pc=0x" << decTohex(pc * 4, 8) << endl;
    pc++;
}

void executeI2(vector<string> operation)
{
    long long int start_addr = stoi(operation[2]) + register_values[aliases[operation[3]]];
    cout << "executed " + operation[0] + " " + operation[1] + ", " + operation[2] + "(" + operation[3] << "); pc=0x" << decTohex(pc * 4, 8) << endl;
    if (operation[0] == "ld")
    {
        uint64_t num = 0;
        uint64_t fac = 1;
        for (int i = 0; i < 8; i++)
        {
            num = num + memory[start_addr + i] * fac;
            fac *= 256;
        }
        register_values[aliases[operation[1]]] = num;
    }
    else if (operation[0] == "lw")
    {
        int num = 0;
        int fac = 1;
        for (int i = 0; i < 4; i++)
        {
            num = num + memory[start_addr + i] * fac;
            fac *= 256;
        }
        register_values[aliases[operation[1]]] = num;
    }
    else if (operation[0] == "lh")
    {
        register_values[aliases[operation[1]]] = int16_t(memory[start_addr + 1] * 256 + memory[start_addr]);
    }
    else if (operation[0] == "lb")
    {
        register_values[aliases[operation[1]]] = int8_t(memory[start_addr]);
    }
    else if (operation[0] == "lwu")
    {
        unsigned int num;
        int fac = 1;
        for (int i = 0; i < 4; i++)
        {
            num = num + memory[start_addr + i] * fac;
            fac *= 256;
        }
        register_values[aliases[operation[1]]] = num;
    }
    else if (operation[0] == "lhu")
    {
        uint16_t num = memory[start_addr + 1] * 256 + memory[start_addr];
        register_values[aliases[operation[1]]] = num;
    }
    else if (operation[0] == "lbu")
    {
        uint8_t num = memory[start_addr];
        register_values[aliases[operation[1]]] = num;
    }
    else if (operation[0] == "jalr")
    {
        callStack.pop_back();
        last_label.pop();
        if (aliases[operation[1]] != "x0")
            register_values[aliases[operation[1]]] = (pc + 1) * 4;
        pc=register_values[aliases[operation[3]]] / 4;
        pc--;
    }
    pc++;
}

void executeS(vector<string> operation)
{
    long long int start_addr = stoi(operation[2]) + register_values[aliases[operation[3]]];
    vector<uint8_t> bytes = numTobytes(decTohex(register_values[aliases[operation[1]]], 16), 8, true);
    int num_bytes;
    if (operation[0] == "sd")
        num_bytes = 8;
    else if (operation[0] == "sw")
        num_bytes = 4;
    else if (operation[0] == "sh")
        num_bytes = 2;
    else if (operation[0] == "sb")
        num_bytes = 1;
    for (int i = 0; i < num_bytes; i++)
    {
        memory[start_addr++] = bytes[i];
    }
    cout << "executed " + operation[0] + " " + operation[1] + ", " + operation[2] + "(" + operation[3] << "); pc=0x" << decTohex(pc * 4, 8) << endl;
    pc++;
}

void executeB(vector<string> operation)
{
    cout << "executed " + operation[0] + " " + operation[1] + ", " + operation[2] + ", " + operation[3] << "; pc=0x" << decTohex(pc * 4, 8) << endl;
    int line_no = find_label(operation[3]);
    if (operation[0] == "beq")
    {
        if (register_values[aliases[operation[1]]] == register_values[aliases[operation[2]]])
        {
            pc=line_no-text_start;
        }
        else
            pc++;
    }
    else if (operation[0] == "bne")
    {
        if (register_values[aliases[operation[1]]] != register_values[aliases[operation[2]]])
        {
            pc=line_no-text_start;
        }
        else
            pc++;
    }
    else if (operation[0] == "blt")
    {
        if (register_values[aliases[operation[1]]] < register_values[aliases[operation[2]]])
        {
            pc=line_no-text_start;
        }
        else
            pc++;
    }
    else if (operation[0] == "bge")
    {
        if (register_values[aliases[operation[1]]] >= register_values[aliases[operation[2]]])
        {
            pc=line_no-text_start;
        }
        else
            pc++;
    }
    else if (operation[0] == "bltu")
    {
        if ((uint64_t)register_values[aliases[operation[1]]] < (uint64_t)register_values[aliases[operation[2]]])
        {
            pc=line_no-text_start;
        }
        else
            pc++;
    }
    else if (operation[0] == "bgeu")
    {
        if ((uint64_t)register_values[aliases[operation[1]]] >= (uint64_t)register_values[aliases[operation[2]]])
        {
            pc=line_no-text_start;
        }
        else
            pc++;
    }
}

void executeJ(vector<string> operation)
{
    int line_no = find_label(operation[2]);
    callStack.push_back({operation[2], 0});
    last_label.push(operation[2]);
    cout << "executed " + operation[0] + " " + operation[1] + ", " + operation[2] << "; pc=0x" << decTohex(pc * 4, 8) << endl;
    register_values[aliases[operation[1]]] = (pc + 1) * 4;
    pc=line_no-text_start;
}

void executeU(vector<string> operation)
{
    string word = operation[2];
    if(operation[2][0]=='0' && operation[2][1]=='x')
        register_values[aliases[operation[1]]] = hexTodec(operation[2].erase(0, 2)) * 4096;
    else
        register_values[aliases[operation[1]]] = stoi(operation[2]) * 4096;
    cout << "executed " + operation[0] + " " + operation[1] + ", " + word << "; pc=0x" << decTohex(pc * 4, 8) << endl;
    pc++;
}

// function to check the encoding format and call the appropriate executing format function
void execute(void)
{
    int i = pc + text_start;

    vector<string> operation = lines[i];
    if (encoding.find(operation[0]) == encoding.end()) // if first word is a label
    {
        operation.erase(operation.begin());
    }
    fields var = encoding[operation[0]];
    int fmt = var.format;
    callStack[callStack.size() - 1] = {last_label.top(), i + 1};

    switch (fmt) // call function according to format, if in any iteration error is returned, exit
    {
    case 1:
    {
        executeR(operation);
        break;
    }

    case 2:
    {
        executeI1(operation);
        break;
    }

    case 3:
    {
        executeI2(operation);
        break;
    }

    case 4:
    {
        executeS(operation);
        break;
    }

    case 5:
    {
        executeB(operation);
        break;
    }

    case 6:
    {
        executeU(operation);
        break;
    }

    case 7:
    {
        executeJ(operation);
        break;
    }

    default:
        cout << "Error on line " << i + 1 << " : Invalid Format" << endl;
        break;
    }
}

int main()
{
    setEncoding();                // set values for encoding
    setRegisters();

    bool loaded=false;           // to keep track of an open file
    ifstream inp;
    while (true)
    {
        string command;
        getline(cin, command);

        stringstream commandstream(command);     // convert into a stream of words (that were separated by spaces)
        vector<string> elements;                 // the input as a vector of strings
        string element;
        while (getline(commandstream, element, ' '))
        {
            elements.push_back(element);
        }

        if (elements[0] == "exit" && elements.size()==1)
        {
            cout << "\nexited the simulator\n"<< endl;
            break;
        }
        else if (elements[0] == "load" && elements.size()==2)
        {
            inp.open(elements[1]);
            string line;
            for(int i=0; i<50; i++)
            {
                lines[i].clear();
            }
            num_lines=0;
            if (inp.is_open())
            {
                while (getline(inp, line)) // reading the file line by line
                {
                    stringstream linestream(line);
                    string word;
                    while (linestream >> word) // splitting by spaces
                    {
                        stringstream wordstream(word);
                        while (wordstream.good())
                        {
                            getline(wordstream, word, ','); // splitting by commas
                            stringstream bracket(word);
                            while (bracket.good())
                            {
                                getline(bracket, word, '('); // splitting by brackets
                                if (word[word.length() - 1] == ')')
                                    word.erase(word.length() - 1, 1);
                                if (word != "")
                                    lines[num_lines].push_back(word);
                            }
                        }
                    }
                    num_lines++;
                }
                inp.close();
            }
            else
            {
                cout<<"\nError in opening file: "<<elements[1]<< " !"<<endl;
                continue;
            }
            if(encode())               // if code is error free
            {
                pc=0;                  // reinitialize pc to zero
                initRegisterValues();
                while(!last_label.empty())
                    last_label.pop();
                callStack.clear();
                last_label.push("main");
                callStack.push_back({"main", 0});
                // cout << "\nSuccessfully loaded file: " << elements[1] << endl;
                loaded=true;   // to know if a file is loaded, for subsequent instructions
            }
            else
            {
                cout<<"\nError in loading file, please make sure the text in the file is valid assembly code!"<<endl;
                continue;
            }
        }
        else if (elements[0] == "run" && elements.size()==1)
        {
            if(!loaded)
            {
                cout<<"\nError: No input file loaded!"<<endl;
                continue;
            }
            while (pc < num_lines-text_start)
            {
                if (breakpoints.find(pc + text_start + 1) != breakpoints.end())
                {
                    cout << "\nexecution stopped at breakpoint\n"<< endl;
                    break;
                }
                execute();
            }
        }
        else if (elements[0] == "regs" && elements.size()==1)
        {
            cout << "\nRegisters:\n";
            string space=" ";
            for (int i = 0; i < 32; i++)
            {
                if(i==10)
                    space="";
                cout << "x" + to_string(i) + space + " = 0x" << hex << register_values["x" + to_string(i)] << dec << endl;
            }
            cout << endl;
        }
        else if (elements[0] == "mem" && elements.size()==3)
        {
            int count = 0;
            for (int i = hexTodec(elements[1].erase(0, 2)); count < stoi(elements[2]); count++)
            {
                cout << "Memory[0x" << hex << i << "]" << " = 0x" << int(memory[i++]) << dec << endl;
            }
        }
        else if (elements[0] == "step" && elements.size()==1)
        {
            if(!loaded)
            {
                cout<<"\nError: No input file loaded!\n"<<endl;
                continue;
            }
            if (pc + text_start < num_lines)
                execute();
            else
                cout << "\nnothing to step\n" << endl;
        }
        else if (elements[0] == "show-stack" && elements.size()==1)
        {
            if (callStack.empty())
            {
                cout << "\nempty call stack: execution complete\n" << endl;
            }
            else
            {
                cout<<"\ncall stack:"<<endl;
                for (auto i : callStack)
                {
                    // if(i.first!="main" || i.second!=0)
                    cout << i.first << ":" << i.second << endl;
                }
            }
        }
        else if (elements[0] == "break" && elements.size()==2)
        {
            if(!loaded)
            {
                cout<<"\nError: No input file loaded!"<<endl;
                continue;
            }
            if(stoi(elements[1]) > num_lines)
            {
                cout<<"\nLine number out of bounds for this file!\n"<<endl;
            }
            breakpoints.insert(stoi(elements[1]));
            cout << "\nBreakpoint set at line " << elements[1] << endl << endl;
        }
        else if (elements[0] == "del" && elements[1] == "break" && elements.size()==3)
        {
            if(!loaded)
            {
                cout<<"\nError: No input file loaded!"<<endl;
                continue;
            }
            if (breakpoints.find(stoi(elements[2])) == breakpoints.end())
            {
                cout << "\nNo breakpoint set for line " << elements[2] << endl <<endl;
            }
            else
            {
                breakpoints.erase(stoi(elements[2]));
            }
        }
        else
        {
            cout << "\nInvalid Command!\n";
        }
        if (pc == num_lines-text_start && !callStack.empty())
        {
            callStack.pop_back();
        }
        cout<<endl;
    }

    return 0;
}