#include "TCL.h"

//Tcl_ObjType TCL::net_objtype_, TCL::cell_objtype_, TCL::pin_objtype_, TCL::board_objtype_;
decltype(TCL::cell_props_) TCL::cell_props_;
decltype(TCL::pin_props_) TCL::pin_props_;
decltype(TCL::net_props_) TCL::net_props_;
//decltype(TCL::board_props_) TCL::board_props_;
TCL * TCL::this_s_;
