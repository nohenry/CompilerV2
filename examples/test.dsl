// Example of a line comment

/*
    Example of a multiline documentation comment

    @param parameters the function takes
    @return the return value of the function

    # Feel free to using `markup` styling in this comment
*/


import std.os.*; // Example of importing directly into current module's namespace
import std.fs; // Example of importing into current module's namespace but through the 'fs' namespace

spec Open {
    function Open();
}

spec Close {
    function Close();
}

template Window {
    uint16 width;
    uint16 height;
}

action Window {
    function DoSomething() => {

    }
}

action Close in Window {
    function Close() => {

    }
}


function value() => uint {
    return 50;
}

function main() => {
    uint foo = 3; // Example of an unsigned integer variable
    let uint bar = 5; // Example of a constant unsigned integer variable

    branch foo {
        when 0: println("Zero");
        when 1: println("One");
        when 2: println("Two");
        when 3: println("Three");
    } else {
        println("Larger than three!");
    }

    if bar == 5 {
        println("Bar is 5");
    }

    uint &fooReference = foo; 
    let uint &constantFooReference = foo; // Compiler will get mad because we have a constant refernce to foo but also a non constant reference to foo
                                          // The non constant reference could be used to change the value of foo when the constant reference thinks foo shouldn't change
                                          // This is an error the compiler will catch
    
    loop x in [0-5] {
        println("Loop range");
    }

    bool go = true;

    loop go {
        go = false;
    }

    loop; // infinite loop
}

function <T> GenericFunction(T &param) => {
    println("Generic function type {}", typeof(T).name);
}

template <T> GenericTemplate {
    T fieldA;
    T fieldB;
}

action <T> GenericTemplate<T> {
    function GenericTemplateAction(T &value) => {
        println("Generic Template action type {}", typeof(T).name);
    }
}

template <T: Open> GenericTemplateConstraint {
    T fieldA;
    T fieldB;
}

function <T: Open & Close> GenericConstraint(T &param) => {
    param.Open();
    param.Close();
}

/* ------------------------- Tags: ---------------------------
   Tags are a very useful way of specifying useful metadata or
   custom information to the compiler about the module, function, or template.

   They can also be used as preprocessor defines as well as macros.
*/

[http.Get('/index.html')]
function IndexRoute() => {

}

if [platform.Windows] {
    function WindowsSpecific() => {

    }
}

function RepeatTag => {
    [actions.Repeat([0-5])] => { // Repeat code in the brackets for the specified range
        int v{i} = i;
    }
    /*
        Produces:

        int v0 = 0;
        int v1 = 1;
        int v2 = 2;
        int v3 = 3;
        int v4 = 4;
    */
}