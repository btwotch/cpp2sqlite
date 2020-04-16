# CPP2SQLITE

Parse C++ and store that information in a sqlite database.

Currently following information is stored:
* class declarations
* class inheritance
* member functions and arguments
* global functions
* mangled names

Moreover dynamic tracing (via -finstrument-functions) is possible; following information is stored:
* calling address
* callee address
* addresses of symbols

## Output

* plantuml class diagram
* plantuml sequence diagram
* dot file

## More info
```
./cpp2sqlite --help-all
```
