Finite Automata
---------------

Regular expressions and finite automata.

## Regular expression syntax

The supported regular expressions are:
```text
0       empty language
1       epsilon
a       literal symbol
r+s     union
rs      concatenation
r*      Kleene star
(r)     grouping
```

Operator precedence is:
```text
*       highest
concat
+       lowest
```

For example:
```text
(a+b)*abb
```
describes all strings over `{a, b}` ending in `abb`.

The characters
```text
0 1 + * ( )
```
are reserved syntax. Escaping is not supported.

## Building

Building requires a C17 compiler (clang):
```sh
make
```

The executable is written to `build/automata`.

## Usage

The command line operates on a stack of finite automata.

`regex <expression>` constructs an ε-NFA and pushes it onto the stack:
```sh
./build/automata regex 'a+b'
```

If the command finishes with one automaton on the stack,
that automaton is written to standard output as Graphviz DOT.
For example, the regex `a+b` produces the following Thompson ε-NFA:

![Thompson ε-NFA for `a+b`](./assets/aplusb.png)

### Unary operations
```text
det      NFA -> DFA
nfa      DFA -> NFA
min      DFA -> DFA
rev      NFA -> NFA, DFA -> NFA
compl    DFA -> DFA
star     NFA -> NFA
```

Operations are applied from left to right; each operation requires the appropriate automaton type.
For example, to determinize and then minimize:
```sh
./build/automata regex '(a+b)*abb' det min
```
This yields the following DFA:

![Minimized DFA for `(a+b)*abb`](./assets/detmin.png)

To reverse an NFA directly:
```sh
./build/automata regex 'ab' rev
```

To determinize, complement, convert back to an NFA, and apply Kleene star:
```sh
./build/automata regex 'ab' det compl nfa star
```

### Binary operations

Multiple regular expressions may be pushed onto the stack, then binary operations turn two
automata into one, pushing back the result:
```text
union    NFA NFA -> NFA, DFA DFA -> DFA
concat   NFA NFA -> NFA
inter    DFA DFA -> DFA
diff     DFA DFA -> DFA
xor      DFA DFA -> DFA
```

For example, NFA union:
```sh
./build/automata regex 'a' regex 'b' union
```

Concatenation:
```sh
./build/automata regex 'a+b' star regex 'abb' concat
```

Difference is left minus right:
```sh
./build/automata regex '(a+b)*' det regex 'a*' det diff
```

### Queries

Queries are terminal operations, as they produce a boolean rather than an automaton.

```text
accept <word>  NFA -> result
               DFA -> result

empty     NFA -> result, DFA -> result
equiv     DFA DFA -> result
subset    DFA DFA -> result
```

Test whether a word is accepted:
```sh
./build/automata regex '(a+b)*abb' accept 'aabb'
```

Test whether two languages are equivalent:
```sh
./build/automata regex 'a+b' det regex 'b+a' det equiv
```

For subset, the operands are interpreted as left ⊆ right.
Predicate queries exit with status 0 when true, 1 when false, and 2 on an error.

## Tests

Tests use Criterion. Run the suite with:

```sh
make test
```

## Coverage

Coverage uses Clang source-based coverage:

```sh
make coverage
```

For annotated source output:

```sh
make coverage-show
```
