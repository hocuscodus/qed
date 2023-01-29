QED - Simple code, fancy apps

The code is quite short but does a lot. It implements a yellow oval increment/decrement widget with a count in the middle and two buttons, - and +, on its sides. A user may press the + and - buttons to modify the count. The yellow widget turns green on a positive count and red on a negative count.

```
void Button(String text) {
  float trp = 80%

  <out: rect; transparency: trp; onPress: trp = 69%; onRelease: {trp = 80%; return()} size: [35, AS_IS];>
  <out: text; transparency: 60%; align: 50%; fontSize: 40;>
}

int count = 0
Button decButton = new Button("-") -> count--
Button incButton = new Button("+") -> count++

<out: oval; color: (count >= 0 ? COLOR_GREEN : 0) | (count <= 0 ? COLOR_RED : 0);>
<_ color: COLOR_BLACK;
  <out: decButton;>
  <.
    <size: [100, 80];>
    <out: count; transparency: 50%; align: 50%; fontSize: 50;>
  >
  <out: incButton;>
>
void Button(String text) {
  float trp = 0.8

  <out: rect; transparency: trp; onPress: trp = 0.69; onRelease: {trp = 0.8; return()}> // size: [35,];>//
  <out: text; transparency: 0.6; align: 0.5; fontSize: 40;>
}

int count = 0
var decButton = new Button("-") -> count = count - 1//count--
var incButton = new Button("+") -> count = count + 1//count++

<out: oval; color: (count >= 0 ? COLOR_GREEN : 0) | (count <= 0 ? COLOR_RED : 0);>
<_ color: COLOR_BLACK;
  <out: decButton;>
  <.
    //<size: [100, 80];>
    <out: count; transparency: 0.5; align: 0.5; fontSize: 50;>
  >
  <out: incButton;>
>
```

The goal of this article is to describe the source code logic, since both QED syntax and semantics differ in part from most programming languages. The syntax is mostly C-styled but there is some user interface directives as well (code surrounded by angle brackhets <>). As we'll explore the code, we'll encounter the semantic deviations as well.

The code starts with a Button function (having a String parameter and no return value), followed by a script that uses it. Both function and script have UI directives at their end, which we'll explain later when the widget UI is generated.

So the program executions starts with the script. Three variables are declared. The first one, `count` is an int holding the count, initialized to 0. The two other variables, `decButton` and `incButton`, are instances of class Button, one for the minus button, one for the plus button. But Button is a function, not a class, how come? In QED, classes are defined as functions which names starts with a capitalized letter. A QED function can be used either as a class if the `new` operator is used and as a normal function if `new` does not precede the call. In both cases, the function body is executed, so the col variable is defined and initialized (Button UI directives are kept for a later stage). The difference lies when the function execution terminates. A `Button` call would need the `return` call to resume caller execution whereas a `Button` instance immediately returns its entire call stack, converted to an object. Button locals `text` and `col` become instance fields in script variables `decButton` and `incButton`.

After defining its three variables the script would normally continue its execution but there are no more instructions. If a `return` call was there, the script would complete like a normal program but in QED, the absence of `return` does not mean an implicit void return (or exit if no caller like in the script). The script cannot return immediately, so it will wait. Wait for what? User input. To do so, prior to waiting, a user interface is generated and presented to the user for interacting with the application.

QED uses the UI directives in the source code to generated the app user interface. For the script and any QED function, there could be a list of UI directives to indicate how to implement the function or script instance. Inside each UI directive, there could be a hierarchy of child UI directives. In the end, they form a tree with an unseen, implicit root.

The script has a simple tree of UI directives.

The Button function has an even simpler UI directive tree.

So to resume execution after the code completes, we generate a user interface before waiting for user input, It will start with the scrit UI directives. The first directive displays an oval which color is green, red or yellow (or'ed green and red) depending on count's value. Initially, it will be yellow since count is 0. The second directive displays three horizontal components over the oval, namely our three variables in the right order (`decButton`, `count` and `incButton`).

To display both `decButton` and `incButton` instances, QED referts to their UI directives defined in the `Button` function body. A rectangle is first drawn, which color is the variable `col` (which is modified upon mouse pressing and releasing, to show button clicking). On top of the rectanble, the text is displayed, namely `-` for `decButton` and `+` for `incButton`.

User input

Other UI attributes...

How does QED know the size of the oval?

Conclusion