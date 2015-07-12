# utfhate
Simple command-line utility to find, mark, count, replace and delete instances of UTF-8 characters in the input stream.

## Usage
utfhate currently reads input from **stdin** and writes all output to **stdout**.

```
utfhate [options] < inputfile
```
or
```
utf8-spewer | utfhate -d
```

### Options
* No options
  - Defaults to the search mode

* --help, -h
  - Displays all available options.

* --search, -s
  - Searches for UTF-8 characters and, if found, marks their location.

* --delete, -d
  - Deletes all UTF-8 characters found in the input without destroying the source.

* --replace, -r
  - Replaces all UTF-8 characters with a specified value.

* --count, -c
  - Counts the number of UTF-8 characters in the input. Modes: 'chars' (default), 'bytes', 'both'.

* --verbose, -v
  - Enables verbose output command output.
  
## Building
Requirements: a C90 compatible compiler e.g. GCC or clang.
```
make
```
To install to /usr/local/bin
```
sudo make install
```
