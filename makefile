.PHONY: all clean

all: clean
	flex scanner.lex
	/opt/homebrew/opt/bison/bin/bison -Wcounterexamples -d parser.ypp
	g++ -g -std=c++17 -o hw5 *.c *.cpp
clean:
	rm -f lex.yy.c parser.tab.*pp hw5 amiti_gurt_hw5.zip

zip:
	zip amiti_gurt_hw5.zip parser.ypp scanner.lex \
							stypes.*pp \
							tokens.*pp \
							symbolTable.*pp \
							bp.*pp \
							hw3_output.*pp
