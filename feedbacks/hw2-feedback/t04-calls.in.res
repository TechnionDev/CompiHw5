31: Type -> INT
5: RetType -> Type
31: Type -> INT
11: FormalDecl -> Type ID
31: Type -> INT
11: FormalDecl -> Type ID
9: FormalsList -> FormalDecl
10: FormalsList -> FormalDecl COMMA FormalsList
8: Formals -> FormalsList
36: Exp -> ID
36: Exp -> ID
35: Exp -> Exp BINOP Exp
21: Statement -> RETURN Exp SC
12: Statements -> Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
31: Type -> INT
5: RetType -> Type
32: Type -> BYTE
11: FormalDecl -> Type ID
31: Type -> INT
11: FormalDecl -> Type ID
9: FormalsList -> FormalDecl
10: FormalsList -> FormalDecl COMMA FormalsList
8: Formals -> FormalsList
36: Exp -> ID
36: Exp -> ID
35: Exp -> Exp BINOP Exp
21: Statement -> RETURN Exp SC
12: Statements -> Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
6: RetType ->  VOID
7: Formals -> epsilon
31: Type -> INT
38: Exp -> NUM
38: Exp -> NUM
29: ExpList -> Exp
30: ExpList -> Exp COMMA ExpList
27: Call -> ID LPAREN ExpList RPAREN
37: Exp -> Call
16: Statement -> Type ID ASSIGN Exp SC
12: Statements -> Statement
31: Type -> INT
39: Exp -> NUM B
38: Exp -> NUM
29: ExpList -> Exp
30: ExpList -> Exp COMMA ExpList
27: Call -> ID LPAREN ExpList RPAREN
37: Exp -> Call
16: Statement -> Type ID ASSIGN Exp SC
13: Statements -> Statements Statement
31: Type -> INT
39: Exp -> NUM B
38: Exp -> NUM
29: ExpList -> Exp
30: ExpList -> Exp COMMA ExpList
27: Call -> ID LPAREN ExpList RPAREN
37: Exp -> Call
16: Statement -> Type ID ASSIGN Exp SC
13: Statements -> Statements Statement
31: Type -> INT
38: Exp -> NUM
38: Exp -> NUM
29: ExpList -> Exp
30: ExpList -> Exp COMMA ExpList
27: Call -> ID LPAREN ExpList RPAREN
37: Exp -> Call
16: Statement -> Type ID ASSIGN Exp SC
13: Statements -> Statements Statement
20: Statement -> RETURN SC
13: Statements -> Statements Statement
4: FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
2: Funcs -> epsilon
3: Funcs -> FuncDecl Funcs
3: Funcs -> FuncDecl Funcs
3: Funcs -> FuncDecl Funcs
1: Program -> Funcs
