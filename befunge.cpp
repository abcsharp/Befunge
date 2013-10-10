#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <stack>
#include <random>
#include <chrono>
#include <algorithm>
#include <Windows.h>

class Program
{
private:
	template<typename T> class Stack
	{
	private:
		std::stack<T> stack;
		
	public:
		void Push(const T& value)
		{
			stack.push(value);
		}

		T&& Pop(void)
		{
			if(stack.empty()) return std::move(T());
			T& value=stack.top();
			stack.pop();
			return std::move(value);
		}

		T Peek(void)
		{
			return stack.empty()?T():stack.top();
		}
	};

	enum Direction
	{
		Up=1,Down=2,Left=3,Right=4
	};
	
	std::array<std::array<char,25>,80> source;
	Stack<int> stack;
	std::mt19937 engine;
	std::uniform_int_distribution<int> distribution;
	int x,y,xMax,yMax;
	Direction direction;
	bool quote,enableVisual;
	void* stdHandle;
	COORD position;

	void Move(void)
	{
		if(direction==Direction::Up) y=y-1<0?yMax-1:y-1;
		else if(direction==Direction::Down) y=y+1>=yMax?0:y+1;
		else if(direction==Direction::Left) x=x-1<0?xMax-1:x-1;
		else if(direction==Direction::Right) x=x+1>=xMax?0:x+1;		
	}

public:
	Program(std::ifstream& file,bool enableVisual)
	{
		x=y=0;
		quote=false;
		this->enableVisual=enableVisual;
		xMax=source.size();
		yMax=source[0].size();
		direction=Direction::Right;
		distribution=std::uniform_int_distribution<int>(1,4);
		auto now=std::chrono::high_resolution_clock::now().time_since_epoch();
		engine.seed(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
		std::string buffer;
		for(int y=0;y<yMax;++y){
			if(!std::getline(file,buffer)) buffer=std::string(80,' ');
			for(int x=0;x<xMax;++x) source[x][y]=x<buffer.length()?buffer[x]:' ';
		}
		if(enableVisual){
			stdHandle=GetStdHandle(STD_OUTPUT_HANDLE);
			int bufferSize=80*25;
			DWORD writtenSize;
			position.X=position.Y=0;
			FillConsoleOutputCharacter(stdHandle,L' ',bufferSize,position,&writtenSize);
			SetConsoleCursorPosition(stdHandle,position);
			for(int y=0;y<yMax;++y){
				for(int x=0;x<xMax;++x) std::cout<<source[x][y];
				std::cout<<std::endl;
			}
		}
	}

	bool Execute(void)
	{
		auto c=source[x][y];
		if(enableVisual){
			DWORD writtenSize=0;
			if(position.X!=x||position.Y!=y)
				FillConsoleOutputAttribute(stdHandle,FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE,1,position,&writtenSize);
			position.X=x;
			position.Y=y;
			FillConsoleOutputAttribute(stdHandle,BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE,1,position,&writtenSize);
		}
		if(c=='\"') quote=!quote;
		else if(quote) stack.Push(c);
		else{
			if(c=='<') direction=Direction::Left;
			else if(c=='>') direction=Direction::Right;
			else if(c=='^') direction=Direction::Up;
			else if(c=='v') direction=Direction::Down;
			else if(c=='?') direction=(Direction)distribution(engine);
			else if(c=='#') Move();
			else if(c=='@') return false;
			else if(c>='0'&&c<='9') stack.Push(c-'0');
			else if(c=='$') stack.Pop();
			else if(c==':') stack.Push(stack.Peek());
			else if(c=='_') direction=stack.Pop()?Direction::Left:Direction::Right;
			else if(c=='|') direction=stack.Pop()?Direction::Up:Direction::Down;
			else if(c=='.') std::cout<<stack.Pop()<<" ";
			else if(c==',') std::cout<<(char)stack.Pop();
			else if(c=='!') stack.Push(!stack.Pop());
			else if(c=='&'){
				int number=0;
				std::cin>>number;
				stack.Push(number);
			}else if(c=='~'){
				char ch=std::cin.get();
				while(std::cin.get()!='\n');
				stack.Push(ch);
			}else if(c=='+'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(a+b);
			}else if(c=='-'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(a-b);
			}else if(c=='*'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(a*b);
			}else if(c=='/'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(a/b);
			}else if(c=='%'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(a%b);
			}else if(c=='`'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(a>b);
			}else if(c=='\\'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(b);
				stack.Push(a);
			}else if(c=='g'){
				int b=stack.Pop();
				int a=stack.Pop();
				stack.Push(source[a][b]);
			}else if(c=='p'){
				int b=stack.Pop();
				int a=stack.Pop();
				int c=stack.Pop();
				source[a][b]=c;
				if(enableVisual){
					COORD position={(SHORT)a,(SHORT)b};
					FillConsoleOutputCharacter(stdHandle,c,1,position,nullptr);
				}
			}
		}
		Move();
		return true;
	}
};

int main(int argc,char** argv)
{
	if(argc<2){
		std::cout<<"Usage: befunge [ -visual ] [ -step time ] sourcefile.bf"<<std::endl;
		return 0;
	}
	bool enableVisual=false;
	int interval=500;
	std::string fileName;
	for(int i=1;i<argc;++i){
		std::string arg=argv[i];
		std::transform(arg.begin(),arg.end(),arg.begin(),[](char c){return (char)std::tolower(c);});
		if(arg=="-visual") enableVisual=true;
		else if(arg=="-step"){
			if(++i==argc){
				std::cout<<"Missing time after \"-step\" option."<<std::endl;
				return 1;
			}
			interval=std::atoi(argv[i]);
			if(interval<20){
				std::cout<<"Cannot set step interval to less than 20."<<std::endl;
				return 1;
			}
		}else if(arg.substr(arg.size()-3)==".bf") fileName=arg;
		else{
			std::cout<<"\""<<arg<<"\" is not befunge source file."<<std::endl;
			return 1;
		}
	}
	if(fileName.empty()){
		std::cout<<"No input file."<<std::endl;
		return 0;
	}
	std::ifstream source(fileName,std::ios::in);
	if(!source){
		std::cout<<"Cannot open \""<<fileName<<"\"."<<std::endl;
		return 1;
	}
	Program program(source,enableVisual);
	for(;program.Execute();) if(enableVisual) Sleep(interval);
	return 0;
}
