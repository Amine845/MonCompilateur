//  A compiler from a very simple Pascal-like structured language LL(k)
//  to 64-bit 80x86 Assembly langage
//  Copyright (C) 2019 Pierre Jourlin
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Build with "make compilateur"


#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

char current;				// Current car
char nextcar; // prochain caractere pour analyse LL(2)

// Modification de ReadChar pour aussi read la nextcar

void ReadChar(void){		// Read character and skip spaces until 
				// non space character is read
    current = nextcar;
    while (cin.get(nextcar) && (nextcar == ' ' || nextcar == '\t' || nextcar == '\n'))
        cin.get(nextcar);
}

void Error(string s){
	cerr<< s << endl;
	exit(-1);
}

// ArithmeticExpression := Term {AdditiveOperator Term}
// Term := Digit | "(" ArithmeticExpression ")"
// AdditiveOperator := "+" | "-"
// Digit := "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"

	
void AdditiveOperator(void){
	if(current=='+'||current=='-')
		ReadChar();
	else
		Error("Opérateur additif attendu");	   // Additive operator expected
}
		
void Digit(void){
	if((current<'0')||(current>'9'))
		Error("Chiffre attendu");		   // Digit expected
	else{
		cout << "\tpush $"<<current<<endl;
		ReadChar();
	}
}

void ArithmeticExpression(void);			// Called by Term() and calls Term()

void Term(void){
	if(current=='('){
		ReadChar();
		ArithmeticExpression();
		if(current!=')')
			Error("')' était attendu");		// ")" expected
		else
			ReadChar();
	}
	else 
		if (current>='0' && current <='9')
			Digit();
	     	else
			Error("'(' ou chiffre attendu");
}

void ArithmeticExpression(void){
	char adop;
	Term();
	while(current=='+'||current=='-'){
		adop=current;		// Save operator in local variable
		AdditiveOperator();
		Term();
		cout << "\tpop %rbx"<<endl;	// get first operand
		cout << "\tpop %rax"<<endl;	// get second operand
		if(adop=='+')
			cout << "\taddq	%rbx, %rax"<<endl;	// add both operands
		else
			cout << "\tsubq	%rbx, %rax"<<endl;	// substract both operands
		cout << "\tpush %rax"<<endl;			// store result
	}

}

// TP2

// <expression> ::= <ExpressionArithmétique> | <ExpressionArithmétique> <OpérateurRelationnel> <ExpressionArithmétique>
//                 <OpérateurRelationnel> ::= '=' | '<>' | '<' | '<=' | '>=' | '>'

string RelationalOperator(void){
    string op;

    if(current == '='){
        op = "=";
        ReadChar();
    } else if(current == '<'){
        if(nextcar == '='){
            op = "<=";
            ReadChar(); ReadChar();
        } else if(nextcar == '>'){
            op = "<>";
            ReadChar(); ReadChar();
        } else {
            op = "<";
            ReadChar();
        }
    } else if(current == '>'){
        if(nextcar == '='){
            op = ">=";
            ReadChar(); ReadChar();
        } else {
            op = ">";
            ReadChar();
        }
    } else {
        Error("Opérateur relationnel attendu");
    }

    return op;
}

void Expression(void){
    ArithmeticExpression();

    // Test si on est face à un opérateur relationnel
    if(current == '=' || current == '<' || current == '>'){
        string op = RelationalOperator();
        ArithmeticExpression();

        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        cout << "\tcmp %rbx, %rax" << endl;

        if(op == "=")
            cout << "\tsete %al" << endl;
        else if(op == "<>")
            cout << "\tsetne %al" << endl;
        else if(op == "<")
            cout << "\tsetl %al" << endl;
        else if(op == "<=")
            cout << "\tsetle %al" << endl;
        else if(op == ">")
            cout << "\tsetg %al" << endl;
        else if(op == ">=")
            cout << "\tsetge %al" << endl;

        cout << "\tmovzbq %al, %rax" << endl;
        cout << "\tpush %rax" << endl;
    }
}


// TP3

bool declared[26] = { false }; // Toutes les lettres non déclarées par défaut

void DeclarationPart() {
    if (current == '[') {
        cout << ".data" << endl;
        ReadChar();  // Consomme le '['

        if ( (current < 'a') || (current > 'z')){
            Error("Lettre attendue après '['");
        }

        while (true) {
            int idx = current - 'a';
            if (declared[idx])
                Error("Variable déjà déclarée");
            declared[idx] = true;

            cout << (char)current << ":\t.quad 0" << endl;
            ReadChar();

            if (current == ']') {
                ReadChar();
                break;
            }
            else if (current == ',') {
                ReadChar();
                if ((current < 'a') || current > 'z')
                    Error("Lettre attendue après ','");
            }
            else {
                Error("',' ou ']' attendu");
            }
        }
    }
}


int main(void){	// First version : Source code on standard input and assembly code on standard output
	// Header for gcc assembler / linker
	cout << "\t\t\t# This code was produced by the CERI Compiler"<<endl;
	cout << "\t.text\t\t# The following lines contain the program"<<endl;
	cout << "\t.globl main\t# The main function must be visible from outside"<<endl;
	cout << "main:\t\t\t# The main function body :"<<endl;
	cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top"<<endl;

	// Let's proceed to the analysis and code production
	ReadChar();
    ReadChar();

    Expression();

    ReadChar();
	// Trailer for the gcc assembler / linker
	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top"<<endl;
	cout << "\tret\t\t\t# Return from main function"<<endl;
	if(cin.get(current)){
		cerr <<"Caractères en trop à la fin du programme : ["<<current<<"]";
		Error("."); // unexpected characters at the end of program
	}

}
		
			





