test_neu.iver: src/neu.v test/test_neu.v
	iverilog -o $@ $^
	vvp $@

clean:
	rm -f *.iver
	rm *.vcd

.PHONY:	all clean
