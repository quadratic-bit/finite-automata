Finite Automata
---------------

A C17 implementation of regular expressions and finite automata.

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

## Usage

The basic command constructs an ε-NFA and writes it to standard output as Graphviz DOT:

```sh
./build/automata regex 'a+b'
```

Additional automaton operations may be chained after the regular expression:

```text
det      NFA -> DFA
min      DFA -> DFA
rev      NFA -> NFA, DFA -> NFA
compl    DFA -> DFA
```

Operations are applied from left to right; each operation requires the appropriate automaton type.
For example, to determinize and then minimize:

```sh
./build/automata regex '(a+b)*abb' det min
```

To reverse an NFA directly:

```sh
./build/automata regex 'ab' rev
```

To determinize, complement, and then reverse:

```sh
./build/automata regex 'ab' det compl rev
```

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
