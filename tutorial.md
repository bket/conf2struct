Ok, so you've written this wonderful tool, and it uses a
configuration file. Because you didn't re-invent the wheel,
you used
[libconfig](https://hyperrealm.github.io/libconfig/). But
because you need to access the configuration data quickly,
you want it in a `struct` or other native C data structures
rather than having to lookup the settings through
libconfig's interface.

That's the first thing `conf2struct` will do for you.
Suppose your configuration file looks something like the
example given in libconfig's documentation:

```
version = "1.0";

application:
{
  window:
  {
    title = "My Application";
    size = { w = 640; h = 480; };
    pos = { x = 350; y = 250; };
  };

  list = ( ( "abc", 123, true ), 1.234, ( /* an empty list */ ) );

  books = ( { title  = "Treasure Island";
              author = "Robert Louis Stevenson";
              price  = 29.95;
              qty    = 5; },
            { title  = "Snow Crash";
              author = "Neal Stephenson";
              price  = 9.99;
              qty    = 8; } );

  misc:
  {
    pi = 3.141592654;
    bigint = 9223372036854775807L;
    columns = [ "Last Name", "First Name", "MI" ];
    bitmask = 0x1FC3;	// hex
    umask = 0027;	// octal. Range limited to that of "int"
  };
};
```

Instead of defining your `struct` and writing reams of code that reads the
configuration into it, simply write a description of your
configuration file and give it to `conf2struct`. Because
`conf2struct` didn't re-invent the wheel, this is done using
libconfig as well. So here we describe all the data that's
in our configuration file:

```
header: "example.h";
parser: "example.c";

config: {
name: "eg";
items: (
        { name: "version"; type: "string"; },
        { name: "application"; type: "group", items: (
                 { name: "window"; type: "group", items: (
                        { name: "title"; type: "string"; },
                        { name: "size" ; type: "group"; items: (
                             { name: "w"; type: "int" },
                             { name: "h"; type: "int" }
                        ) },
                        { name: "pos" ; type: "group"; items: (
                             { name: "x"; type: "int" },
                             { name: "y"; type: "int" }
                        ) }
                 ) },
                 { name: "books"; type: "list"; items: (
                      { name: "title"; type: "string" },
                      { name: "author"; type: "string" },
                      { name: "price"; type: "float" },
                      { name: "qty"; type: "int" }
                 ) },

                 { name: "misc"; type: "group"; items: (
                      { name: "pi"; type: "float" },
                      { name: "bigint"; type: "int64" },
                      { name: "columns"; type: "array"; element_type: "string" },
                      { name: "bitmask"; type: "int" },
                      { name: "umask"; type: "int" }
                 ) }
        ) }
    )
}
```

This is pretty straigh-forward: we have a `config` element
that contains a name and a list of items which describe
what's in the root of configuration file. Each item is
described as a name and type. The two special types `list`
and `group` contain a further list of `items`, which
themselves describe what's in the list.

Here we have a limitation: we do not support lists of items
that have no fixed type. We hope that's not too bad in the
real world.

Two additional settings, `header` and `parser`, specify the
output files into which `conf2struct` will write the type
definitions and the parser code, respectively.

So we end up with a `example.h` which defines types for each
level of the configuration. Each type is prefixed with the
`name` set in the `config` setting, in our case `eg`:

```
/* Generated by conf2struct (https://www.rutschle.net/tech/conf2struct/README)
 * on Wed Jan 16 22:27:52 2019. */
 
#ifndef C2S_EG_H
#define C2S_EG_H
#include <libconfig.h>


struct eg_application_window_size_item {
	int	w;
	int	h;
};

struct eg_application_window_pos_item {
	int	x;
	int	y;
};

struct eg_application_window_item {
	const char*	title;
	struct eg_application_window_size_item* size;
	struct eg_application_window_pos_item* pos;
};

struct eg_application_books_item {
	const char*	title;
	const char*	author;
	double	price;
	int	qty;
};

struct eg_application_misc_item {
	double	pi;
	long long	bigint;
	size_t	columns_len;
	const char** columns;
	int	bitmask;
	int	umask;
};

struct eg_application_item {
	struct eg_application_window_item* window;
	size_t	books_len;
	struct eg_application_books_item* books;
	struct eg_application_misc_item* misc;
};

struct eg_item {
	const char*	version;
	struct eg_application_item* application;
};

int eg_parse_file(
        const char* filename,
        struct eg_item* eg, 
        const char** errmsg);

void eg_print(
    struct eg_item *eg,
    int depth);

int eg_cl_parse(
    int argc,
    char* argv[],
    struct eg_item *eg);

#endif
```

We also get the prototypes to 3 functions: a file parser, a
pretty-printer, and a command-line parser (more on that
later). Now, reading a configuration file is as simple as
calling `eg_parse_file()` on a file, and you'll get a
structure all filled up, with variable-length items
allocated and all.

Now wouldn't it be nice if we had a command-line as well,
which read in the same `struct`, and in fact allowed us to
override the settings in the configuration file?

That's what you get for free with `eg_cl_parse()`. Because
`eg_cl_parse()` is based on
[argtable3](https://www.argtable.org/), it also
pretty-prints the options and allows you to define
descriptions for each options, which you'll add in the
`conf2struct` configuration file. There is also a
`conffile_option` which allows to tell `conf2struct` which
option should be defined to read the configuration file. So
now we have: 

```
header: "example.h";
parser: "example.c";

conffile_option: ( "F", "conffile" );

config: {
name: "eg";
items: (
        { name: "version"; type: "string"; description: "Specify version number" },
        { name: "application"; type: "group", items: (
                 { name: "window"; type: "group", items: (
[...]
                 ) }
        ) }
    )
}
```

And now running the application with no command-line
parameters will print a nice usage message:

```

$ ./example -?
eg: invalid option "-?"
 [-F <file>] [--version=<str>] [--application-window-title=<str>]... [--application-window-size-w=<n>]... [--application-window-size-h=<n>]... [--application-window-pos-x=<n>]... [--application-window-pos-y=<n>]... [--application-books-title=<str>]... [--application-books-author=<str>]... [--application-books-price=<n>]... [--application-books-qty=<n>]... [--application-misc-pi=<n>]... [--application-misc-bigint=<n>]... [--application-misc-columns=<str>]... [--application-misc-bitmask=<n>]... [--application-misc-umask=<n>]... [--size=<string>]... [--book=<string>]...
  -F, --conffile=<file>         Specify configuration file
  --version=<str>               Specify version number
  --application-window-title=<str>      Specify window title
  --application-window-size-w=<n>
  --application-window-size-h=<n>
  --application-window-pos-x=<n>
  --application-window-pos-y=<n>
  --application-books-title=<str>
  --application-books-author=<str>
  --application-books-price=<n>
  --application-books-qty=<n>
  --application-misc-pi=<n>
  --application-misc-bigint=<n>
  --application-misc-columns=<str>
  --application-misc-bitmask=<n>
  --application-misc-umask=<n>
```

As you can see, `conf2struct` has generated `accessors' for
each setting, including nested settings. Lists can be filled
by specifying the same option several times, and groups are
filled by specifying several options. If a configuration
file is specified, it is read first and command line options
will override its settings. Otherwise, unspecified options
can have default values coming from the specification file
(see full documentation in the README).

Here we set the window size without a configuration file, so
most of the `struct` is empty:

```
$ ./example --application-window-size-w 500 --application-window-size-h 1000
application:
    window:
        title: (null)
        size:
            w: 500
            h: 1000
        pos:
            x: 0
            y: 0
    books [0]:
    misc:
        pi: 0.000000
        bigint: 0
        columns [0]:
        bitmask: 0
        umask: 0

```

Here we set several books from the command line:

```
$ ./example --application-books-title "Lord of the Rings" --application-books-author "JRR Tolkien" --application-books-title "A song of fire and ice" --application-books-author "GRR Martin"
from command line:
version: (null)
application:
    window:
        title: (null)
        size:
            w: 0
            h: 0
        pos:
            x: 0
            y: 0
    books [2]:
        title: Lord of the Rings
        author: JRR Tolkien
        price: 0.000000
        qty: 0
        title: A song of fire and ice
        author: GRR Martin
        price: 0.000000
        qty: 0
    misc:
        pi: 0.000000
        bigint: 0
        columns [0]:
        bitmask: 0
        umask: 0
```

Setting groups like this is a little inconvenient: users
might feel like throwing stones at us, and we don't want
that. So we can specify compound options which will set an
entire group at once, based on a regular expression that
explains how to parse the option value, and a mapping to the
group field names. So we define compound options to set
window sizes and books:

```
cl_groups: (
       { name: "size"; pattern: "([[:digit:]]+),([[:digit:]]+)";
         list: "application.window.size";
         targets: (
                { path: "w"; value: "$1"; },
                { path: "h"; value: "$2"; }
         );
       },
       { name: "book";
         pattern: "(.+),(.+),([0-9.]+),([[:digit:]]+)";
         list: "application.books";
        override: "title";
         targets: (
                   { path: "title"; value: "$1" },
                   { path: "author"; value: "$2" },
                   { path: "price"; value: "$3" },
                   { path: "qty"; value: "$4" }
         );
       }
)
```

Here we have defined a `size` option which takes a value
composed of 2 integers separated by a coma. This will set
fields in the `application.window.size`, in which `w` will
take the first value matched (`$1`), and `h` will take the
second value.

We also defined a `book` option which will set all the
fields of an `application.books` entry. Because we specified
`override: "title"`, any command-line setting `--book` will
check if the `title` field specified on the command-line
matches one already set in the configuration file, and in
that case it will replace that setting. Otherwise, it'll add
a new entry to the list of books.

So we can add books to those in the configuration file:
```
$ ./example -F example.cfg --book "Lord of the Rings","JRR Tolkien",25.95,5
from command line:
version: 1.0
application:
    window:
        title: My Application
        size:
            w: 640
            h: 480
        pos:
            x: 350
            y: 250
    books [3]:
        title: Treasure Island
        author: Robert Louis Stevenson
        price: 29.950000
        qty: 5
        title: Snow Crash
        author: Neal Stephenson
        price: 9.990000
        qty: 8
        title: Lord of the Rings
        author: JRR Tolkien
        price: 25.950000
        qty: 5
    misc:
        pi: 3.141593
        bigint: 9223372036854775807
        columns [3]:
            0:  Last Name
            1:  First Name
            2:  MI
        bitmask: 8131
        umask: 27
```

Or we can modify entries that were already set:
```
t$ ./example -F example.cfg --book "Treasure Island","Stevenson",13.95,3
from command line:
version: 1.0
application:
    window:
        title: My Application
        size:
            w: 640
            h: 480
        pos:
            x: 350
            y: 250
    books [2]:
        title: Treasure Island
        author: Stevenson
        price: 13.950000
        qty: 3
        title: Snow Crash
        author: Neal Stephenson
        price: 9.990000
        qty: 8
    misc:
        pi: 3.141593
        bigint: 9223372036854775807
        columns [3]:
            0:  Last Name
            1:  First Name
            2:  MI
        bitmask: 8131
        umask: 27
```

All of this is done by the tiny code in `parser.c`, which
only calls `eg_cl_parse()` and then `eg_print()` the result:

```
#include <string.h>
#include <stdlib.h>
#include <libconfig.h>

#include "example.h"

void main(int argc, char* argv[]) {
    struct eg_item config, config_cl;
    const char* err;
    int res;

    res = eg_cl_parse(argc, argv, &config_cl);
    if (!res) {
        exit(1);
    }
    printf("from command line:\n");
    eg_print(&config_cl,0);
}
```

We just need to add `example.c` and `argtable3.c` to our
build, link `libconfig`, and we're good to go!
