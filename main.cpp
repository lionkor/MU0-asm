#include "debug.h"
#include "Parser.h"

int main(int argc, char** argv) {
    if (argc == 1) {
        fatal("no input file specified");
        return -1;
    }

    Parser parser(argv[1]);
    if (parser.invalid()) {
        fatal("errors occurred during initial parsing.");
        return -1;
    } else {
        log("initial parsing ok");
    }

    parser.parse_all();

    parser.write_asm_to("a.asm");
    parser.write_to("a.out");

    return 0;
}
