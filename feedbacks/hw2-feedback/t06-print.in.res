6: RetType ->  VOID
7: Formals -> epsilon
31: Type -> INT
38: Exp -> NUM
16: Statement -> Type ID ASSIGN Exp SC
12: Statements -> Statement
39: Exp -> NUM B
29: ExpList -> Exp
27: Call -> ID LPAREN ExpList RPAREN
19: Statement -> Call SC
13: Statements -> Statements Statement
40: Exp -> STRING
29: ExpList -> Exp
27: Call -> ID LPAREN ExpList RPAREN
19: Statement -> Call SC
13: Statements -> Statements Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
6: RetType ->  VOID
7: Formals -> epsilon
28: Call -> ID LPAREN RPAREN
19: Statement -> Call SC
12: Statements -> Statement
38: Exp -> NUM
29: ExpList -> Exp
27: Call -> ID LPAREN ExpList RPAREN
19: Statement -> Call SC
13: Statements -> Statements Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
2: Funcs -> epsilon
3: Funcs -> FuncDecl Funcs
3: Funcs -> FuncDecl Funcs
1: Program -> Funcs
