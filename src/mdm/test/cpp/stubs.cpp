#include <tcl/tcl.h>

#include "ord/OpenRoad.hh"

char** cmd_argv;
int cmd_argc;
const char* log_filename = "";
const char* metrics_filename = "";

static const char* init_filename = ".openroad";

int ord::tclAppInit(Tcl_Interp* interp)
{
  return -1;
}
