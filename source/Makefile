test_neu.iver: src/neu.v test/test_neu.v
	iverilog -o $@ $^
	vvp $@

test_fabric.iver: src/neu.v src/fabric.v test/test_fabric.v
	iverilog -Isrc -o $@ $^
	vvp $@

clean:
	rm -f *.iver
	rm *.vcd

.PHONY:	all clean
