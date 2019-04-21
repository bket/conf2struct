/* Example parser for conf2struct, using sslh configuration file. */

#include <string.h>
#include <stdlib.h>
#include <libconfig.h>

#include "c2s.h"

void main(int argc, char* argv[]) {
    struct eg_item config, config_cl;
    const char* err;
    int res;

    res = eg_cl_parse(argc, argv, &config_cl);
    if (!res) {
        exit(1);
    }
    eg_fprint(stdout, &config_cl,0);
}
