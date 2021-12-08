# textx-cpp

## Plan

 * Create an interpreter for [textx](https://github.com/textX/textX) grammars.
 * This is a just-for-fun project and a proof of concept...
 * Example: see [test/model.t.cpp](test/model.t.cpp)
## Details

### arpeggio.h

 * Responsibility: basic parser functionality, inspirited by [Arpeggio](https://github.com/textX/Arpeggio).
 * Code config, arpeggio.h: `#define ARPEGGIO_USE_BOOST_FOR_REGEX` activates the boost version of regex; else the std-lib version is used. Since the boost is *much* faster, this define is introduced (+CMakeLists.txt adaptations).

### assert.h

 * Responsibility: exceptions tools with information about model/file location.

### grammar.h

 * Responsibility: container for rules and interface to trigger parsing.

### lang.h

 * Responsibility: migration of [lang.py](https://github.com/textX/textX/blob/master/textx/lang.py)
   (the textx grammar language).

### metamodel.h

 * Responsibility: extended user-grammar representation. Allows to load a model (inspired by metamodel.py).

### model.h

 * Responsibility: 
   - user model representation (inspired by model.py).
   - user model parsing (inspired by model.py).

### object.h

 * Responsibility: simple model representation tree (resides inside a textx::Model).

### rule.h

 * Responsibility: textx user grammar rule representation (extends a simple textx::apreggio::Pattern with, e.g. attribute and attribute type information).

### textx_grammar_parsetree.h

 * Responsibility: internal helper to parse user grammars.
## Links

 * Textx, Arpeggio, etc: [https://github.com/textX](https://github.com/textX)
 * Motivation for basic Arpeggio re-impl: [https://blog.bruce-hill.com/](https://blog.bruce-hill.com/packrat-parsing-from-scratch)

## Open Points

### Unclear
### Open
 * TODO minor: it must be possible for "eolterm" to be combined with a separator pattern
 * TODO: use has_match_suppression
 * include/reference other metamodels
 * TODO handle referenced/included metamodels, in Metamodel::ref
 * TODO importURI
 * RREL
 * unordered group with choices
 * boolean assignment (at the moment we have null/"" or the value instead of true/false)
