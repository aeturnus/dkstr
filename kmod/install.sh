#!/bin/bash
insmod dkstr_int.ko
mknod /dev/dkstr_int c 243 0
