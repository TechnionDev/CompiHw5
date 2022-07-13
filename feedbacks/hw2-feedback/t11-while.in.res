6: RetType ->  VOID
7: Formals -> epsilon
38: Exp -> NUM
38: Exp -> NUM
29: ExpList -> Exp
27: Call -> ID LPAREN ExpList RPAREN
19: Statement -> Call SC
12: Statements -> Statement
14: Statement -> LBRACE Statements RBRACE
24: Statement -> WHILE LPAREN Exp RPAREN Statement
12: Statements -> Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
2: Funcs -> epsilon
3: Funcs -> FuncDecl Funcs
1: Program -> Funcs
