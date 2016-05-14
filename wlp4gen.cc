#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <stdlib.h>
#include <utility>
#include <algorithm>


using namespace std;

struct tNode {
	string rule;
	vector<string> ruleTokens;
	vector<tNode*> children;
	vector<string> childrenTypes;
};

bool isTerminal (const string &symbol)
{
	if (symbol == "BOF" || symbol == "BECOMES" || symbol == "COMMA" || symbol == "ELSE" || symbol == "EOF" || symbol == "EQ" || symbol == "GE" || symbol == "GT" || symbol == "ID" || symbol == "IF" || symbol == "INT" || symbol == "LBRACE" || symbol == "LE" || symbol == "LPAREN" || symbol == "LT" || symbol == "MINUS" || symbol == "NE" || symbol == "NUM" || symbol == "PCT" || symbol == "PLUS" || symbol == "PRINTLN" || symbol == "RBRACE" || symbol == "RETURN" || symbol ==  "RPAREN" || symbol == "SEMI" || symbol == "SLASH" || symbol == "STAR" || symbol == "WAIN" || symbol == "WHILE" || symbol == "AMP" || symbol == "LBRACK" || symbol == "RBRACK" || symbol == "NEW" || symbol == "DELETE" || symbol == "NULL")
	{
		return true;
	}
	else
	{
		return false;
	}
}


int currentIndex = 0;
void buildTree (vector<tNode*> &treeNodes, tNode* root) {

	int numChildren;

	//If LHS of the rule is a terminal, it's a leaf
	if (isTerminal(root->ruleTokens.at(0)))
	{
		numChildren = 0;
	}
	else //Otherwise it's number of children = # of symbols on RHS 
	{
		numChildren = root->ruleTokens.size() - 1; 
	}

	//Enters this loop if node has children
 	for (int i = 1; i <= numChildren; i++) {
 		root->children.push_back(treeNodes.at(++currentIndex)); //Link the parent to it's FIRST child
 		buildTree(treeNodes, treeNodes.at(currentIndex)); //Recursively call the function to check if it's child has children and, if so, link it's children to itself first before continuing.
 	}
 	return; //Function returns if the node has no children (thus allowing the for loop to continue executing)

}

void tokenizeLine (const string &line, vector<string> &ruleTokens)
{
	istringstream ss (line);
	string token;

	while (ss >> token)
	{
		ruleTokens.push_back(token);
	}
}


// map<string, pair<vector<string>, map<string,string> > > topSymTbl;

map<string, pair<vector<string>, map<string,pair<string,int> > > > topSymTbl;
string currProcedure; 

int currOffset = 0;

void buildSymbolTable(tNode* root)
{	
	//Check if the rule is a declaration and if so, extract the type and name 
	if (root->rule == "dcl type ID")
	{
		string name = root->children.at(1)->ruleTokens.at(1);
		string type = root->children.at(0)->rule;


		if (type == "type INT")
		{
			type = "int";
		}
		else if (type == "type INT STAR")
		{
			type = "int*";
		}


		//Attempt to add to symbol table
		if (topSymTbl[currProcedure].second.find(name) != topSymTbl[currProcedure].second.end())
		{
			//Variable name has already been declared/exists, so error out
			cerr << "ERROR: variable name " << name << " has already been declared." << endl;
			exit(0);
		}
		else //Add to symbol table
		{ 
			topSymTbl[currProcedure].second[name].first = type;
			topSymTbl[currProcedure].second[name].second = currOffset;
			//cout << "Current Procedure: " << currProcedure << " ID: " << name << " Type: " << type << " Offset: " << currOffset << endl;
			currOffset -= 4;

		}
	}
	//Check if the rule is one where an ID is referenced and extract it to see if it has been declared
	else if (root->rule == "factor ID" || root->rule == "lvalue ID")
	{
		string id = root->children.at(0)->ruleTokens.at(1);
		if (topSymTbl[currProcedure].second.find(id) == topSymTbl[currProcedure].second.end())
		{
			cerr << "ERROR: the variable " << id << " has been used without being declared." << endl;
			exit(0);
		}
	}
	//New procedure definition
	else if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE" || root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
	{
		//Extract the procedure name from the node
		string procName = root->children.at(1)->ruleTokens.at(1);
		//Check to see if a procedure with that name has already been defined.
		if (topSymTbl.find(procName) != topSymTbl.end())
		{
			cerr << "ERROR: procedure named " << procName << " has already been defined." << endl;
			exit(0);
		}
		//Otherwise, add it to our top-level symbol table
		else
		{
			//Initiliaze the signature of the procedure
			vector<string> procSignature; 
			//Initialize a symbol table for the new procedure
			map<string,pair<string,int> > newProcSymTbl; 
			//Initialize a pair for the signature and procedure symbol table
			pair<vector<string>, map<string,pair<string,int> > > sigSymPair (procSignature,newProcSymTbl);
			//Map the procedure name to the initialized pair
			topSymTbl[procName] = sigSymPair;
			//Set the global variable currProcedure to the current procedure
			currProcedure = procName;
			//Reset the stack frame offset
			currOffset = 0;
		}

		//If the procedure definition was for wain, extract the paramter types and add them to the signature
		if (root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
		{
			string paramType1 = root->children.at(3)->children.at(0)->rule;
			string paramType2 = root->children.at(5)->children.at(0)->rule;

			if (paramType1 == "type INT")
			{
				paramType1 = "int";
			}
			else if (paramType1 == "type INT STAR")
			{
				paramType1 = "int*";
			}	

		    if (paramType2 == "type INT")
			{
				paramType2 = "int";
			}
			else if (paramType2 == "type INT STAR")
			{
				paramType2 = "int*";
				// cerr << "ERROR: the second parameter of wain cannot be int*" << endl; exit(0);
			}	
			//Add wain types to signature vector
			topSymTbl["wain"].first.push_back(paramType1);
			topSymTbl["wain"].first.push_back(paramType2);
		}

	}
	//Signature definition
	else if (root->rule == "paramlist dcl" || root->rule == "paramlist dcl COMMA paramlist")
	{
		//Extract the type of the parameter
		string paramType = root->children.at(0)->children.at(0)->rule;
		if (paramType == "type INT")
		{
			paramType = "int";
		}
		else if (paramType == "type INT STAR")
		{
			paramType = "int*";
		}
		//Add type to currProcedure's signature vector
		topSymTbl[currProcedure].first.push_back(paramType);
	}
	//Attempting to call a procedure
	else if (root->rule == "factor ID LPAREN RPAREN" || root->rule == "factor ID LPAREN arglist RPAREN")
	{
		string procName = root->children.at(0)->ruleTokens.at(1);
		//Check if procedure called hasn't been declared and if so error out.
		if (topSymTbl.find(procName) == topSymTbl.end())
		{
			cerr << "ERROR: procedure " << procName << " was called but not declared." << endl;
			exit(0);
		}
		//Check if procedure attempting to be called is also declared as a variable with the same name and if so error out.
		else if (topSymTbl[currProcedure].second.find(procName) != topSymTbl[currProcedure].second.end())
		{
			cerr << "ERROR: the procedure " << procName << " was called is also declared as a variable." << endl;
			exit(0);
		}
	}

	//If it's none of the special cases above, continue recursing through the tree
	for (vector<tNode*>::iterator it = root->children.begin(); it != root->children.end(); it++)
	{
		buildSymbolTable(*it);
	}

}

// void printSymbolTable()
// {
// 	for (map<string, pair<vector<string>, map<string,string> > >::iterator it1 = topSymTbl.begin(); it1 != topSymTbl.end(); it1++)
// 	{
// 		//Print the proc name
// 		cerr << it1->first << " ";
// 		//Print the proc's signature
// 		for (vector<string>::iterator vec = it1->second.first.begin(); vec != it1->second.first.end(); vec++)
// 		{
// 			//Check if we're printing the LAST element of the signature
// 			if (vec + 1 == it1->second.first.end())
// 			{
// 				cerr << *vec;
// 			}
// 			else 
// 			{
// 				cerr << *vec << " ";
// 			}
// 		}
// 		cerr << endl;
// 		//Print the proc's symbol table
// 		for (map<string,string>::iterator it2 = it1->second.second.begin(); it2 != it1->second.second.end(); it2++)
// 		{
// 			cerr << it2->first << " " << it2->second << endl;
// 		}	
// 		//Print extra newline between procs
// 		cerr << endl;
// 	}
// }


//Global vector<string> to temporarily store a proc's arglist for comparison
vector<string> tempArgList;


string typeOf(tNode* root)
{

	if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
	{
		//Update currProcedure (for context when returning ID types)
		currProcedure = root->children.at(1)->ruleTokens.at(1);
	}
	else if (root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
	{
		//Update currProcedure (for context when returning ID types)
		currProcedure = root->children.at(1)->ruleTokens.at(1);
	}

	//Process your children's types first and push them into a local vector
	vector<string> childTypes;
	for (vector<tNode*>::iterator it = root->children.begin(); it != root->children.end(); it++)
	{
		childTypes.push_back(typeOf(*it));
	}

	//Once you've exhausted your children's types, determine your type

	//NUM is always of type int
	if (root->rule == "factor NUM")
	{
		return "int";
	}
	//NULL is always of type int*
	else if (root->rule == "factor NULL")
	{
		return "int*";
	}
	//If we have an ID, look up and return it's type
	else if (root->rule == "factor ID" || root->rule == "lvalue ID")
	{
		return topSymTbl[currProcedure].second[root->children.at(0)->ruleTokens.at(1)].first;
	}
	//Case for (E)
	else if (root->rule == "lvalue LPAREN lvalue RPAREN" || root->rule == "factor LPAREN expr RPAREN")
	{
		if (childTypes.at(1) == "int" || childTypes.at(1) == "int*")
		{
			return childTypes.at(1);  
		}
		else
		{
			cerr << "ERROR: 0" << endl;
			exit(0);
		}
	}
	//Singleton productions (type of LHS == type of RHS)
	else if (root->rule == "expr term" || root->rule == "term factor")
	{
		return childTypes.at(0);
	}
	//Pointer case 1: &E
	else if (root->rule == "factor AMP lvalue")
	{
		if (childTypes.at(1) == "int")
		{
			return "int*";
		}
		else
		{
			cerr << "ERROR: 1" << endl;
			exit(0);
		}
	}
	//Pointer case 2: *E
	else if (root->rule == "factor STAR factor" || root->rule == "lvalue STAR factor")
	{
		if (childTypes.at(1) == "int*")
		{
			return "int";
		}
		else
		{
			cerr << "ERROR: 2" << endl;
			exit(0);
		}
	}
	else if (root->rule == "factor NEW INT LBRACK expr RBRACK")
	{
		if (childTypes.at(3) == "int")
		{
			return "int*";
		}
		else
		{
			cerr << "ERROR: 3" << endl;
			exit(0);
		}
	}
	else if (root->rule == "expr expr PLUS term")
	{
		string E1 = childTypes.at(0);
		string E2 = childTypes.at(2);

		root->childrenTypes.push_back(E1);
		root->childrenTypes.push_back(E2);

		if (E1 == "int" && E2 == "int")
		{
			return "int";
		}
		else if (E1 == "int*" && E2 == "int")
		{
			return "int*";
		}
		else if (E1 == "int" && E2 == "int*")
		{
			return "int*";
		}
		else
		{
			cerr << "ERROR: 4" << endl;
			exit(0);
		}
	}
	else if (root->rule == "expr expr MINUS term")
	{
		string E1 = childTypes.at(0);
		string E2 = childTypes.at(2);

		root->childrenTypes.push_back(E1);
		root->childrenTypes.push_back(E2);

		if (E1 == "int" && E2 == "int")
		{
			return "int";
		}
		else if (E1 == "int*" && E2 == "int")
		{
			return "int*";
		}
		else if (E1 == "int*" && E2 == "int*")
		{
			return "int";
		}
		else
		{
			cerr << "ERROR: 5" << endl;
			exit(0);
		}
	}
	else if (root->rule == "term term STAR factor" || root->rule == "term term SLASH factor" || root->rule == "term term PCT factor")
	{
		string E1 = childTypes.at(0);
		string E2 = childTypes.at(2);

		if (E1 == "int" && E2 == "int")
		{
			return "int";
		}
		else
		{
			cerr << "ERROR: 6" << endl;
			exit(0);
		}
	}
	//Parameterless procedure call
	else if (root->rule == "factor ID LPAREN RPAREN")
	{
		string procName = root->children.at(0)->ruleTokens.at(1);
		if (topSymTbl.find(procName) == topSymTbl.end())
		{
			cerr << "ERROR: 7" << endl;
			exit(0);
		}
		else
		{
			return "int";
		}
	}
	//Procedure call with parameters
	else if (root->rule == "factor ID LPAREN arglist RPAREN")
	{
		string procName = root->children.at(0)->ruleTokens.at(1);
		if (topSymTbl.find(procName) == topSymTbl.end())
		{
			cerr << "ERROR: 9" << endl;
			exit(0);
		}

		vector<string> procSig = topSymTbl[procName].first;

		//Compute the types of the arglist of this procedure
		//Note the result will be stored in the tempArgList vector

		//Reverse the arglist vector (recursion has it backwards)
		reverse(tempArgList.begin(),tempArgList.end());

		//Compare the contents of the tempArgList vector to our proc's sig. defn
		//tempArgList MUST be equal to proc's sig. in order for this to be accepted
		if (tempArgList.size() == procSig.size() && equal(tempArgList.begin(), tempArgList.end(), procSig.begin()))
		{
			tempArgList.erase(tempArgList.begin(),tempArgList.end()); //Purge the vector for future use 
			return "int";
		}
		else
		{
			cerr << "ERROR: 10" << endl;
			exit(0);
		}
	}
	else if (root->rule == "arglist expr" || root->rule == "arglist expr COMMA arglist")
	{	
		if (childTypes.at(0) == "int" || childTypes.at(0) == "int*")
		{
			tempArgList.push_back(childTypes.at(0));
			return "WT";
		}
		else
		{
			cerr << "ERROR: 11" << endl;
			exit(0);
		}
	}
	//Comparison cases
	else if (root->rule == "test expr EQ expr" || root->rule == "test expr NE expr" || root->rule == "test expr LT expr" || root->rule == "test expr LE expr" || root->rule == "test expr GE expr" || root->rule == "test expr GT expr")
	{
		string E1Type = childTypes.at(0);
		string E2Type = childTypes.at(2);

		root->childrenTypes.push_back(E1Type);
		root->childrenTypes.push_back(E2Type);

		if (E1Type == E2Type)
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 12" << endl;
			exit(0);
		}
	}
	//Control flow case 1: while loop
	else if (root->rule == "statement WHILE LPAREN test RPAREN LBRACE statements RBRACE")
	{
		string T = childTypes.at(2);
		string S = childTypes.at(5);

		if (T == "WT" && S == "WT")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 13" << endl;
			exit(0);
		}
	}
	//Control flow case 2: if-else 
	else if (root->rule == "statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE")
	{
		string T = childTypes.at(2);
		string S1 = childTypes.at(5);
		string S2 = childTypes.at(9);

		if (T == "WT" && S1 == "WT" && S2 == "WT")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 14" << endl;
			exit(0);
		}
	}
	//Deallocation
	else if (root->rule == "statement DELETE LBRACK RBRACK expr SEMI")
	{
		if (childTypes.at(3) == "int*")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 15" << endl;
			exit(0);
		}
	}
	//Printing
	else if (root->rule == "statement PRINTLN LPAREN expr RPAREN SEMI")
	{
		if (childTypes.at(2) == "int")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 16" << endl;
			exit(0);
		}
	}
	//Assignment
	else if (root->rule == "statement lvalue BECOMES expr SEMI")
	{
		string E1Type = childTypes.at(0);
		string E2Type = childTypes.at(2);

		if (E1Type == E2Type)
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 17" << endl;
			exit(0);
		}
	}
	//Sequencing case 1: no statement
	else if (root->rule == "statements")
	{
		return "WT";
	}
	//Sequencing case 2: statements
	else if (root->rule == "statements statements statement")
	{
		string S1 = childTypes.at(0);
		string S2 = childTypes.at(1);

		if (S1 == "WT" && S2 == "WT")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 18" << endl;
			exit(0);
		}
	}
	//Declarations case 1: no declaration
	else if (root->rule == "dcls")
	{
		return "WT";	
	}
	//Declarations case 2: declaring an int
	else if (root->rule == "dcls dcls dcl BECOMES NUM SEMI")
	{
		string dclsType = childTypes.at(0);
		string dclRule = root->children.at(1)->children.at(0)->rule;

		if (dclsType == "WT" && dclRule == "type INT")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 19" << endl;
			exit(0);
		}
	}
	//Declarations case 3: declaring a pointer
	else if (root->rule == "dcls dcls dcl BECOMES NULL SEMI")
	{
		string dclsType = childTypes.at(0);
		string dclRule = root->children.at(1)->children.at(0)->rule;

		if (dclsType == "WT" && dclRule == "type INT STAR")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 20" << endl;
			exit(0);
		}
	}
	//Procedure case 1: wain
	else if (root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
	{
		string dcl2Rule = root->children.at(5)->children.at(0)->rule;
		string dclsType = childTypes.at(8);
		string SType = childTypes.at(9);
		string EType = childTypes.at(11);


		if (dcl2Rule == "type INT" && dclsType == "WT" && SType == "WT" && EType == "int")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 21" << endl;
			exit(0);
		}
	}
	//Procedure case 2: procedure other than wain
	else if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
	{
		string dclsType = childTypes.at(6);
		string SType = childTypes.at(7);
		string EType = childTypes.at(9);

		if (dclsType == "WT" && SType == "WT" && EType == "int")
		{
			return "WT";
		}
		else
		{
			cerr << "ERROR: 22" << endl;
			exit(0);
		}
	}
	else 
	{
		return "";
	}

}


void pushReg3()
{
	cout << "sw $3, -4($30)" << endl;
	cout << "sub $30, $30, $4" << endl;
}

void popIntoReg5()
{
	cout << "lw $5, 0($30)" << endl;
	cout << "add $30, $30, $4" << endl;
}

void pushReg31()
{
	cout << "sw $31, -4($30)" << endl;
	cout << "sub $30, $30, $4" << endl;
}

void popIntoReg31()
{
	cout << "lw $31, 0($30)" << endl;
	cout << "add $30, $30, $4" << endl;
}



void genMainPro(const string &param1Type)
{
	cout << "; Main prologue" << endl;
	//Imports
	cout << ".import print" << endl;

	cout << ".import init" << endl;
	cout << ".import new" << endl;
	cout << ".import delete" << endl;


	//Set constant registers
	cout << "lis $4" << endl;
	cout << ".word 4" << endl;

	cout << "lis $10" << endl;
	cout << ".word print" << endl;

	cout << "lis $11" << endl;
	cout << ".word 1" << endl;

	//Initialize frame pointer
	cout << "sub $29, $30, $4" << endl;

	//Push $1 and $2 (params of wain) onto the stack
	cout << "sw $1, -4($30)" << endl;
	cout << "sub $30, $30, $4" << endl;
	cout << "sw $2, -4($30)" << endl;
	cout << "sub $30, $30, $4" << endl;

	//First param of wain is int, so $2 <- 0 for init
	if (param1Type == "type INT")
	{
		cout << "add $2, $0, $0" << endl;
	}
	//Second param of wain is int*, so $2 has the size of the array
	else if (param1Type == "type INT STAR")
	{
		//No-op
	}

	//Call init
	pushReg31();
	cout << "lis $5" << endl;
	cout << ".word init" << endl;
	cout << "jalr $5" << endl;
	popIntoReg31();

	cout << "; End of main prologue" << endl;
}

void genMainEpi()
{
	cout << "; Main epilogue" << endl;
	//Restore contents of the stack pointer ($30) and return
	cout << "add $30, $29, $4" << endl;
	cout << "jr $31" << endl;
}


string convertIntToString (int num)
{
	ostringstream oss;
	oss << num;

	return oss.str();
}


void print()
{
	pushReg31();
	cout << "jalr $10" << endl;
	popIntoReg31();
}

void alloc()
{
	pushReg31();
	cout << "lis $5" << endl;
	cout << ".word new" << endl;
	cout << "jalr $5" << endl;
	popIntoReg31();
}


void dealloc()
{
	pushReg31();
	cout << "lis $5" << endl;
	cout << ".word delete" << endl;
	cout << "jalr $5" << endl;
	popIntoReg31();
}


//map<string, pair<vector<string>, map<string,pair<string,int> > > > topSymTbl;

void updateOffsets(const string& procName)
{
	int numArgs = topSymTbl[procName].first.size();
	int updateVal = numArgs * 4;

	//Add the updateVal to all the offsets in the procedures symbol table
	for (map<string,pair<string,int> >::iterator it = topSymTbl[procName].second.begin(); it != topSymTbl[procName].second.end(); it++)
	{
		it->second.second += updateVal;
		// cout << "ID: " << it->first << "New Offset: " << it->second.second << endl;
	}
}




int whileCount = 0;
int ifCount = -1;
int deleteCount = 0;



void genASMCode(tNode* root)
{	
	//No code gen needed so skip
	if (root->rule == "start BOF procedures EOF")
	{
		genASMCode(root->children.at(1));
	}
	//No code gen needed so skip
	else if (root->rule == "procedures main")
	{
		genASMCode(root->children.at(0));
	}
	else if (root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
	{
		//Update currProcedure (for context when working with ID's)
		currProcedure = root->children.at(1)->ruleTokens.at(1);

		//Determine the type of the first param of wain
		string param1Type = root->children.at(3)->children.at(0)->rule;

		//Generate the wain prologue
		genMainPro(param1Type);

		//Recursively generate code for it's children
		genASMCode(root->children.at(8));
		genASMCode(root->children.at(9));
		genASMCode(root->children.at(11));

		genMainEpi();
	}
	else if (root->rule == "dcls")
	{
		return;
	}
	else if (root->rule == "statements")
	{
		return;
	}
	else if (root->rule == "expr term")
	{
		genASMCode(root->children.at(0));
	}
	else if (root->rule == "term factor")
	{
		genASMCode(root->children.at(0));
	}
	else if (root->rule == "factor ID")
	{	
		string id = root->children.at(0)->ruleTokens.at(1);
		string offset = convertIntToString(topSymTbl[currProcedure].second[id].second);
		cout << "lw $3, " << offset << "($29)" << endl;
		return;
	}
	else if (root->rule == "factor LPAREN expr RPAREN")
	{
		genASMCode(root->children.at(1));
	}
	else if (root->rule == "expr expr PLUS term")
	{
		//$3 <- expr2
		genASMCode(root->children.at(0));
		//Push $3 onto the stack
		pushReg3();
		//$3 <- term
		genASMCode(root->children.at(2));
		//Pop the old contents of $3 (from expr2) into $5
		popIntoReg5();


		//$5 = expr
		//$3 = term

		//Cases for pointer arithmetic
		//Case 1: int* + int
		if (root->childrenTypes.at(0) == "int*" && root->childrenTypes.at(1) == "int")
		{
			//$3 * 4
			cout << "mult $3, $4" << endl;
			//$3 <- $3 * 4
			cout << "mflo $3" << endl;
		}
		//Case 2: int + int*
		else if (root->childrenTypes.at(0) == "int" && root->childrenTypes.at(1) == "int*")
		{
			cout << "mult $5, $4" << endl;
			cout << "mflo $5" << endl;
		}
		else
		{
			//No-op (both int's so nothing additional need be done)
		}


		//Perform the addition
		cout << "add $3, $5, $3" << endl;
		return;
	}
	else if (root->rule == "expr expr MINUS term")
	{
		//$3 <- expr2
		genASMCode(root->children.at(0));
		//Push $3 onto the stack
		pushReg3();
		//$3 <- term
		genASMCode(root->children.at(2));
		//Pop the old contents of $3 (from expr2) into $5
		popIntoReg5();


		//$5 = expr
		//$3 = term

		//Cases for pointer arithmetic
		//Case 1: int* - int
		if (root->childrenTypes.at(0) == "int*" && root->childrenTypes.at(1) == "int")
		{
			//$3 * 4
			cout << "mult $3, $4" << endl;
			//$3 <- $3 * 4
			cout << "mflo $3" << endl;
			//Peform the subtraction
			cout << "sub $3, $5, $3" << endl;
		}
		//Case 2: int* - int*
		else if (root->childrenTypes.at(0) == "int*" && root->childrenTypes.at(1) == "int*")
		{
			cout << "sub $3, $5, $3" << endl;
			cout << "div $3, $4" << endl;
			cout << "mflo $3" << endl;
		}
		else
		{
			//Otherwise, perform the subtraction as normal
			cout << "sub $3, $5, $3" << endl;
		}

		return;
	}
	else if (root->rule == "term term STAR factor")
	{
		//$3 <- term
		genASMCode(root->children.at(0));
		//Push $3 onto the stack
		pushReg3();
		//$3 <- factor
		genASMCode(root->children.at(2));
		//Pop the old contents of $3 into $5
		popIntoReg5();
		//Perform the multiplication
		cout << "mult $5, $3" << endl;
		//Move the product to $3
		cout << "mflo $3" << endl;
		return;
	}
	else if (root->rule == "term term SLASH factor")
	{
		//$3 <- term
		genASMCode(root->children.at(0));
		//Push $3 onto the stack
		pushReg3();
		//$3 <- factor
		genASMCode(root->children.at(2));
		//Pop the old contents of $3 into $5
		popIntoReg5();
		//Perform the division
		cout << "div $5, $3" << endl;
		//Move the quotient to $3
		cout << "mflo $3" << endl;
		return;
	}
	else if (root->rule == "term term PCT factor")
	{
		//$3 <- term
		genASMCode(root->children.at(0));
		//Push $3 onto the stack
		pushReg3();
		//$3 <- factor
		genASMCode(root->children.at(2));
		//Pop the old contents of $3 into $5
		popIntoReg5();
		//Perform the division
		cout << "div $5, $3" << endl;
		//Move the remainder to $3
		cout << "mfhi $3" << endl;
		return;
	}
	else if (root->rule == "factor NUM")
	{
		string num = root->children.at(0)->ruleTokens.at(1);
		cout << "lis $3" << endl;
		cout << ".word " << num << endl;
		return;
	}
	else if (root->rule == "statements statements statement")
	{
		genASMCode(root->children.at(0));
		genASMCode(root->children.at(1));
	}
	else if (root->rule == "statement PRINTLN LPAREN expr RPAREN SEMI")
	{
		//$3 <- expr
		genASMCode(root->children.at(2));
		//$1 <- $3
		cout << "add $1, $3, $0" << endl;
		//Call print procedure
		print();
	}
	else if (root->rule == "dcls dcls dcl BECOMES NUM SEMI")
	{
		genASMCode(root->children.at(0));

		//$3 <- NUM
		string num = root->children.at(3)->ruleTokens.at(1);
		cout << "lis $3" << endl;
		cout << ".word " << num << endl;
		
		//Store $3 at the appropriate offset relative to $29 based on ID
		string id = root->children.at(1)->children.at(1)->ruleTokens.at(1);
		string offset = convertIntToString(topSymTbl[currProcedure].second[id].second);
		cout << "sw $3, " << offset << "($29)" << endl; 
		cout << "sub $30, $30, $4" << endl; //Decrement stack pointer
		return;
	}
	else if (root->rule == "statement lvalue BECOMES expr SEMI")
	{
		//$3 <- expr
		genASMCode(root->children.at(2));

		//Reach into the tree to grab the prod. rule applied to lvalue
		string lvalueRule = root->children.at(0)->rule;

		tNode* temp = root->children.at(0);

		if (lvalueRule == "lvalue LPAREN lvalue RPAREN")
		{
			while (lvalueRule == "lvalue LPAREN lvalue RPAREN")
			{
				//Dive into the lvalue child + update the lvalue Rule
				lvalueRule = temp->children.at(1)->rule;
				//Update temp node
				temp = temp->children.at(1);
			}
		}

		if (lvalueRule == "lvalue ID")
		{
			string id = temp->children.at(0)->ruleTokens.at(1);
			string offset = convertIntToString(topSymTbl[currProcedure].second[id].second);
			cout << "sw $3, " << offset << "($29)" << endl;	
		}
		else if (lvalueRule == "lvalue STAR factor")
		{
			//Push $3 onto the stack
			pushReg3();

			//$3 <- factor
			genASMCode(temp->children.at(1));

			//Pop into $5
			popIntoReg5();

			//Store the value in $5 at the address specified by $3
			cout << "sw $5, 0($3)" << endl;
		}

		return;

	}
	else if (root->rule == "test expr LT expr")
	{
		//$3 <- expr1
		genASMCode(root->children.at(0));
		//$6 <- $3
		cout << "add $6, $3, $0" << endl;
		//$3 <- expr2
		genASMCode(root->children.at(2));

		//Determine whether to use a signed or unsigned comparison based on the types of the expr's
		if (root->childrenTypes.at(0) == "int")
		{
			//$3 <- $6 < $3
			cout << "slt $3, $6, $3" << endl;
		}
		else if (root->childrenTypes.at(0) == "int*")
		{
			//$3 <- $6 < $3
			cout << "sltu $3, $6, $3" << endl;
		}

		return;
	}
	else if (root->rule == "statement WHILE LPAREN test RPAREN LBRACE statements RBRACE")
	{
		//Print the label to mark the beginning of the loop
		string labelNum = convertIntToString(whileCount);
		cout << "loop" << labelNum << ":" << endl;

		//$3 <- test
		genASMCode(root->children.at(2));
		//Test the result of the condition in $3
		cout << "beq $3, $0, done" << labelNum << endl;
		//Increment label counter PRIOR to recursing (could have nested while loops)
		whileCount++;
		//Generate code for the statement
		genASMCode(root->children.at(5));
		//Branch back to the top of the loop to check the condition
		cout << "beq $0, $0, loop" << labelNum << endl;
		//Label to exit the loop
		cout << "done" << labelNum << ":" << endl;
		return;
	}
	else if (root->rule == "test expr NE expr")
	{
		//$3 <- expr1
		genASMCode(root->children.at(0));
		//$6 <- $3
		cout << "add $6, $3, $0" << endl;
		//$3 <- expr2
		genASMCode(root->children.at(2));

		//Determine whether to use a signed or unsigned comparison based on the types of the expr's
		if (root->childrenTypes.at(0) == "int")
		{
			//$7 <- $6 < $3
			cout << "slt $7, $6, $3" << endl;
			//$8 <- $3 < $6
			cout << "slt $8, $3, $6" << endl;
		}
		else if (root->childrenTypes.at(0) == "int*")
		{
			//$7 <- $6 < $3
			cout << "sltu $7, $6, $3" << endl;
			//$8 <- $3 < $6
			cout << "sltu $8, $3, $6" << endl;
		}

		//$3 <- $7 | $8
		cout << "add $3, $7, $8" << endl;
		return;
	}
	else if (root->rule == "test expr EQ expr")
	{
		//$3 <- expr1
		genASMCode(root->children.at(0));
		//$6 <- $3
		cout << "add $6, $3, $0" << endl;
		//$3 <- expr2
		genASMCode(root->children.at(2));


		//Determine whether to use a signed or unsigned comparison based on the types of the expr's
		if (root->childrenTypes.at(0) == "int")
		{
			//$7 <- $6 < $3
			cout << "slt $7, $6, $3" << endl;
			//$8 <- $3 < $6
			cout << "slt $8, $3, $6" << endl;
		}
		else if (root->childrenTypes.at(0) == "int*")
		{
			//$7 <- $6 < $3
			cout << "sltu $7, $6, $3" << endl;
			//$8 <- $3 < $6
			cout << "sltu $8, $3, $6" << endl;
		}

		//$3 <- $7 | $8
		cout << "add $3, $7, $8" << endl;
		//$3 <- 1 - ($7 | $8)
		cout << "sub $3, $11, $3" << endl;
		return;
	}
	else if (root->rule == "test expr GT expr")
	{
		//$3 <- expr1
		genASMCode(root->children.at(0));
		//$6 <- $3
		cout << "add $6, $3, $0" << endl;
		//$3 <- expr2
		genASMCode(root->children.at(2));


		//Determine whether to use a signed or unsigned comparison based on the types of the expr's
		if (root->childrenTypes.at(0) == "int")
		{
			//$3 <- $3 < $6
			cout << "slt $3, $3, $6" << endl;
		}
		else if (root->childrenTypes.at(0) == "int*")
		{
			//$3 <- $3 < $6
			cout << "sltu $3, $3, $6" << endl;
		}
		
		return;
	}
	else if (root->rule == "test expr LE expr")
	{
		//$3 <- expr1
		genASMCode(root->children.at(0));
		//$6 <- $3
		cout << "add $6, $3, $0" << endl;
		//$3 <- expr2
		genASMCode(root->children.at(2));


		//Determine whether to use a signed or unsigned comparison based on the types of the expr's
		if (root->childrenTypes.at(0) == "int")
		{
			//$3 <- $3 < $6
			cout << "slt $3, $3, $6" << endl;
		}
		else if (root->childrenTypes.at(0) == "int*")
		{
			//$3 <- $3 < $6
			cout << "sltu $3, $3, $6" << endl;
		}

		//Flip the bit (the above process is to determine expr1 > expr2)
		cout << "sub $3, $11, $3" << endl;
		return;
	}
	else if (root->rule == "test expr GE expr")
	{
		//$3 <- expr1
		genASMCode(root->children.at(0));
		//$6 <- $3
		cout << "add $6, $3, $0" << endl;
		//$3 <- expr2
		genASMCode(root->children.at(2));


		//Determine whether to use a signed or unsigned comparison based on the types of the expr's
		if (root->childrenTypes.at(0) == "int")
		{
			//$3 <- $6 < $3
			cout << "slt $3, $6, $3" << endl;
		}
		else if (root->childrenTypes.at(0) == "int*")
		{
			//$3 <- $6 < $3
			cout << "sltu $3, $6, $3" << endl;
		}

		//Flip the bit (the above process is to determine expr1 < expr2)
		cout << "sub $3, $11, $3" << endl;
		return;
	}
	else if (root->rule == "statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE")
	{
		//Increment the if label counter
		ifCount++;
		//Compute the label number
		string labelNum = convertIntToString(ifCount);
		//$3 <- test
		genASMCode(root->children.at(2));
		//Check the result of the test (in $3) and branch accordingly
		cout << "bne $3, $11, else" << labelNum << endl;
		//Print code for first statement
		genASMCode(root->children.at(5));
		cout << "beq $0, $0, endif" << labelNum << endl;
		cout << "else" << labelNum << ":" << endl;
		//Print code for second statement
		genASMCode(root->children.at(9));
		cout << "endif" << labelNum << ":" << endl;
		return;
	}
	else if (root->rule == "factor STAR factor")
	{
		//$3 <- factor
		genASMCode(root->children.at(1));

		//Load the thing at the address in $3 into $3
		cout << "lw $3, 0($3)" << endl;

		return;
	}
	else if (root->rule == "factor NULL")
	{
		//$3 <- 1 (NULL)
		cout << "add $3, $0, $11" << endl;
		return;
	}
	else if (root->rule == "dcls dcls dcl BECOMES NULL SEMI")
	{
		genASMCode(root->children.at(0));

		//$3 <- 1 (NULL)
		cout << "add $3, $0, $11" << endl;
		
		//Store $3 at the appropriate offset relative to $29 based on ID
		string id = root->children.at(1)->children.at(1)->ruleTokens.at(1);
		string offset = convertIntToString(topSymTbl[currProcedure].second[id].second);
		cout << "sw $3, " << offset << "($29)" << endl; 
		cout << "sub $30, $30, $4" << endl; //Decrement stack pointer
		return;
	}
	else if (root->rule == "factor AMP lvalue")
	{
		//Reach into the tree to grab the prod. rule applied to lvalue
		string lvalueRule = root->children.at(1)->rule;

		tNode* temp = root->children.at(1);

		if (lvalueRule == "lvalue LPAREN lvalue RPAREN")
		{
			while (lvalueRule == "lvalue LPAREN lvalue RPAREN")
			{
				//Dive into the lvalue child + update the lvalue Rule
				lvalueRule = temp->children.at(1)->rule;
				//Update temp node
				temp = temp->children.at(1);
			}
		}


		if (lvalueRule == "lvalue ID")
		{
			string id = temp->children.at(0)->ruleTokens.at(1);
			string offset = convertIntToString(topSymTbl[currProcedure].second[id].second);
			cout << "lis $3" << endl;
			cout << ".word " << offset << endl;
			cout << "add $3, $3, $29" << endl;

		}
		else if (lvalueRule == "lvalue STAR factor")
		{
			genASMCode(temp->children.at(1));
		}

		return; 
		
	}
	else if (root->rule == "factor NEW INT LBRACK expr RBRACK")
	{
		//$3 <- expr
		genASMCode(root->children.at(3));

		//$1 <- $3
		cout << "add $1, $3, $0" << endl;

		//Call new
		alloc();

		//Check if allocation of mem was successful, if not set $3 to NULL
		cout << "bne $3, $0, 1" << endl;
		cout << "add $3, $11, $0" << endl;

	}
	else if (root->rule == "statement DELETE LBRACK RBRACK expr SEMI")
	{
		string labelNum = convertIntToString(deleteCount);
		deleteCount++;

		//$3 <- expr
		genASMCode(root->children.at(3));

		//Only call delete if $3 is not NULL
		cout << "beq $3, $11, skip" << labelNum << endl;
		cout << "add $1, $3, $0" << endl;
		dealloc();


		cout << "skip" << labelNum << ":" << endl;

	}
	else if (root->rule == "procedures procedure procedures")
	{
		genASMCode(root->children.at(1));
		genASMCode(root->children.at(0));
	}
	else if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
	{

		string procName = root->children.at(1)->ruleTokens.at(1);

		//Update currProcedure for context when working with ID's
		currProcedure = procName; 

		//Check to see if procedure has params and if so update it's offsets in symbol table
		string paramsRule = root->children.at(3)->rule;

		//Procedure has params
		if (paramsRule == "params paramlist")
		{
			updateOffsets(procName);
		}

		//Print label for procedure
		cout << "F" << procName << ":" << endl;

		//Set $29
		cout << "sub $29, $30, $4" << endl;
		//push(decls)
		genASMCode(root->children.at(6));
		//Push regs that are used onto stack
		cout << "sw $1, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;

		cout << "sw $2, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;

		cout << "sw $5, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;

		cout << "sw $6, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;

		cout << "sw $7, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;

		cout << "sw $8, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;

		//code(stmts)
		genASMCode(root->children.at(7));
		//code(expr) $3 <- expr
		genASMCode(root->children.at(9));

		//Pop back into regs
		cout << "lw $8, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;

		cout << "lw $7, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;

		cout << "lw $6, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;

		cout << "lw $5, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;

		cout << "lw $2, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;

		cout << "lw $1, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;

		//Restore stack ptr
		cout << "add $30, $29, $4" << endl;
		cout << "jr $31" << endl;
		

	}
	else if (root->rule == "factor ID LPAREN RPAREN")
	{
		string procName = root->children.at(0)->ruleTokens.at(1);

		//Push $29 onto stack
		cout << "sw $29, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;
		//Push $31 onto stack
		pushReg31();
		//Call the procedure
		cout << "lis $5" << endl;
		cout << ".word " << "F" << procName << endl;
		cout << "jalr $5" << endl;

		//Restore contents of $31 and $29
		popIntoReg31();
		cout << "lw $29, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;
	}
	else if (root->rule == "factor ID LPAREN arglist RPAREN")
	{
		string procName = root->children.at(0)->ruleTokens.at(1);

		//Push $29 onto stack
		cout << "sw $29, -4($30)" << endl;
		cout << "sub $30, $30, $4" << endl;
		//Push $31 onto stack
		pushReg31();
		//Push args onto stack
		genASMCode(root->children.at(2));

		//Call the procedure
		cout << "lis $5" << endl;
		cout << ".word " << "F" << procName << endl;
		cout << "jalr $5" << endl;

		//Pop all args off the stack
		//popVal = # of args * 4
		string popVal = convertIntToString(topSymTbl[procName].first.size() * 4);
		cout << "lis $5" << endl;
		cout << ".word " << popVal << endl;
		cout << "add $30, $30, $5" << endl;

		//Restore contents of $31 and $29
		popIntoReg31();
		cout << "lw $29, 0($30)" << endl;
		cout << "add $30, $30, $4" << endl;
	
	}
	else if (root->rule == "arglist expr COMMA arglist")
	{
		//$3 <- expr
		genASMCode(root->children.at(0));
		//Push expr onto stack
		pushReg3();
		//Process arglist
		genASMCode(root->children.at(2));
	}
	else if (root->rule == "arglist expr")
	{
		//$3 <- expr
		genASMCode(root->children.at(0));
		//Push expr onto stack
		pushReg3();
	}

}


int main (int argc, const char * argv[]) {

	string input;
	vector<tNode*> treeNodes;

	while (getline(cin,input))
	{
		tNode *newNode = new tNode();
		newNode->rule = input;
		tokenizeLine(input, newNode->ruleTokens);
		treeNodes.push_back(newNode);
	}


	//Call recursive function to build tree
 	buildTree(treeNodes, treeNodes.at(0));

 	//Build the symbol table
 	buildSymbolTable(treeNodes.at(0));

 	//Print the symbol table
 	//printSymbolTable();

 	//Check types
 	typeOf(treeNodes.at(0));

 	//Generate assembly code
 	genASMCode(treeNodes.at(0));

 	
 	return 0;
}

