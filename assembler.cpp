#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
using namespace std;
#define endl '\n'

unordered_map<string, string> OPcode = {{"ADD", "0000"}, {"SUB", "0001"},
                                        {"NAND", "0010"},
                                        {"SLL", "0011"}, {"SRL", "0100"},
                                        {"J", "0101"}, {"BEQZ", "0110"},
                                        {"SEQ", "0111"}, {"SLT", "1000"},
                                        {"LW", "1001"}, {"SW", "1010"},
                                        {"INIT", "1011"}, {"CPY", "1100"},
                                        {"IN", "1101"}, {"OUT", "1110"}};
unordered_map<string, string> REGcode = {{"$ZERO", "000"},
                                         {"$S0", "001"}, {"$S1", "010"}, {"$S2", "011"},
                                         {"$T0", "100"},
                                         {"$T1", "101"}, {"$T2", "110"}, {"$T3", "111"},
                                         {"$0", "000"},
                                         {"$1", "001"}, {"$2", "010"}, {"$3", "011"},
                                         {"$4", "100"},
                                         {"$5", "101"}, {"$6", "110"}, {"$7", "111"}};
unordered_map<string, char> OPtype = {{"ADD", 'R'}, {"SUB", 'R'},
                                      {"NAND", 'R'},
                                      {"SLL", 'I'}, {"SRL", 'I'},
                                      {"J", 'T'}, {"BEQZ", 'T'},
                                      {"SEQ", 'R'}, {"SLT", 'R'},
                                      {"LW", 'R'}, {"SW", 'R'},
                                      {"INIT", 'T'}, {"CPY", 'R'},
                                      {"IN", 'I'}, {"OUT", 'I'}};
unordered_map<string, int> labels;
ifstream fin;
ofstream fout;
bool err = false;

string numToStr(int n, int len){
    string str;
    for (int i = 1<<(len-1); i; i = i>>1){
        if (n&i)
            str += '1';
        else
            str += '0';
    }
    return str;
}

int strToNum(string str){
    int num, i;
    for (i = num = 0; i < str.length(); ++i)
        if (str[i] == '1')
            num |= 1<<(str.length()-i-1);
    return num;
}

bool regCheck(string r, string line, bool comma = false){
    if (r[0] != '$'){
        fout<<"$ missing at the start of "<<r<<":\t"<<line<<endl;
        err = true;
        return true;
    }
    if (comma && r[r.size()-1] != ','){
        fout<<", missing at the end of "<<r<<":\t"<<line<<endl;
        err = true;
        return true;
    }
    return false;
}

void decode(string line, int n){
    stringstream buffer(line);
    string op, bin, r1, r2, label;
    unsigned int i;
    int t;
    if (!(buffer>>op))
        return;
    if (op[op.size()-1] == ':' && !(buffer>>op)){
        fout<<"Missing instruction after label "<<op<<endl;
        err = true;
        return;
    }
    if (OPcode.find(op) == OPcode.end()){
        fout<<"Invalid Opcode:"<<"\t"<<line<<endl;
        err = true;
        return;
    }
    bin = OPcode[op];
    switch(OPtype[op]){
        case 'R':
            if (!(buffer>>r1>>r2)){
                fout<<"Invalid R-Type Format:"<<"\t"<<line<<endl;
                err = true;
                return;
            }
            if (regCheck(r1, line, true) || regCheck(r2, line)) return;
            r1.pop_back();
            if (REGcode.find(r1) == REGcode.end()){
                fout<<"Invalid Register 1:"<<"\t"<<line<<endl;
                err = true;
                return;
            }
            if (REGcode.find(r2) == REGcode.end()){
                fout<<"Invalid Register 2:"<<"\t"<<line<<endl;
                err = true;
                return;
            }
            bin += REGcode[r1] + REGcode[r2];
            break;
        case 'I':
            if (op == "IN" || op == "OUT"){
                if (!(buffer>>r1)){
                    fout<<"Invalid Format for In/Out:"<<"\t"<<line<<endl;
                    err = true;
                    return;
                }
                if (regCheck(r1, line)) return;
                i = (op == "IN")?0:1;
            }
            else{
                if (!(buffer>>r1>>i)){
                    fout<<"Invalid I-Type Format:"<<"\t"<<line<<endl;
                    err = true;
                    return;
                }
                if (regCheck(r1, line, true)) return;
                r1.pop_back();
            }
            if (REGcode.find(r1) == REGcode.end()){
                fout<<"Invalid Register 1:"<<"\t"<<line<<endl;
                err = true;
                return;
            }
            if (i >= 8){
                fout<<"Immediate can only be 0 to 7:"<<"\t"<<line<<endl;
                err = true;
                return;
            }
            bin += REGcode[r1] + numToStr(i, 3);
            break;
        case 'T':
            if (op == "J" || op == "BEQZ"){
                if (!(buffer>>label)){
                    fout<<"Label missing"<<"\t"<<line<<endl;
                    err = true;
                    return;
                }
                if (labels.find(label) == labels.end()){
                    fout<<"Label does not exist"<<"\t"<<line<<endl;
                    err = true;
                    return;
                }
                t = labels[label] - n;
                if (!t){
                    fout<<"Can't branch to the same line"<<"\t"<<line<<endl;
                    err = true;
                    return;
                }
                --t;
            }
            else if (!(buffer>>t)){
                fout<<"Invalid T-Type Format:"<<"\t"<<line<<endl;
                err = true;
                return;
            }
            if (t >= 32 || t < -32){
                fout<<"Target can only be -32 to 31:"<<"\t"<<line<<endl;
                err = true;
                return;
            }
            bin += numToStr(t, 6);
            break;
    }
    fout<<internal<<setfill('0');
    fout<<"0x"<<hex<<uppercase<<setw(3)<<strToNum(bin)<<nouppercase<<dec<<endl;
    fout<<bin<<"\t"<<line<<endl;
}

int main(int argc, char *argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    cout.tie(NULL);

    if (argc == 1){
        fin.open("Input.txt");
        fout.open("Output.txt");
        if (!fin){
            cout<<"Default Input File (Input.txt) Does Not Exist"<<endl;
            return 0;
        }
    }
    else if (argc == 2){
        fin.open(argv[1]);
        fout.open("Output.txt");
        if (!fin){
            cout<<"Specified Input File ("<<argv[1]<<") Does Not Exist"<<endl;
            return 0;
        }
    }
    else if (argc == 3){
        fin.open(argv[1]);
        fout.open(argv[2]);
        if (!fin){
            cout<<"Specified Input File ("<<argv[1]<<") Does Not Exist"<<endl;
            return 0;
        }
    }
    else{
        cout<<"Invalid Command Line Arguments."<<endl;
        return 0;
    }

    string line, s;
    for (int i = 1; getline(fin, line); ++i){
        if (!line.empty()){
            stringstream buffer(line);
            buffer>>s;
            if (s[s.size()-1] == ':'){
                s.pop_back();
                transform(s.begin(), s.end(), s.begin(), ::toupper);
                labels[s] = i;
            }
        }
        else
            --i;
    }
    fin.clear();
    fin.seekg(0, ios::beg);

    for (int i = 1; getline(fin, line); ++i){
        if (line.empty()){
            --i;
            continue;
        }
        transform(line.begin(), line.end(), line.begin(), ::toupper);
        decode(line, i);
    }

    cout<<"Assembling Complete ";
    cout<<((err)?"With":"Without")<<" Errors"<<endl;
    fin.close();
    fout.close();
    return 0;
}
