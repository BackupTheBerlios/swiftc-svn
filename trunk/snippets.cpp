bool AssignStatement::analyze()
{
    // check params
    bool result = exprList_->analyze();

    // check result tupel
    result &= tupel_->analyze();

    if (!result)
        return false;

    TypeList in = exprList_->getTypeList();
    TypeList out = tupel_->getTypeList();

    // check whether there is a const type in out
    for (const Tupel* iter = tupel_; iter != 0; iter = iter->next())
    {
        const TypeNode* typeNode = iter->typeNode();

        const Expr* expr = dynamic_cast<const Expr*>(typeNode);
        if ( expr )
        {
            if ( expr->getType()->isReadOnly() )
            {
                const Id* id = dynamic_cast<const Id*>(expr);

                if (id)
                {
                    errorf(line_, "assignment of read-only variable '%s'", 
                            id->id_->c_str() );
                    return false;
                }
                else
                {
                    errorf(line_, "assignment of read-only location '%s'", 
                            expr->toString().c_str() );
                    return false;
                }
            }   
        }
    }

    swiftAssert(  in.size() > 0, "must have at least one element" );
    swiftAssert( out.size() > 0, "must have at least one element" );

    if ( in.size() > 1 && out.size() > 1 )
    {
        errorf(line_, "either the left-hand side or the right-hand side of an "
                "assignment statement must have exactly one element");

        return false;
    }

    if (out.size() == 1)
    {
        bool isDecl = dynamic_cast<const Decl*>(tupel_->typeNode());

        if ( out[0]->isNonAtomicBuiltin() )
            return out[0]->hasAssignCreate(in, isDecl, line_);

        swiftAssert( typeid(*out[0]) == typeid(BaseType), "TODO" );

        const BaseType* bt = (const BaseType*) out[0];
        Class* _class = bt->lookupClass();
        Method* assignCreate = symtab->lookupAssignCreate(_class, in, isDecl, line_);

        if (!assignCreate)
            return false;

        genAssignCall(_class, assignCreate);
    }
    else
    {
        if ( !analyzeFunctionCall(in, out) )
            return false;
    }

    if (!result)
        return false;

    genSSA();

    // TODO
    //if (typeid(*expr_) == typeid(MemberAccess) )
        //delete ((MemberAccess*) expr_)->rootStructOffset_;

    return true;
}

bool AssignStatement::analyzeFunctionCall(const TypeList& in, const TypeList& out)
{
    // -> it is an ordinary call of a function
    FunctionCall* fc = exprList_->getFunctionCall();
    if (!fc)
    {
        errorf(line_, "the right-hand side of an assignment statement with"
                "more than one item on the left-hand side" 
                "must be a function call");

        return false;
    }

    Method* method = fc->getMethod();

    if ( !method->sig_->checkOut(out) )
    {
        errorf( line_, "right-hand side needs out types '%s' but '%s' are given",
                out.toString().c_str(), 
                method->sig_->getOut().toString().c_str() ); 

        return false;
    }

    /*
     * check whether copy constructors or assign operators are available
     * for types on the lhs respectively
     */

    for (const Tupel* iter = tupel_; iter != 0; iter = iter->next())
    {
        const Decl* decl = dynamic_cast<const Decl*>( iter->typeNode() );

        if (decl)
        {
            const BaseType* bt = dynamic_cast<const BaseType*>( decl->getType() );

            if (bt)
            {
                Class* _class = bt->lookupClass();

                if (_class->copyCreate_ == Class::COPY_NONE)
                {
                    errorf( line_, 
                            "class '%s' does not provide a copy constructor 'create(%s)'",
                            _class->id_->c_str(),
                            _class->id_->c_str() );

                    return false;
                }
            }
        }
        else
        {
            swiftAssert( dynamic_cast<const Expr*>(iter->typeNode()),
                    "must be an Expr here" );
            const Expr* expr = (const Expr*) iter->typeNode();
            const BaseType* bt = dynamic_cast<const BaseType*>( expr->getType() );

            if (bt)
            {
                Class* _class = bt->lookupClass();
                TypeList in;
                in.push_back(bt);
                Method* assign = symtab->lookupAssign(_class, in, line_);

                if (!assign)
                    return false;
            }

        }
    }

    return true;
}

void AssignStatement::genConstructorCall(Class* _class, Method* /*method*/)
{
    PlaceList lhsPlaces = tupel_->getPlaceList();
    PlaceList rhsPlaces = exprList_->getPlaceList();

    if ( !_class || BaseType::isBuiltin(_class->id_) )
    {
        me::functab->appendInstr( 
                new me::AssignInstr(kind_ , (me::Reg*) lhsPlaces[0], rhsPlaces[0]) );
        //tupel_->genStores();
        return;
    }

    // else TODO
    swiftAssert(false, "TODO");
}

void AssignStatement::genAssignCall(Class* _class, Method* /*method*/)
{
    PlaceList lhsPlaces = tupel_->getPlaceList();
    PlaceList rhsPlaces = exprList_->getPlaceList();

    if ( !_class || BaseType::isBuiltin(_class->id_) )
    {
        me::functab->appendInstr( 
                new me::AssignInstr(kind_ , (me::Reg*) lhsPlaces[0], rhsPlaces[0]) );
        //tupel_->genStores();
        return;
    }

    // else TODO
    //swiftAssert(false, "TODO");
}

void AssignStatement::genPtrAssignCreate()
{
}

void AssignStatement::genSSA()
{
    /*
     * decl = expr1, expr2, ...
     *  - eval expr1
     *  - eval expr2
     *  - ...
     *  - create call with new var of decl
     *
     * expr0 = expr1, expr2, ...
     *  - eval expr1
     *  - eval expr2
     *  - ...
     *  - eval expr0
     *  - assign call with place of expr0
     *
     * decl_or_expr1, decl_or_expr2, ... = functioncall 
     *  - eval functioncall
     *  - eval expr1
     *  - eval expr0
     *  - ...
     *  - functioncall
     *  - results are copyied  to lhs expr via copy assign
     *  - results are created with lhs decls via copy constructor
     * 
     */ 
    //TypeList rhsPlaces = exprList_->getPlaceList();

    //for (Tupel* iter = tupel_; iter != 0; iter = iter->next())
    //{
        //Expr* expr = dynamic_cast<Expr*>( iter->typeNode() );
        //if (!expr)
            //continue;

        //MemberAccess* ma = dynamic_cast<MemberAccess*>(expr);
        //if (!ma)
            //continue;

        //swiftAssert( dynamic_cast<const me::Var*>(expr->getPlace()),
                //"expr->place must be a me::Reg*" );

        //me::Store* store = new me::Store( 
                //(me::Var*) ma->getPlace(),          // memory variable
                //(me::Var*) exprList_->expr_->getPlace(),// argument 
                //ma->rootStructOffset_);             // offset 
        //me::functab->appendInstr(store);

        ////else
        ////{
            ////me::functab->appendInstr( 
                    ////new me::AssignInstr(kind_ , (me::Reg*) expr_->place_, exprList_->expr_->place_) );
        ////}
    //}
}


