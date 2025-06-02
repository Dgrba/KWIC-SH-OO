#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <memory>

using namespace std;

//Method to sort words case-sensitively prioritizing lower-case
bool caseSensitive(const string& a, const string& b) {
    for (size_t i = 0; i < min(a.size(), b.size()); i++) {
        char ca = a[i], cb = b[i];
        if (tolower(ca) != tolower(cb)) {
            return tolower(ca) < tolower(cb);
        }
        if (ca != cb) {
            return islower(ca) && isupper(cb);
        }
    }
    return a.size() < b.size();
}

string toLowerString(const string& str) {
    string result;
    for (auto &c : str) {
        result += tolower(c); // Convert each character to lowercase
    }
    return result;
}

//Method to determine if word provided is found in the noise word list. 
bool isNoiseWord(const string& word, const shared_ptr<vector<string>>& noiseWords) {
    return find(noiseWords->begin(), noiseWords->end(), toLowerString(word)) != noiseWords->end();
}

//This class implements shared memory for inital input filtering and line storage
class LineRepo {
private:
    shared_ptr<vector<string>> lines;
    shared_ptr<vector<vector<string>>> lines2d;
public:
    shared_ptr<vector<string>> noiseWords;

    LineRepo() : 
    lines(make_shared<vector<string>>()),
    lines2d(make_shared<vector<vector<string>>>()),
    noiseWords(make_shared<vector<string>>()) {}
    
    shared_ptr<vector<string>> getLines(){
        return lines;
    }

    shared_ptr<vector<vector<string>>> getLines2d(){
        return lines2d;
    }

    void add(string str){
        lines->push_back(str);
    }

    void add(vector<string> vect){
        lines2d->push_back(vect);
    }
};

//This class implements shared memory for circular shift filtering and shifted line storage
class ShiftRepo {
private:
    shared_ptr<vector<string>> shifts;
public:

    ShiftRepo() : 
    shifts(make_shared<vector<string>>()){}
    
    shared_ptr<vector<string>> getShifts(){
        return shifts;
    }

    void add(string str){
        shifts->push_back(str);
    }
};

//This class implements shared memory for sorted circular shift filtering and sorted shifted line storage
class SortedShiftRepo {
private:
    shared_ptr<vector<string>> sortedShifts;
public:

    SortedShiftRepo() : 
    sortedShifts(make_shared<vector<string>>()){}
    
    shared_ptr<vector<string>> getSortedShifts(){
        return sortedShifts;
    }

    void add(string str){
        sortedShifts->push_back(str);
    }

    void copy(shared_ptr<vector<string>> sharedVect){
        sortedShifts = sharedVect;
    }
};

//This class takes the input filename from shared storage and handles input processing.
class InputFilter {
public:
    void process(string inputFile, string noiseWordFile, LineRepo& lines){

        //Error thrown when noise word file cannot be opened.
        ifstream nwfstream(noiseWordFile);
        if(!nwfstream) {
            cerr << "Error opening noise words file" << endl;
            exit(1);
        }

        //Store noise words from noiseWordFile in shared memory (stored in lowercase for case-insensitive comparison)
        string nwline;
        while(getline(nwfstream, nwline)){
            stringstream ss(nwline);
            string noiseWord;
            while(ss >> noiseWord){   
                lines.noiseWords->push_back(toLowerString(noiseWord)); 
            }
        }
        nwfstream.close();

        //Error thrown when input file cannot be opened.
        ifstream ifstream(inputFile);
        if(!ifstream) {
            cerr << "Error opening input file" << endl;
            exit(1);
        }

        //Store lines from inputFile and words separated from each line in shared memory
        string line;
        while(getline(ifstream, line)){
            vector<string> words;
            stringstream ss(line);
            string word;
            while(ss >> word){   
                words.push_back(word); 
            }
            lines.add(words);
        }
        ifstream.close();
    }
};

//This class takes in the filtered input data from shared storage and circular-shifts the elements.
class CircularShiftFilter {
    public: 
        void process(LineRepo& lines, ShiftRepo& shifts){
            for(auto &vect : *lines.getLines2d()){
                size_t size = vect.size();
                for(size_t i = 0; i < size; ++i){
                    if(!isNoiseWord(vect[0], lines.noiseWords)){
                        string temp;
                        for(auto &w : vect){
                            temp += w + " ";
                        }
                        shifts.add(temp);
                    }
                    rotate(vect.begin(), vect.begin() + 1, vect.end());
                }
            }
        }
    };
    

//This class takes the circular-sorted lines from shared storage and sorts them alphabetically.
class AlphabetizerFilter {
public:
    void process(ShiftRepo& shifts, SortedShiftRepo& sortedShifts){
        sortedShifts.copy(shifts.getShifts());
        sort((*sortedShifts.getSortedShifts()).begin(), (*sortedShifts.getSortedShifts()).end(), caseSensitive);
    }
};

//This class outputs the sorted circular-shifted lines to the user.
class OutputFilter {
public:
    void process(SortedShiftRepo& sortedShifts){
        for(auto &line : *sortedShifts.getSortedShifts()){
            cout << "\n" << line << endl;
        }
    }
};

//This class simplifies the operations needed in the main function and creates a pipeline for ease of modification
class Pipeline {
public:
    void process(const string& inputFile, const string& noiseWord, LineRepo& lines, ShiftRepo& shifts, SortedShiftRepo& sortedShifts) {
        InputFilter input;
        CircularShiftFilter shift;
        AlphabetizerFilter sort;
        OutputFilter output;

        input.process(inputFile, noiseWord, lines);
        shift.process(lines, shifts);
        sort.process(shifts, sortedShifts);
        output.process(sortedShifts);
    }
};

int main (int argc, char* argv[]){

    //Errors thrown if argument count is off.
    if(argc < 3){
        cerr << "Error: Not enough arguments in command line.\n"
                "Please input arguments in the correct format:\n\n"
                "./program_name input_filename noise_words_filename" << endl;
        return 1;
    }
    if(argc > 3){
        cerr << "Error: Too many arguments in command line.\n"
                "Please input arguments in the correct format:\n\n"
                "./program_name input_filename noise_words_filename" << endl;
        return 1;
    }

    Pipeline pipeline;
    LineRepo lines;
    ShiftRepo shifts;
    SortedShiftRepo sortedShifts;

    //Start of the clock for benchmarking
    auto start = chrono::steady_clock::now();

    //Main process
    pipeline.process(argv[1], argv[2], lines, shifts, sortedShifts);

    //End of the clock for benchmarking + duration calculation
    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

    cout << "\n\n" << duration.count() << " microseconds to complete." << endl;

    return 0;
}