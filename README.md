This are some experiments based on the ICFP 2006 competition.

See [http://www.boundvariable.org/] http://www.boundvariable.org/ for more
information (and a paper about the goals of the competition).

Nothing fancy there, mostly a simple implementation of a VM with some
interestig properties.

Besides the VM itself, what was interesting for me was the debugging phase
that I had to go through when developping the VM. Debugging is no fun and the
absence of a REPL or any kind of runtime VM inspector was really "annoying".
I therefore developed some kind of a debugger, with a simple language associated
to it. The VM can be run from within the debugger by adding the parameter "-d"
to the command line.

What the debugger allowed me to play with (very simple stuff):
     * parser / <b>stack based interpreter</b> for the debugger command line. It runs a simple
     VM also that interprets the command. The command is manually parser through
     a simple <b>recursive descent parser</b> (bad one), and the opcode stack is built
     through a simple <b>post-order traversal of the parse tree</d>
     * <b>buddy memory</b> allocator (http://en.wikipedia.org/wiki/Buddy_memory_allocation)
     * simple garbage collector
     * <b>Schorr / Waite</b> tree traversal

Everything is pretty naive. It is not rocket science (yet to come).



