

#include <cstdlib>
#include <queue>
#include <vector>
#include <stack>
#include "node.h"
#include "reader.h"
#include <cassert>
#include <algorithm>
#include <map>
// #include "error.h"


#define ALPHA 	1
#define DIGIT 	3
#define LBRACK	4 //[]
#define RBRACK	5 //[]
#define LBRACE	6 //{}
#define RBRACE	7 //{}
#define LPAREN	8 //()
#define RPAREN	9 //()
#define OR		10 //|
#define STAR	11 //*
#define DOT		12 //.
#define GREEDY	13 //?
#define BACKLASH 14 
#define PLUS	15

using namespace std;

/*
A Regex praser, supporting tokens include () [] + * | and 0-9 a-z A-Z
*/

class Regex
{
public:
	Regex(){};
	Regex(string regex);
	~Regex();

	void compile(string regex);
	bool isMatch(string str);
	void print();
private:
	int getType(char c);
	Node* constructTree	(Reader &reader);
	Node* handleParen	(char c, Reader&);
	Node* handleBrace	(Node*, Reader&);

	Node* mergeNode		(Node*, Node*);//merge right and left then return the parent
	void mergeNode		(Node*, Node*, Node*);//merge right and left to the first node

	Node* handleChar	(char c, Reader&, stack<Node*>&);	
	Node* handleBrack	(char c, Reader&, stack<Node*>&);	
	Node* handleRepeat	(char c, Reader&, stack<Node*>&);
	Node* handleDot		(char c, Reader&, stack<Node*>&);
	Node* handleOr		(char c, Reader&, stack<Node*>&);

	void getFollowPos(Node*);

	set<Node*> getUnion(set<Node*>&,set<Node*>, set<Node*>);


private:
	stack<char> _operator_stack;
	Node* _root;
	map<int, Node*> _symbol;
};

Regex::Regex(string str){

	compile(str);
}
Regex::~Regex(){
}


//not finished
void Regex::print(){
	queue<Node*> queue;
	queue.push(_root);
	stringstream ss_type;
	stringstream ss_child;
	
	while (!queue.empty()){
		int n = queue.size();
		for (int i = 0; i < n; i++){
			Node *node = queue.front();
			queue.pop();

			node->printNode(ss_type,ss_child); 
			
			if (!node->isLeftChildNull())
				queue.push(node->getLeft());

			if (!node->isRightChildNull())
				queue.push(node->getRight());
		}
		cout << ss_type.str() << endl;
		cout << ss_child.str() << endl;
		ss_type.str("");
		ss_child.str("");
		cout << endl;
	}
	
}

void Regex::compile(string str){
	_root = new Node;
	Reader reader;
	reader.init(str + "\0");
	
	Node* node_end = new Node;
	node_end->setType(NodeType::nEnd);
	node_end->addValue('#');
	node_end->setParent(_root);
	node_end->first_pos_.insert(node_end);
	node_end->last_pos_.insert(node_end);

	_root->setType(NodeType::nCAT);
	Node* node = constructTree(reader);
	node->setParent(_root);
	_root->addLeft(node);
	_root->addRight(node_end);
	_root->first_pos_ = _root->getLeft()->first_pos_; 
	_root->last_pos_ = _root->getRight()->last_pos_;
	/******Tree finished******/
	getFollowPos(_root);
	
	return;
}

Node* Regex::constructTree(Reader &reader){
	stack<Node*> _node_stack;
	while (reader.current() != '\0'){
		
		char tmp_char = reader.read();
		Node *tmp_node;
		
		switch (getType(tmp_char)){
			case ALPHA:case DIGIT:
				tmp_node = handleChar(tmp_char, reader, _node_stack);
				break;
			case LPAREN:
				_operator_stack.push('(');
				tmp_node = constructTree(reader);
				_node_stack.push(tmp_node);
				break;
			case RPAREN:
				if (_operator_stack.top() != '(')
					cout << "illegal operator:" << _operator_stack.top() << " expect ')'" << endl;
				assert(_operator_stack.top() == '(');
				_operator_stack.pop();
				tmp_node = _node_stack.top(); _node_stack.pop();
				return tmp_node;
				break;
			case LBRACK:
				tmp_node = handleBrack(tmp_char, reader, _node_stack);
			case STAR:case PLUS:
				tmp_node = handleRepeat(tmp_char, reader, _node_stack);
				break;
			case OR:
				tmp_node = handleOr(tmp_char, reader, _node_stack);
				break;
			case BACKLASH:
				tmp_node = handleChar(reader.read(), reader, _node_stack);
				break;
			case DOT://not supported
				tmp_node = handleDot(reader.read(), reader, _node_stack);
				break;
			default:
				cout << "illegal char" << endl;// error::msg("illegal char");
				assert(0);
				return nullptr;
		}
		if (_node_stack.size() == 2){
			_node_stack.pop();

			Node *node = new Node;
			node->setType(NodeType::nCAT);
			
			Node *left = _node_stack.top(); _node_stack.pop();
			node->addLeft(left); 
			node->addRight(tmp_node);
			tmp_node->setParent(node);
			left->setParent(node);

			node->first_pos_ = left->first_pos_;
			node->last_pos_ = tmp_node->last_pos_;
			if (left->isNullable()){				
				node->first_pos_.insert(tmp_node);
			}
			_node_stack.push(node);
		}

	}
	Node *node = _node_stack.top(); _node_stack.pop();
	return node;

}

int Regex::getType(char c){
	if (isalpha(c)){
		return ALPHA;
	}
	else if (isdigit(c)){
		return DIGIT;
	}
	else{
		switch (c){
		case '(':
			return LPAREN;
			break;
		case ')':
			return RPAREN;
			break;
		case '[':
			return LBRACK;
			break;
		case ']':
			return RBRACK;
			break;
		case '{':
			return LBRACE;
			break;
		case '}':
			return RBRACE;
			break;
		case '*':
			return STAR;
			break;
		case '|':
			return OR;
			break;
		case '.':
			return DOT;
			break;
		case '+':
			return PLUS;
			break;
		default:
			return -1;
		}
	}

}

Node* Regex::handleChar(char c, Reader &reader, stack<Node*>& _node_stack){
	Node *node = new Node;
	node->addValue(c);
	node->first_pos_.insert(node);
	node->last_pos_.insert(node);
	_symbol.insert(pair<int, Node*>::pair(_symbol.size() + 1, node));
	
	if (reader.current() == '+'){
		reader.read();
		Node *tmp_node = new Node;
		tmp_node->setType(NodeType::nREPEAT);
		tmp_node->setRange(1, -1, true);
		tmp_node->addLeft(node);
		tmp_node->first_pos_ = node->first_pos_;
		tmp_node->last_pos_ = node->last_pos_;
		_node_stack.push(tmp_node);
		return tmp_node;
	}
	else if (reader.current() == '*'){
		reader.read();
		Node *tmp_node = new Node;
		tmp_node->setType(NodeType::nREPEAT);
		tmp_node->setRange(0, -1, true);
		tmp_node->addLeft(node);
		tmp_node->first_pos_ = node->first_pos_;
		tmp_node->last_pos_ = node->last_pos_;
		_node_stack.push(tmp_node);
		return tmp_node;
	}
	else if (reader.current() == '{'){
		reader.moveToNext();
		node->first_pos_ = node->first_pos_;
		node->last_pos_ = node->last_pos_;

		Node *tmp_node = handleBrace(node,reader);
		
		_node_stack.push(tmp_node);
		return tmp_node;
	}
	else{
		_node_stack.push(node);
		return node;
	}



	
}

// not used
Node* Regex::handleParen(char c, Reader &reader){
	stack<Node*> _node_stack;
	while (reader.current() != ')'){
		char tmp_char = reader.read();
		Node *tmp_node;

		switch (getType(tmp_char)){
		case ALPHA:case DIGIT:
			tmp_node = handleChar(tmp_char, reader, _node_stack);
			break;
		case LPAREN:
			tmp_node = handleParen(tmp_char, reader);
			break;
		case LBRACK:
			tmp_node = handleBrack(tmp_char, reader, _node_stack);
		case STAR:case PLUS:
			tmp_node = handleRepeat(tmp_char, reader, _node_stack);
			break;
		case OR:
			tmp_node = handleOr(tmp_char, reader, _node_stack);
			break;
		case BACKLASH:
			tmp_node = handleChar(reader.read(), reader, _node_stack);
		case DOT://not supported
			tmp_node = handleDot(reader.read(), reader, _node_stack);
		default:
			cout << "illegal char" << endl;// error::msg("illegal char");
			return nullptr;
		}
		if (_node_stack.size() == 2){
			_node_stack.pop();

			Node *node = new Node;
			node->setType(NodeType::nCAT);

			node->addLeft(_node_stack.top()); _node_stack.pop();
			node->addRight(tmp_node);
			_node_stack.push(node);
		}
	}
	
	Node *node = _node_stack.top(); _node_stack.pop();
	return node;

}

Node* Regex::handleBrack(char c, Reader &reader, stack<Node*>& _node_stack){
	Node *node = new Node;
	while (reader.current() != ']')
	{
		if (reader.next() == '-')
		{
			char start = reader.read();
			reader.moveToNext();
			char end = reader.read();
			if (start > end){
				cout << "error:'" << start << "' expect to be binaryly bigger than '" << end << "'" << endl;
			}
			assert(start <= end);
			while (start <= end){
				node->addValue(start++);
			}			

		}
		else if (reader.current() == '\\')
		{
			reader.moveToNext();
			node->addValue(reader.read());
		}
		
	}
	reader.moveToNext();
	_node_stack.push(node);
	return node;

}

Node* Regex::handleBrace(Node *child, Reader &reader){
	Node *tmp_node = new Node;
	tmp_node->setType(NodeType::nREPEAT);
	tmp_node->addLeft(child);
	child->setParent(tmp_node);

	tmp_node->first_pos_ = child->first_pos_;
	tmp_node->last_pos_ = child->last_pos_;

	if (reader.next() == ',')
	{
		char start = reader.read();
		if (!isdigit(start))
			cout << "error: '" << start << "' expected to be a digit" << endl;
		assert(isdigit(start));
		reader.moveToNext();//skip the ','
		char end = reader.read();
		if (!isdigit(end))
			cout << "error: '" << end << "' expected to be a digit" << endl;
		assert(isdigit(end));
		if (start > end){
			cout << "error:'" << start << "' expect to be bigger than '" << end << "'" << endl;
		}
		assert(start <= end);
			
		tmp_node->setRange(start - '0', end - '0', false);

	}
	else if (isdigit(reader.current()) && reader.next() == '}' )
	{
		tmp_node->setRange(reader.current() - '0', reader.current() - '0', false);
	}
	else{
		cout << "error: Synax error on '{'" << endl;
		assert(0);
	}
	reader.read();
	return tmp_node;
}

Node* Regex::handleRepeat(char c, Reader &reader, stack<Node*>& _node_stack){
	Node *node = new Node;
	node->setType(NodeType::nREPEAT);
	if (c == '+')
		node->setRange(1, -1, true);
	else if (c == '*')
		node->setRange(0, -1, true);
	else {
		cout << "error:ilegal char in handleRepeat"<<endl;
	}
	Node* tmp = _node_stack.top();
	node->first_pos_ = tmp->first_pos_;
	node->last_pos_ = tmp->last_pos_;
	tmp->setParent(node);
	node->addLeft(tmp); _node_stack.pop();
	_node_stack.push(node);
	return node;
}

Node* Regex::handleDot(char c, Reader &reader, stack<Node*>& _node_stack){ return nullptr; }


set<Node*> Regex::getUnion(set<Node*> &target,set<Node*> a, set<Node*> b){
	set<Node*>::iterator i_a=a.begin(), i_b=b.begin();
	while (i_a != a.end()){
		target.insert(*i_a);
		i_a++;
	}
	while (i_b != b.end()){
		target.insert(*i_b);
		i_b++;
	}
	return target;
}

Node* Regex::handleOr(char c, Reader &reader, stack<Node*>& _node_stack){
	Node* node = constructTree(reader);
	Node* tmp_node = _node_stack.top();
	_node_stack.pop();
	Node* tmp_node2 = new Node;
	tmp_node2->setType(NodeType::nOR);
	tmp_node2->addLeft(tmp_node);
	tmp_node2->addRight(node);

	tmp_node->setParent(tmp_node2);
	node->setParent(tmp_node2);

	getUnion(tmp_node2->first_pos_, tmp_node2->getLeft()->first_pos_, tmp_node2->getRight()->first_pos_);
	getUnion(tmp_node2->last_pos_, tmp_node2->getLeft()->last_pos_, tmp_node2->getRight()->last_pos_);


	_node_stack.push(tmp_node2);
	return tmp_node2;
}



void Regex::getFollowPos(Node* root){
	if (!root->isLeftChildNull())
		getFollowPos(root->getLeft());
	if (!root->isRightChildNull())
		getFollowPos(root->getRight());
	if (root->getNodeType() == NodeType::nCAT){
		set<Node*> left = root->getLeft()->last_pos_;
		set<Node*> right = root->getRight()->first_pos_;
		
		for (set<Node*>::iterator i = left.begin(); i != left.end(); i++){
			for (set<Node*>::iterator j = right.begin(); j != right.end(); j++){
				//to do
				(*i)->follow_pos_.insert((*j));
			}
		}
	}
	else if (root->getNodeType() == NodeType::nREPEAT){
		set<Node*> last = root->last_pos_;
		set<Node*> first = root->first_pos_;
		for (auto i = last.begin(); i != last.end(); i++){
			for (auto j = first.begin(); j != first.end(); j++){
				(*i)->follow_pos_.insert((*j));
			}
		}
	}
	return;
}

