# Qbrt Core Modules

Here are the core qbrt modules that are currently implemented.
Much about these modules are very likely to change before qbrt becomes
stable.

## io

Programs that cannot interact with the outside world are not
very much fun. This is the library that allows input and output.

### print

Print a register to stdout. This is here during testing and likely to
go away before long.

Parameters:

* **value** - the value that should be printed

Returns:

Nothing

### open

Open a file for reading or writing.

Parameters:

* **filename** - the name of the file to be opened
* **mode** - the mode in which the file should be opened (see fopen man page for possible mode values)

Returns:

A stream associated with the open file.

### readline

Read a line of input from the given file. Can be stdin, stdout or stderr.

Parameters:

* **file** - the open file stream from which to read

Returns:

A string containing characters from one line of the given file.
Currently excludes the newline character from the end of the line
but this is likely to change.

### write

Write a string to an open file stream.

Parameters:

* **stream** - the open file stream where the output should be written
* **text** - the text string to be written to the open stream

Returns:

Nothing
