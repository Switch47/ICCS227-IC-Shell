TARGET=icsh
make:
    gcc -o  icsh icsh.c
clean:
    rm$(TARGET)