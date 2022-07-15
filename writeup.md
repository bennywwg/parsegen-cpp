# Implementing a Parser Frontend for the 21st Century

In the context of parsers, programs which scan strings of text and perform actions based on the content of that text, a *Grammar* is defined by a set of production rules. This grammar then defines a *language*.  Grammar Production Rules take the following form:

`T`<sub>`Product`</sub> `|=` `T`<sub>`2`</sub> `T`<sub>`3`</sub> `...`  `T`<sub>`n`</sub>

When parsing text, the text is broken up into a string of *nonterminals*, denoted here as `T`<sub>`n`</sub>. The parser proceeds along the string, and when it encounters the sequence of nonterminals on the right hand side, it may convert that sequence into the left hand side.

With that in mind, I will turn to `parsegen-cpp`: This is a great little parser project that does exactly what I wrote above. It is a C++ library used to define grammars using production rules.  Now before I enter criticism mode, I want to give credit where it is due: the style of `parsegen-cpp` is extremely unopinioated, and it isn't designed *poorly*. Every part of it was eminently understandable and every aspect felt intuitive. For example, defining a parsegen language could not be simpler:

```
struct language {
  struct token {
    string name;
    string regex;
  };
  vector<token> tokens;
  struct production {
    string lhs;
    vector<string> rhs;
  };
  vector<production> productions;
};
```

So defining a grammar could look something like this:

```
language lang;
lang.tokens = {
    token { "A_Token", "A" }
};
lang.productions = {
    production { "Root_Nonterminal", { "A_List" } },
    production { "A_List", { "A_Token" } },
    production { "A_List", { "A_List", "A_Token" } }
};
```

This language would accepts any nonzero number of A's in a string. Seems pretty good, right? And for simply making a program that will either accept or reject a given input string, its probably nearly impossible to improve on this. Unfortunately, accepting and rejecting a string isn't the only task of a parser. To do anything interesting, the parser needs to invoke behavior when a production rule occurs. And here, in my opinion, is where `parsegen-cpp` falls apart from a usability and api design perspective.

What if we want to count the number of A's by incrementing a global variable any time either of "A_List" production rules is invoked? `parsegen-cpp` offers a virtual function that can be overridden to do just this, with the following signature:
  inline virtual any reduce(int prod, vector<any>& rhs)

This function is called every time a rule is invoked. Here is where the first major usability problem occurs, in my opinion: the function parameter `prod` is an index into lang.productions, which represents which production rule is occurring. Oh boy, where do I even begin. Well, this technically does offer all the information needed to accomplish our task. We could simple override the function to be:

```
inline virtual any reduce(int prod, vector<any>& rhs) override {
  if (prod >= 1 && prod <= 2) {
    global_A_count++;
  }
}
```

The usability problem with this is that if the productions table were ever to be changed, the indices of the rules would also change, which means any time the production rules change, we also need to change the overriden reduce function. This is simply unacceptable. The second major problem, which is much worse in my opinion, is that the use of `any` to transfer information between nonterminals means any time information is retrieved during a production rule, a programmer needs to mentally keep track of which types will be present in the any vector. This is also unacceptable. Clearly, the functionality exposed by `parsegen-cpp` is not suitable to being used directly to develop a parser, however as I will show below, it can be used as the foundation for a truly usable system that eliminates both of these problems.

How We Can Make Improvements

Imagine the following association between grammar concepts and C++ concepts:

    production rule <-> function
    nonterminal <-> type


A C++ function looks like this:

```
ReturnType FunctionName(Param0 p0, Param1 p1, ..., ParamN pN) { ... }
```

or

```
[](Param0 p0, Param1 p1, ..., ParamN pN) -> ReturnType { ... }
```

This looks eerily similar to a production rule:

```
struct production { string lhs; vector<string> rhs; };
```

What if there was a way to convert functions into productions, for example, the function:

```
[](Param0 p0, Param1 p1, ..., ParamN pN) -> ReturnType { ... }
```

would become:

```
production { "ReturnType", { "Param0", "Param1", ..., "ParamN" } }
```

But is this possible? Using C++ typeid operator, we can in fact get the string associated with a certain type. For example, typeid(int).name() will return the string "int". Or at least, it is guaranteed to return a unique string associated with that type. But that is not enough, in fact we need to create a vector<string> of the unique names of each the types of each parameter for the function. This can be accomplished using recursive variadic templates and SFINAE:

```
template<typename ...Args>
typename enable_if<sizeof...(Args) == 0, void>::type
RegTypes(vector<string>&) { };

template<typename R, typename ...Args>
typename enable_if<sizeof...(Args) == 0, void>::type
RegTypes(vector<string>& res) {
    res.back() = typeid(R).name();
};

template<typename R, typename ...Args>
typename enable_if<sizeof...(Args) != 0, void>::type
RegTypes(vector<string>& res) {
    RegTypes<Args...>(res);
    res[res.size() - sizeof...(Args) - 1] = typeid(R).name();
};
```

Here's an example of how this function in practice:

```
vector<string> names;
names.resize(4);
RegTypes<int, float, string, double>(names);
// names == { "int", "float", "string", "double" }
```

Now let me show you what this enables. Since we can convert a list of types into a string of their names, we have all the necessary ingredients to make a production rule! Here's an example function that could do that:

```
template<typename R, typename ...Args>
production MakeProduction(function<R(Args...)> func) {
  // Get the result typename
  string lhs = typeid(R).name();

  // Now, get the list of type names used to produce the result typename
  vector<strings> rhs;
  rhs.resize(sizeof...(Args));
  RegTypes<Args...>(rhs);

  return production { lhs, rhs };
}
```

And we can use this function to automatically create production rule from a function or lambda, like this example which converts two ints into a bool, for example maybe like an equality operator:

```
// The template arguments are automatically deduced!
MakeProduction(function([](int firstOperand, int secondOperand) -> bool {
    return firstOperand == secondOperand;
}));
```

So what was originally a labor intensive task of writing a producion rule, keeping track of its index, and associating that index with a function, can now be accomplished automatically by converting a function into a production rule. But how do we actually make this association take place? Recall the signature for parsegen's virtual function that handles production rules:

```
virtual any reduce(int prod, vector<any>& rhs);
```

Ultimately, every callback function that is registered ultimately will be called through this function. The `rhs` parameter of that function is what contains the arguments for our functions, and it turns out to be nontrivial to call an `function` with a `vector<any>` as input. In specific cases this isn't very difficult and can be accomplished like follows, for example with a function that takes two integers and returns an integer:

```
// An add function that takes two integers and returns an integer
function<int(int, int)> addFunc = [](int lhs, int rhs) -> int { return lhs + rhs; };

// Create a vector of anys to be used as input
vector<any> parameters = { 1, 2 };

// Call the function by casting each parameter
addFunc(any_cast<int>(parameters[0]), any_cast<int>(parameters[1]));
```

So as we can see, calling an `function` with a `vector<any>` requires performing an `any_cast` on each element of the array, and using the results of that operation as input. To more formally state the challenge, we must convert a function of the type `function<R(P0,P1,...,Pn)>` into `function<any(vector<any>&)>`. This is broadly part of a class of programming techniques known as *type erasure*, meaning static type information is removed from an object without losing access to the behavior of that type. It essentially amounts to hiding type information behind a more generic type. One might think of virtual functions as a kind of *type erasure*, because the information about the type of a derived class is hidden behind a pointer to a more generic class; this information is recovered when calling a dynamic function by a VTABLE. So how might we go about implementing such a type erasure system?

```
template<typename R, typename ...Args>
function<any(vector<any>&)> EraseFunctionType(function<R(Args...)> func) {

    // Capture the input function in the lambda so we can call it
    return [func](vector<any>& rhs) -> any {

        // Step 1 - convert rhs into its actual types using any_cast

        // Step 2 - call the function with those parameters

        func(/* Convert rhs into their actual types*/)
    };
}
```

The problem seems hopeless at first... but it is in fact possible. I'll give one hint before proceeding: if variadic templates are causing the problem, they are probably also necessary for the solution. So lets dive in. First, we will try and convert our `vector<any>` into a `tuple<Args...>` by expanding `Args`. After doing this, we will at least have the actual concrete types needed to be used as parameters. So how do we create this tuple? It's actually fairly straighforward and similar to the way we originally converted each of our `Args...` into strings in `RegTypes`, and our new function called `PackAnys` will fallow the exact same pattern:

```
template<int Index, typename ...Args>
typename enable_if<Index == 0, void>::type
inline PackAnys(tuple<Args...>&, vector<any>&) const { }

template<int Index, typename ...Args>
typename enable_if<Index == 1, void>::type
inline PackAnys(tuple<Args...>& args, vector<any>& anys) const {
    get<0>(args) = any_cast<decltype(get<0>(args))>(anys[0]);
}

template<int Index, typename ...Args>
typename enable_if<(Index > 1), void>::type
inline PackAnys(tuple<Args...>& args, vector<any>& anys) const {
    PackAnys<Index - 1, Args...>(args, anys);
    get<Index - 1>(args) = any_cast<decltype(get<Index - 1>(args))>(anys[Index - 1]);
}
```

So now we have accomplished Step 1 and `EraseFunctionType` looks like this:

```
template<typename R, typename ...Args>
function<any(vector<any>&)> EraseFunctionType(function<R(Args...)> func) {

    // Capture the input function in the lambda so we can call it
    return [func](vector<any>& rhs) -> any {
        tuple<Args...> args;
        PackAnys<sizeof...(Args), Args...>(args, rhs);

        // Step 2 - call the function with those parameters

        func(/* Convert rhs into their actual types*/)
    };
}
```

Now, we need a way to expand this tuple into our function parameters. The novel trick to do this is that variadic expansion can not be used merely on `Args`, but an expression involving `Args`. For example, `any_cast<int>(Args)...` would create an `any_cast` expression for each argument. This is quite powerful! Using standard library functions, we can create *another* variadic pack containing the indices `[0, N)`, and use `get` to unpack a value from the tuple:

```
template<typename Function, typename Tuple, size_t ... I>
auto call(Function f, Tuple t, index_sequence<I ...>) const {
    return f(get<I>(t) ...);
}

template<typename Function, typename Tuple>
auto call(Function f, Tuple t) const {
    static constexpr auto size = tuple_size<Tuple>::value;
    return call(f, t, make_index_sequence<size>{});
}
```

And finally, we've solved Step 2, yielding our final function:

```
template<typename R, typename ...Args>
function<any(vector<any>&)> EraseFunctionType(function<R(Args...)> func) {

    // Capture the input function in the lambda so we can call it
    return [func](vector<any>& rhs) -> any {
        tuple<Args...> args;
        PackAnys<sizeof...(Args), Args...>(args, rhs);

        return call(func, args); // The result is implicitly cast to any
    };
}
```

This function is accomplishes our goal of erasing the type information from our input function, and converting it into a function that takes a `vector<any>` and returns an `any`. Now below, shows a struct that can combines all these ingredients into a **frontend** that automatically packages `function<T>`s into production rules, adds them to the list of all productions, and calls those functions during the appropriate parsegen callback:

```
struct frontend {
    // These are the actual production rules given to parsegen
    vector<production> productions;

    // Map from parsegen production index to our callback functions
    map<int, function<any(vector<any>&)>> callbacks;

    template<typename T>
    void AddProduction(function<T> func) {

        // The index of the production we will add
        int index = static_cast<int>(productions.size());

        // Make a production rule and add it to the list
        productions.push_back(MakeProduction(func));

        // Map the index to the function
        callbacks[index] = EraseFunctionType(func);
    }

    inline virtual any reduce(int prod, vector<any>& rhs) override {

      // Call our erased functions
      return callbacks[prod](rhs);
    }
}
```

Of course, its never quite that simple, but this hopefully illustrates some of the core concepts that make such a frontend possible.
