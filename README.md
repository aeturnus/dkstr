# dkstr
Pathfinding hardware accelerator for EE 382N.4 Advanced Microcontroller Systems

## Directory structure

### app/
Contains the source of the example application that can run pathfinding
examples in a software implementation or on the hardware accelerator.

To compile with interrupts enabled, uncomment the line in
`app/src/dkstr.c` that defines `INTERRUPT`.

Run make to compile it.

There are several commands the program runs. For their arguments,
run the command with no other arguments to see a printout of the arguments.

`put_map`: places a map file's weights into BRAM

`dump_map`: prints out a map file as a hex dump. You can use to as an input for the Icarus Verilog simulation in `app/source`.

`put_path`: places a path's information into BRAM (no start coordinates at the beginning)

`print_path`: prints the path information in a visual manner. An example is under `app/maps/test1.path`

`play`: runs a pathfinding algorithm for a given map, with a selectable SW or HW implementation. An example map is provided under `app/maps/test1.map`.

`playback`: plays a paths file that contains the starting coordinates at the beginning of the file. An example is provided under `app/paths/paths.hex` (this is `test1.path` except with starting coordinates at the beginning).

`rand`: generates a random map and works like `play`

`profile`: profiles a given implementation, printing out stats

### kmod/
Kernel module code to expose the interrupt to user code.
Run the Makefile and use the install and uninstall scripts to
create/remove the required nodes and insert/remove the kernel module.

### source/
Verilog source for the Node Execution Unit (`source/src/neu.v`) and Fabric (`source/src/fabric.v`).
These are the core modules of the project.
The Makefile has targets to build and run testbenches for the NEU and Fabric.
As it stands in the `source/test/test_fabric.v` file, the input data will come from `source/test28.hex`.
This input data is a packed format containing node weights for a map. This file can be produced by
invoking the "dump" command from the example application above.

The Fabric testbench will produce a `paths.hex`: this hexdump will be what the
accelerator is expected to output for a given input data, with the exception of the
first two lines which serve as the starting x and starting y position in the map.
The remaining lines after this are what is to be expected to appear in BRAM on the
physical implementaion.

The example application can also playback a given `paths.hex` file.

Dependencies: iverilog

Invoke `make test_fabric.iver`.

Not advised to run this simulation on the ZedBoard.

### vivado/
Vivado specific files to be source controlled. These three files serve as the Verilog code
for the accelerator's IP Package code.

### doc/
Assorted documentation. `neu_fabric.xml` is a draw.io file.
