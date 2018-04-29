test_neu.iver: src/neu.v test/test_neu.v
	iverilog -o $@ $^
	vvp $@

test_fabric32.iver: src/neu.v src/fabric32.v test/test_fabric32.v
	iverilog -Isrc -o $@ $^
	vvp $@

clean:
	rm -f *.iver
	rm *.vcd

.PHONY:	all clean
