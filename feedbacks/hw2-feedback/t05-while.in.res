31: Type -> INT
5: RetType -> Type
7: Formals -> epsilon
38: Exp -> NUM
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
31: Type -> INT
38: Exp -> NUM
16: Statement -> Type ID ASSIGN Exp SC
12: Statements -> Statement
36: Exp -> ID
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
25: Statement -> BREAK SC
22: Statement -> IF LPAREN Exp RPAREN Statement
13: Statements -> Statements Statement
31: Type -> INT
38: Exp -> NUM
16: Statement -> Type ID ASSIGN Exp SC
13: Statements -> Statements Statement
36: Exp -> ID
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
26: Statement -> CONTINUE SC
22: Statement -> IF LPAREN Exp RPAREN Statement
13: Statements -> Statements Statement
36: Exp -> ID
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
25: Statement -> BREAK SC
12: Statements -> Statement
14: Statement -> LBRACE Statements RBRACE
24: Statement -> WHILE LPAREN Exp RPAREN Statement
13: Statements -> Statements Statement
25: Statement -> BREAK SC
13: Statements -> Statements Statement
14: Statement -> LBRACE Statements RBRACE
24: Statement -> WHILE LPAREN Exp RPAREN Statement
12: Statements -> Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
31: Type -> INT
5: RetType -> Type
7: Formals -> epsilon
38: Exp -> NUM
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
31: Type -> INT
38: Exp -> NUM
16: Statement -> Type ID ASSIGN Exp SC
12: Statements -> Statement
36: Exp -> ID
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
25: Statement -> BREAK SC
22: Statement -> IF LPAREN Exp RPAREN Statement
13: Statements -> Statements Statement
31: Type -> INT
38: Exp -> NUM
16: Statement -> Type ID ASSIGN Exp SC
13: Statements -> Statements Statement
36: Exp -> ID
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
26: Statement -> CONTINUE SC
22: Statement -> IF LPAREN Exp RPAREN Statement
13: Statements -> Statements Statement
36: Exp -> ID
38: Exp -> NUM
46: Exp -> Exp RELOP Exp
25: Statement -> BREAK SC
12: Statements -> Statement
14: Statement -> LBRACE Statements RBRACE
24: Statement -> WHILE LPAREN Exp RPAREN Statement
13: Statements -> Statements Statement
14: Statement -> LBRACE Statements RBRACE
24: Statement -> WHILE LPAREN Exp RPAREN Statement
12: Statements -> Statement
25: Statement -> BREAK SC
13: Statements -> Statements Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
6: RetType ->  VOID
7: Formals -> epsilon
31: Type -> INT
28: Call -> ID LPAREN RPAREN
37: Exp -> Call
16: Statement -> Type ID ASSIGN Exp SC
12: Statements -> Statement
31: Type -> INT
28: Call -> ID LPAREN RPAREN
37: Exp -> Call
16: Statement -> Type ID ASSIGN Exp SC
13: Statements -> Statements Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
2: Funcs -> epsilon
3: Funcs -> FuncDecl Funcs
3: Funcs -> FuncDecl Funcs
3: Funcs -> FuncDecl Funcs
1: Program -> Funcs
