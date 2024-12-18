#include <bits/stdc++.h>
using namespace std;

// structure to hold encoding data for instructions
typedef struct fields
{
    int format;
    string opcode;
    string funct3;
    string funct7;
} fields;

ofstream out("output.hex");

// maps to implement efficient access of encoding data
unordered_map<string, string> registers;
unordered_map<string, fields> encoding;

int num_lines = 0;
// 2-D array to store the instruction lines word by word
vector<vector<string>> lines(50);

// to convert a decimal number into a binary string with number of bits specified
string decTobin(int n, int bits)
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
    while (digits > 0)       // to extend MSB
    {
        binary = '0' + binary;
        digits--;
    }
    if (neg)                 // 2's complement
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

// to set the 5bit register values for the register names and aliases
void setRegisters(void)
{
    for (int i = 0; i < 32; i++)
    {
        registers["x" + to_string(i)] = decTobin(i, 5);
    }
    for (int i = 0; i < 32; i++)
    {
        if (i >= 5 && i <= 7)
            registers["t" + to_string(i - 5)] = registers["x" + to_string(i)];
        if (i >= 10 && i <= 17)
            registers["a" + to_string(i - 10)] = registers["x" + to_string(i)];
        if (i >= 18 && i <= 27)
            registers["s" + to_string(i - 16)] = registers["x" + to_string(i)];
        if (i >= 28 && i <= 31)
            registers["t" + to_string(i - 25)] = registers["x" + to_string(i)];
    }
    registers["zero"] = "00000";
    registers["ra"] = "00001";
    registers["sp"] = "00010";
    registers["gp"] = "00011";
    registers["tp"] = "00100";
    registers["fp"] = "01000";
    registers["s0"] = "01000";
    registers["s1"] = "01001";
}

// to set the various fields associated with an instruction
void setEncoding(void)
{
    encoding["add"] = {1, "0110011", "000", "0000000"};        // R
    encoding["sub"] = {1, "0110011", "000", "0100000"};
    encoding["and"] = {1, "0110011", "111", "0000000"};
    encoding["or"] = {1, "0110011", "110", "0000000"};
    encoding["xor"] = {1, "0110011", "100", "0000000"};
    encoding["sll"] = {1, "0110011", "001", "0000000"};
    encoding["srl"] = {1, "0110011", "101", "0000000"};
    encoding["sra"] = {1, "0110011", "101", "0100000"};

    encoding["addi"] = {2, "0010011", "000", "-1"};         //  I
    encoding["andi"] = {2, "0010011", "111", "-1"};
    encoding["ori"] = {2, "0010011", "110", "-1"};
    encoding["xori"] = {2, "0010011", "100", "-1"};
    encoding["slli"] = {2, "0010011", "001", "000000"};
    encoding["srli"] = {2, "0010011", "101", "000000"};
    encoding["srai"] = {2, "0010011", "101", "010000"};

    encoding["ld"] = {2, "0000011", "101", "-1"};               // I
    encoding["lw"] = {2, "0000011", "010", "-1"};
    encoding["lh"] = {2, "0000011", "001", "-1"};
    encoding["lb"] = {2, "0000011", "000", "-1"};
    encoding["lwu"] = {2, "0000011", "110", "-1"};
    encoding["lhu"] = {2, "0000011", "101", "-1"};
    encoding["lbu"] = {2, "0000011", "100", "-1"};

    encoding["sd"] = {3, "0100011", "101", "-1"};               // S
    encoding["sw"] = {3, "0100011", "100", "-1"};
    encoding["sh"] = {3, "0100011", "001", "-1"};
    encoding["sb"] = {3, "0100011", "000", "-1"};

    encoding["beq"] = {4, "1100011", "000", "-1"};              // B
    encoding["bne"] = {4, "1100011", "001", "-1"};
    encoding["blt"] = {4, "1100011", "100", "-1"};
    encoding["bge"] = {4, "1100011", "101", "-1"};
    encoding["bltu"] = {4, "1100011", "110", "-1"};
    encoding["bgeu"] = {4, "1100011", "111", "-1"};

    encoding["jal"] = {6, "1101111", "-1", "-1"};               // J
    encoding["jalr"] = {2, "1100111", "000", "-1"};             // I

    encoding["lui"] = {5, "0110111", "000", "-1"};              // U
}

// to convert a 32 bit binary string into hexadecimal
string binTohex(string s)
{
    string ans = "";
    for (int i = 7; i >= 0; i--)   // converting 4 binary digits into 1 hexadecimal digit
    {
        int num = stoi(to_string(s[4 * i + 3] - 48)) + stoi(to_string(s[4 * i + 2] - 48)) * 2 + stoi(to_string(s[4 * i + 1] - 48)) * 4 + stoi(to_string(s[4 * i] - 48)) * 8;
        if (num < 0 || num > 15)
            return "error";
        if (num < 10)
            ans = to_string(num) + ans;
        else
            ans = char((int)'A' + (num - 10)) + ans;
    }
    return ans;
}

// to convert a hexadecimal immediate into binary
string hexTobin(string s, int bits)
{
    int num = 0;
    int fac = 1;
    for (int i = s.length() - 1; i >= 0; i--)       // first convert to decimal
    {
        if (s[i] == '1')
            num = fac + num;
        fac *= 16;
    }
    return decTobin(num, bits);             // then to binary
}

// to find the line number of a label, also to check if there are zero or multuple labels with the same name
int find_label(string label)
{
    int count = 0;
    int ind = -1;
    for (int i = 0; i < num_lines; i++)
    {
        if (lines[i][0] == label)
        {
            count++;
            ind = i;
            if (count == 2)             // multiple labels
                return -2;
        }
    }
    if(count==0)                    // label not found
        return -1;
    return ind;
}

// functions to perform encoding for each format
bool encodeR(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;
    string funct7 = codes.funct7;

    int words = operation.size();
    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return 0;
    }
    string rd = operation[1];
    if (registers.find(rd) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return 0;
    }
    string rs1 = operation[2];
    if (registers.find(rs1) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return 0;
    }
    string rs2 = operation[3];
    if (registers.find(rs2) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs2 not valid, must have syntax --> " << operation[0] << " rd, rs1, rs2" << endl;
        return 0;
    }
    rd = registers[rd];
    rs1 = registers[rs1];
    rs2 = registers[rs2];

    string ans = binTohex(funct7 + rs2 + rs1 + funct3 + rd + opcode);
    if (ans == "error")
    {
        cout << "Error on line " << line_no + 1 << endl;
        return 0;
    }
    out << ans << endl;
    return 1;
}

bool encodeI(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;
    string funct7 = codes.funct7;

    int words = operation.size();
    string imm, rs1, rd;
    rd = operation[1];
    if (opcode == "0010011")
    {
        if (words != 4)
        {
            cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, rs1, imm" << endl;
            return 0;
        }
        int num = stoi(operation[3]);
        if (funct3 == "001" || funct3 == "101")
        {
            if (num < 0 || num > 32)
            {
                cout << "Error on line " << line_no + 1 << ": immediate value out of range : must be between 0 and 32!";
                return 0;
            }
            imm = funct7 + decTobin(num, 6);
        }
        else
        {
            if (num < -2048 || num >= 2048)
            {
                cout << "Error on line " << line_no + 1 << ": immediate value cannot fit in 12 bits!";
                return 0;
            }
            imm = decTobin(num, 12);
        }
        rs1 = operation[2];
        if (registers.find(rd) == registers.end())
        {
            cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, rs1, imm" << endl;
            return 0;
        }
        if (registers.find(rs1) == registers.end())
        {
            cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rd, rs1, imm" << endl;
            return 0;
        }
    }
    else
    {
        if (words != 4)
        {
            cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, imm(rs1)" << endl;
            return 0;
        }
        int num = stoi(operation[2]);
        if (num < -2048 || num >= 2048)
        {
            cout << "Error on line " << line_no + 1 << ": immediate value cannot fit in 12 bits!";
            return 0;
        }
        imm = decTobin(num, 12);
        rs1 = operation[3];
        if (registers.find(rd) == registers.end())
        {
            cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, imm(rs1)" << endl;
            return 0;
        }
        if (registers.find(rs1) == registers.end())
        {
            cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rd, imm(rs1)" << endl;
            return 0;
        }
    }
    rd = registers[rd];
    rs1 = registers[rs1];

    string ans = binTohex(imm + rs1 + funct3 + rd + opcode);
    if (ans == "error")
    {
        cout << "Error on line " << line_no + 1 << endl;
        return 0;
    }
    out << ans << endl;
    return 1;
}

bool encodeS(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rs2, imm(rs1)" << endl;
        return 0;
    }
    int num = stoi(operation[2]);
    if (num < -2048 || num >= 2048)
    {
        cout << "Error on line " << line_no + 1 << ": immediate value cannot fit in 12 bits!";
        return 0;
    }
    string imm = decTobin(num, 12);
    string imm7 = imm.substr(0, 7);
    string imm5 = imm.substr(7, 5);
    string rs1 = operation[3];
    if (registers.find(rs1) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rs2, imm(rs1)" << endl;
        return 0;
    }
    string rs2 = operation[1];
    if (registers.find(rs2) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs2 not valid, must have syntax --> " << operation[0] << " rs2, imm(rs1)" << endl;
        return 0;
    }
    rs1 = registers[rs1];
    rs2 = registers[rs2];

    string ans = binTohex(imm7 + rs2 + rs1 + funct3 + imm5 + opcode);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return 0;
    }
    out << ans << endl;
    return 1;
}

bool encodeB(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 4)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rs1, rs2, label" << endl;
        return 0;
    }
    int found = find_label(operation[operation.size() - 1] + ":");
    if (found == -1)
    {
        cout << "Error: Cannot find label '" << operation[operation.size() - 1] << "' on line " << line_no + 1 << endl;
        return 0;
    }
    if (found == -2)
    {
        cout << "Error: Multiple labels named '" << operation[operation.size() - 1] << "' on line " << line_no + 1 << endl;
        return 0;
    }
    string imm = decTobin((found - line_no) * 4, 13);
    string imm_5to10 = imm.substr(2, 6);
    string imm_1to4 = imm.substr(8, 4);
    string rs1 = operation[1];
    if (registers.find(rs1) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs1 not valid, must have syntax --> " << operation[0] << " rs1, rs2, label" << endl;
        return 0;
    }
    string rs2 = operation[2];
    if (registers.find(rs2) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rs2 not valid, must have syntax --> " << operation[0] << " rs1, rs2, label" << endl;
        return 0;
    }
    rs1 = registers[rs1];
    rs2 = registers[rs2];

    string ans = binTohex(imm[0] + imm_5to10 + rs2 + rs1 + funct3 + imm_1to4 + imm[1] + opcode);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return 0;
    }
    out << ans << endl;
    return 1;
}

bool encodeU(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 3)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, imm" << endl;
        return 0;
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
            return 0;
        }
        imm = decTobin(num, 20);
    }
    string rd = operation[1];
    if (registers.find(rd) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, imm" << endl;
        return 0;
    }
    rd = registers[rd];

    string ans = binTohex(imm + rd + opcode);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return 0;
    }
    out << ans << endl;
    return 1;
}

bool encodeJ(vector<string> operation, fields codes, int line_no)
{
    string opcode = codes.opcode;
    string funct3 = codes.funct3;

    int words = operation.size();
    if (words != 3)
    {
        cout << "Error on line " << line_no + 1 << ": must have syntax --> " << operation[0] << " rd, label" << endl;
        return 0;
    }
    int found = find_label(operation[operation.size() - 1] + ":");
    if (found == -1)
    {
        cout << "Error: Cannot find label '" << operation[operation.size() - 1] << "' on line " << line_no + 1 << endl;
        return 0;
    }
    if (found == -2)
    {
        cout << "Error: Multiple labels named '" << operation[operation.size() - 1] << "' on line " << line_no + 1 << endl;
        return 0;
    }
    string imm = decTobin((found - line_no) * 4, 21);
    string imm_12to19 = imm.substr(1, 8);
    string imm_1to10 = imm.substr(10, 10);
    string rd = operation[1];
    if (registers.find(rd) == registers.end())
    {
        cout << "Error on line " << line_no + 1 << ": rd not valid, must have syntax --> " << operation[0] << " rd, label" << endl;
        return 0;
    }
    rd = registers[rd];

    string ans = binTohex(imm[0] + imm_1to10 + imm[9] + imm_12to19 + rd + opcode);
    if (ans == "error")
    {
        cout << "Error: line " << line_no + 1 << endl;
        return 0;
    }
    out << ans << endl;
    return 1;
}

// function to check the encoding format and call the appropriate encoding function
void encode(void)
{
    for (int i = 0; i < num_lines; i++)
    {
        vector<string> operation = lines[i];
        if (encoding.find(operation[0]) == encoding.end())         // if first word is a label
            operation.erase(operation.begin());
        if (encoding.find(operation[0]) == encoding.end())          //second word cannot be a label
        {
            cout << "Error: cannot find instruction '" << operation[0] << "' on line " << i + 1 << endl;
            return;
        }
        fields var = encoding[operation[0]];
        int fmt = var.format;

        switch (fmt)               // call function according to format, if in any iteration error is returned, exit
        {
        case 1:
            if (!encodeR(operation, var, i))
                return;
            break;

        case 2:
            if (!encodeI(operation, var, i))
                return;
            break;

        case 3:
            if (!encodeS(operation, var, i))
                return;
            break;

        case 4:
            if (!encodeB(operation, var, i))
                return;
            break;

        case 5:
            if (!encodeU(operation, var, i))
                return;
            break;

        case 6:
            if (!encodeJ(operation, var, i))
                return;
            break;

        default:
            cout << "Error on line " << i + 1 << " : Invalid Format" << endl;
            return;
            break;
        }
    }
    cout << "Successfully Assembled!" << endl;          // on successful assembly, without errors
}

int main()
{
    ifstream inp("input.s");
    string line;

    setRegisters();
    setEncoding();

    if (inp.is_open())
    {
        while (getline(inp, line))                       // reading the file line by line
        {
            stringstream linestream(line);
            string word;
            while (linestream >> word)                    //splitting by spaces
            {
                stringstream wordstream(word);
                while (wordstream.good())
                {
                    getline(wordstream, word, ',');            //splitting by commas
                    stringstream bracket(word);
                    while (bracket.good())
                    {
                        getline(bracket, word, '(');          //splitting by brackets
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
    encode();        //calling the encode function
    out.close();

    return 0;
}