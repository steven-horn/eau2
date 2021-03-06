// lang::cpp

#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include "object.h"
#include "dataframe.h"

using namespace std;

class Sorer: public Object {
public:
    string filename, longest_line;
    size_t from, len;
    Schema* schema;
    vector<string> goodrows;
    KVStore* kc;

    Sorer(string filename, KVStore* kc_)
    : filename(filename), kc(kc_) {
        /** set default len and from values */
        ifstream file_len (filename, ios::binary);
        streampos fsize = file_len.tellg();
        file_len.seekg(0, ios::end);
        fsize = file_len.tellg() - fsize;
        file_len.close();
        len = fsize;
        from = 0;
        find_golden_row();
        find_bool_int_true_schema();
    }

    Sorer(string filename, size_t from, int len, KVStore* kc_)
    : filename(filename), from(from), len(len), kc(kc_) {
        if (len == -1) {
            ifstream file_len (filename, ios::binary);
            streampos fsize = file_len.tellg();
            file_len.seekg(0, ios::end);
            fsize = file_len.tellg() - fsize;
            file_len.close();
            len = fsize;
        }
        find_golden_row();
        find_bool_int_true_schema();
    }

    ~Sorer() {
        delete schema;
    }

    // Function that returns count of the given
    // character in the string
    // Source: https://www.geeksforgeeks.org/program-count-occurrence-given-character-string/
    int count(char* s, const char c)
    {
      // Count variable
      int res = 0;

      for (size_t i=0;i<strlen(s);i++)
      // checking character in string
        if (s[i] == c) res++;
      return res;
    }

    /**
     * Finds the correct schema and sets it.
     * Often times boolean and integer schemas can be mistaken.
     * 1 and 0 are both integer/booleans.
     * This method solves the problem by choosing the row with the least
     * amount of booleans in it.
     * authors: horn.s@husky.neu.edu, armani.a@husky.neu.edu
     **/
     void find_bool_int_true_schema() {
       // compare to two rows of the same size to find correct schema.
       if(goodrows.size() > 1) {
         srand (time(NULL));
         int rand1 = rand() % (goodrows.size() + 1);
         this_thread::sleep_for(chrono::milliseconds(10));
         srand(time(NULL));
         int rand2 = rand() % (goodrows.size() + 1);
         if (rand1 == rand2) {
           rand1 = rand() % (goodrows.size() + 1);
           rand2 = rand() % (goodrows.size() + 1);
         }
         longest_line = goodrows.at(rand1);
         Schema* temp1 = make_schema();
         longest_line = goodrows.at(rand2);
         Schema* temp2 = make_schema();
         if (count(temp2->types_->c_str(), 'B') < count(temp1->types_->c_str(), 'B')) {
           delete temp2;
           schema = temp1;
         } else {
           delete temp1;
           schema = temp2;
         }
         goodrows.clear();
       } else {
         schema = make_schema();
         goodrows.clear();
       }
     }

    /**
     * scans the first 500 lines and returns the row number
     * with the most valid fields
     * @return: the row number of the first longest valid row
     * authors: horn.s@husky.neu.edu, armani.a@husky.neu.edu
     */
    string find_golden_row() {
        int longest_line_length = 0;
        int longest_line_empties = 0;
        string longest_str;
        string line;
        ifstream file (filename);
        if (file.is_open()) {
            int lines_read = 0;
            // loop through first 500 lines in file
            while (getline(file, line) && lines_read < 500) {
                lines_read++;
                int fields = 0;
                int empty_fields = 0;
                bool well_formed = true;
                bool open_found = false;
                bool other_found = false;
                bool quotes_found = false;
                bool quotes_ended = false;
                bool space_found = false;
                // loop through characters in the line
                for (size_t i = 0; line[i] != 0; i++) {
                    // found a field
                    if (line[i] == '<' && !quotes_found) {
                    fields++;
                    open_found = true;
                    }
                    // found a space after another character and without quotes
                    else if (other_found && !quotes_found && line[i] == ' ') {
                    space_found = true;
                    }
                    // found quotes without seeing anything else
                    else if (open_found && line[i] == '\"' && !other_found && !quotes_found) {
                    quotes_found = true;
                    other_found = true;
                    open_found = false;
                    }
                    // found something after the opening of the field
                    else if (open_found && !other_found && line[i] != ' ' && line[i] != '>') {
                    other_found = true;
                    open_found = false;
                    }
                    // found the end of the field
                    else if (line[i] == '>' && !quotes_found) {
                    if (!other_found) {
                        empty_fields += 1;
                    }
                    other_found = false;
                    space_found = false;
                    open_found = false;
                    quotes_found = false;
                    quotes_ended = false;
                    }
                    // character after quote
                    else if (quotes_ended && line[i] != ' ') {
                        well_formed = false;
                        break;
                    }
                    // found a quote after another -> quotes ended
                    else if (quotes_found && line[i] == '\"') {
                        quotes_ended = true;
                        quotes_found = false;
                    }
                    // found a space in the middle of the field-> out of the running
                    else if (space_found && !quotes_found) {
                        well_formed = false;
                        break;
                    }
                }
                // this row was well formed and it's the longest row so far
                // OR: this row was well formed and it has the same amount of
                //     fields as the longest so far, with less empty fields
                if ((well_formed && fields > longest_line_length)
                || (well_formed && fields == longest_line_length &&
                    empty_fields < longest_line_empties)) {
                    goodrows.clear();
                    longest_line_length = fields;
                    longest_line_empties = empty_fields;
                    longest_str = line;
                    goodrows.push_back(line);
                } else if (well_formed && fields == longest_line_length &&
                empty_fields == longest_line_empties) {
                  goodrows.push_back(line);
                }

            }
            file.close();
        } else {
            cout << "unable to open file" << endl;
            exit(1);
        }
        return longest_str;
    }

    /**
     * creates schema based on longest line's types
     * @returns the schema
     * authors: horn.s@husky.neu.edu, armani.a@husky.neu.edu
     */
    Schema* make_schema() {

        string line = longest_line;
        string types = "";
        // loop through characters
        for (size_t character = 0; line[character] != 0; character++) {
            string field = "";
            // found a field
            if(line[character] == '<') {
                // loop through the characters of this field
                while (line[character] != '>') {
                    character++;
                    // found a quote -> add to field regardless of spaces
                    if (line[character] == '\"') {
                        field += line[character];
                        character++;
                        while (line[character] != '\"') {
                            field += line[character];
                            character++;
                        }
                        field += line[character];
                    } else if (line[character] != ' ' && line[character] != '>') {
                        field += line[character];
                    }
                }
                // Case: field is empty -> BOOL
                if (field.size() == 0) {
                    types += 'B';
                    continue;
                }
                // Case: found a + or - (DOUBLE || INT) -> trim field
                if (field[0] == '+' || field[0] == '-') {
                    field = field.substr(1, field.length());
                }
                // Case: found a period. (DOUBLE || STRING)
                if (field.find('.') != string::npos) {
                    // try stod -> succeed = double, fail = string
                    try {
                        stod(field);
                        types += 'D';
                    } catch (...) {
                        types += 'S';
                    }
                }

                // Case: no period (INT || STRING)
                else {
                    // try stoi -> succeed = int || bool (0, 1), fail = string
                    try {
                        int n = stoi(field);
                        if (n > 1) {
                            types += 'I';
                        } else {
                            types += 'B';
                        }
                    } catch (...) {
                        types += 'S';
                    }
                }
            }
        }
        Schema* scm = new Schema(types.c_str());
        return scm;
    }

    /**
     * generates the database of the file based on inferred schema.
     * @returns a generated dataframe based on the file and schema.
     * authors: horn.s@husky.neu.edu, armani.a@husky.neu.edu
     */
    DataFrame* generate_dataframe() {
        DataFrame* df = new DataFrame(*schema, kc);
        vector<Row> validated_rows;

        string line;
        size_t bytes_read = 0;
        // Boolean to keep track of if we started vectorizing.
        bool started = false;
        // if we start at beginning of file, no need to skip rows
        if (from == 0) {
            started = true;
        }
        ifstream file (filename);
        if (file.is_open()) {
            // loop through lines of the file
            while (getline(file, line)) {
                // skip rows if not starting at 0 (or not at from yet)
                if (!started) {
                    // count the bytes in the line
                    for (size_t i = 0; line[i] != 0; i++) {
                        bytes_read++;
                    }
                    // if we have gotten to or past from, start at the next line
                    if (bytes_read >= from) {
                        bytes_read = bytes_read - from;
                        started = true;
                    }
                    continue;
                }

                // if we can't get past this line because of len, end here
                if (bytes_read + line.length() > len) {
                    break;
                }

                // this row is well formed so we can add to database
                bool well_formed = true;
                int num_field = 0;  // num of fields in this row
                vector<string> tmp; // fields in this row (before validating types)

                // loop through characters in this row
                for (size_t character = 0; line[character] != 0; character++) {
                    // NOTE: every time we increment character, we increment bytes read
                    bytes_read++;
                    string field = "";
                    // found a field
                    if(line[character] == '<') {
                    bool other_found = false;
                    bool space_found = false;
                    bool quotes_found = false;
                    character++;
                    bytes_read++;
                    // loop through the field
                    while (line[character] != '>') {
                        // found a quote
                        if (line[character] == '\"') {
                        quotes_found = true;
                        character++;
                        bytes_read++;
                        // while still in the quote, add all to field
                        while (line[character] != '\"') {
                            field += line[character];
                            character++;
                            bytes_read++;
                        }
                        }
                        // we've found something in the field
                        else if (line[character] != ' ' && line[character] != '>' && !space_found && !quotes_found) {
                        other_found = true;
                        field += line[character];
                        }
                        // found a space after value
                        else if (line[character] == ' ' && other_found) {
                        space_found = true;
                        }
                        // found a value and then a space and then another value -> not well formed
                        else if (space_found) {
                        well_formed = false;
                        }
                        // something found after quotes
                        else if (quotes_found && line[character] != ' ') {
                        well_formed = false;
                        }
                        // found end of field
                        if (line[character] == '>') {
                        other_found = false;
                        space_found = false;
                        }
                        character++;
                        bytes_read++;
                    }

                    // increment num of fields, add field to tmp
                    num_field++;
                    tmp.push_back(field);
                    }
                }
                // something in the row is not well formed -> throw out row
                if (!well_formed) {
                    continue;
                }

                // Only add in if it fits the schema.
                Row validated(*schema);
                // Determines if we throw out the row or not.
                bool isValidated = true;
                if (tmp.size() != schema->width()) continue;
                // Insert into vector
                for (size_t i = 0; i < tmp.size(); i++) {
                    // Case: missing -> no
                    if(tmp[i] == "") {
                        isValidated = false;
                        continue;
                    }
                    // Case: String, push no matter what.
                    if(schema->col_type(i) == 'S') {
                        String* str = new String(tmp[i].c_str());
                        validated.set(i, str);
                        //delete str;
                    }
                    // Case: Double, push back if one of <DOUBLE> <INT> <BOOL>
                    else if(schema->col_type(i) == 'D') {
                        try {
                            double f = stod(tmp[i]);
                            //string toAdd = "";
                            //toAdd += tmp[i];
                            //if (tmp[i].find('.') == string::npos) {
                            //    toAdd += ".0";
                            //}
                            validated.set(i, f);
                        } catch (...) {
                            try {
                                double f = stoll(tmp[i]);
                                //string toAdd = "";
                                //toAdd += tmp[i];
                                //if (tmp[i].find('.') == string::npos) {
                                //    toAdd += ".0";
                                //}
                                validated.set(i, f);
                            } catch ( ...) {
                                isValidated = false;
                                break;
                            }
                        }
                    }
                    // Case: Int, push back if one of <INT> <BOOL>
                    else if (schema->col_type(i) == 'I') {
                        // Anything with a period is thrown out.
                        if (tmp[i].find('.') != string::npos) {
                            isValidated = false;
                            break;
                        }
                        try {
                            int n = stoi(tmp[i]);
                            validated.set(i, n);
                        } catch (...) {
                            try {
                                long long int n = stoll(tmp[i]);
                                validated.set(i, (int)n);
                            } catch ( ...) {
                                isValidated = false;
                                break;
                            }
                        }
                    }

                    // Case: Bool, push back if one of <BOOL>
                    else if (schema->col_type(i) == 'B') {
                        if (strcmp(tmp[i].c_str(), "1") == 0) validated.set(i, true);
                        else if (strcmp(tmp[i].c_str(), "0") == 0) validated.set(i, false);
                        else isValidated = false;
                    }
                } // End creation of validated row
                if (isValidated) {
                  df->add_row(validated);
                }
            }
        } else {
            cout << "unable to open file" << endl;
            exit(1);
        }
        df->finalize_all();
        return df;
    }
};
