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

The basic command constructs an ε-NFA and writes to stdout as Graphviz DOT:

```sh
./build/automata regex 'a+b'
```

Additional automaton operations may be chained after the regular expression:

```text
dfa          determinize the current automaton
min          minimize the current automaton
reverse      reverse the language of the current automaton
complement   complement the language of the current automaton
```

Operations are applied from left to right. For example, to first minimize then reverse:
```text
./build/automata regex '(a+b)*abb' min reverse
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
